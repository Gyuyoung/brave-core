/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/wallet/wallet_create.h"

#include <vector>

#include "base/json/json_reader.h"
#include "bat/ledger/internal/common/security_helper.h"
#include "bat/ledger/internal/common/time_util.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/request/request_promotion.h"
#include "bat/ledger/internal/request/request_util.h"
#include "bat/ledger/internal/state/state_util.h"
#include "net/http/http_status_code.h"

using std::placeholders::_1;

namespace {

ledger::Result ParseResponse(
    const std::string& response,
    std::string* payment_id) {
  DCHECK(payment_id);

  base::Optional<base::Value> value = base::JSONReader::Read(response);
  if (!value || !value->is_dict()) {
    return ledger::Result::LEDGER_ERROR;
  }

  base::DictionaryValue* dictionary = nullptr;
  if (!value->GetAsDictionary(&dictionary)) {
    return ledger::Result::LEDGER_ERROR;
  }

  const auto* payment_id_string = dictionary->FindStringKey("paymentId");
  if (!payment_id_string || payment_id_string->empty()) {
    BLOG(1, "Payment id is wrong");
    return ledger::Result::LEDGER_ERROR;
  }

  *payment_id = *payment_id_string;
  return ledger::Result::LEDGER_OK;
}

}  // namespace

namespace braveledger_wallet {

WalletCreate::WalletCreate(bat_ledger::LedgerImpl* ledger) : ledger_(ledger) {
}

WalletCreate::~WalletCreate() = default;

void WalletCreate::Start(ledger::ResultCallback callback) {
  const auto payment_id = braveledger_state::GetPaymentId(ledger_);
  const auto stamp = ledger_->GetCreationStamp();

  if (!payment_id.empty() && stamp != 0) {
    BLOG(1, "Wallet already exists");
    callback(ledger::Result::WALLET_CREATED);
    return;
  }

  auto key_info_seed = braveledger_helper::Security::GenerateSeed();
  braveledger_state::SetRecoverySeed(ledger_, key_info_seed);

  std::vector<uint8_t> secret_key =
      braveledger_helper::Security::GetHKDF(key_info_seed);
  std::vector<uint8_t> public_key;
  std::vector<uint8_t> new_secret_key;
  braveledger_helper::Security::GetPublicKeyFromSeed(
      secret_key,
      &public_key,
      &new_secret_key);

  std::string public_key_hex =
      braveledger_helper::Security::Uint8ToHex(public_key);

  auto url_callback = std::bind(&WalletCreate::OnCreate,
      this,
      _1,
      callback);

  const auto headers = braveledger_request_util::BuildSignHeaders(
      "post /v3/wallet/brave",
      "",
      public_key_hex,
      braveledger_state::GetRecoverySeed(ledger_));

  const std::string url = braveledger_request_util::GetCreateWalletURL();
  ledger_->LoadURL(
      url,
      headers,
      "",
      "application/json; charset=utf-8",
      ledger::UrlMethod::POST,
      url_callback);
}

void WalletCreate::OnCreate(
    const ledger::UrlResponse& response,
    ledger::ResultCallback callback) {
  BLOG(6, ledger::UrlResponseToString(__func__, response));

  if (response.status_code != net::HTTP_CREATED) {
    callback(ledger::Result::BAD_REGISTRATION_RESPONSE);
    return;
  }

  std::string payment_id;
  ParseResponse(response.body, &payment_id);
  braveledger_state::SetPaymentId(ledger_, payment_id);

  ledger_->SetRewardsMainEnabled(true);
  ledger_->SetAutoContributeEnabled(true);
  ledger_->ResetReconcileStamp();
  if (!ledger::is_testing) {
    braveledger_state::SetFetchOldBalanceEnabled(ledger_, false);
  }
  braveledger_state::SetCreationStamp(
      ledger_,
      braveledger_time_util::GetCurrentTimeStamp());
  braveledger_state::SetInlineTippingPlatformEnabled(
      ledger_,
      ledger::InlineTipsPlatforms::REDDIT,
      true);
  braveledger_state::SetInlineTippingPlatformEnabled(
      ledger_,
      ledger::InlineTipsPlatforms::TWITTER,
      true);
  braveledger_state::SetInlineTippingPlatformEnabled(
      ledger_,
      ledger::InlineTipsPlatforms::GITHUB,
      true);
  callback(ledger::Result::WALLET_CREATED);
}

}  // namespace braveledger_wallet
