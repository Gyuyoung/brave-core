/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/wallet/wallet_claim.h"

#include <string>
#include <utility>

#include "base/strings/stringprintf.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/request/request_promotion.h"
#include "bat/ledger/internal/request/request_util.h"
#include "bat/ledger/internal/state/state_util.h"
#include "bat/ledger/internal/state/state_keys.h"
#include "bat/ledger/internal/uphold/uphold_util.h"
#include "net/http/http_status_code.h"

using std::placeholders::_1;
using std::placeholders::_2;

namespace braveledger_wallet {

WalletClaim::WalletClaim(bat_ledger::LedgerImpl* ledger) :
    ledger_(ledger),
    uphold_(std::make_unique<braveledger_uphold::Uphold>(ledger))  {
}

WalletClaim::~WalletClaim() = default;

void WalletClaim::Start(ledger::ResultCallback callback) {
  ledger_->FetchBalance(std::bind(&WalletClaim::OnBalance,
      this,
      _1,
      _2,
      callback));
}

void WalletClaim::OnBalance(
    const ledger::Result result,
    ledger::BalancePtr balance,
    ledger::ResultCallback callback) {
  if (result != ledger::Result::LEDGER_OK || !balance) {
    BLOG(0, "Anon funds transfer failed");
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  if (ledger_->GetBooleanState(ledger::kStateAnonTransferChecked) &&
      balance->user_funds == "0") {
    BLOG(1, "Second ping with zero balance");
    callback(ledger::Result::LEDGER_OK);
    return;
  }

  auto wallets = ledger_->GetExternalWallets();
  auto wallet_ptr = braveledger_uphold::GetWallet(std::move(wallets));

  if (!wallet_ptr) {
    BLOG(0, "Wallet is null");
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  if (!wallet_ptr->anon_address.empty()) {
    BLOG(1, "Anon address already exists");
    TransferFunds(
        ledger::Result::LEDGER_OK,
        callback);
    return;
  }

  auto anon_callback = std::bind(&WalletClaim::TransferFunds,
      this,
      _1,
      callback);

  uphold_->CreateAnonAddressIfNecessary(anon_callback);
}

void WalletClaim::TransferFunds(
    const ledger::Result result,
    ledger::ResultCallback callback) {
  auto wallets = ledger_->GetExternalWallets();
  auto wallet_ptr = braveledger_uphold::GetWallet(std::move(wallets));
  if (!wallet_ptr) {
    BLOG(0, "Wallet is null");
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  if (result != ledger::Result::LEDGER_OK ||
      wallet_ptr->anon_address.empty()) {
    BLOG(0, "Anon address is missing");
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  const std::string payment_id = braveledger_state::GetPaymentId(ledger_);

  auto url_callback = std::bind(&WalletClaim::OnTransferFunds,
      this,
      _1,
      callback);

  const std::string header_url = base::StringPrintf(
      "post /v3/wallet/brave/%s/claim",
      payment_id.c_str());

  const auto headers = braveledger_request_util::BuildSignHeaders(
      header_url,
      "",
      payment_id,
      braveledger_state::GetRecoverySeed(ledger_));

  const std::string url = braveledger_request_util::GetClaimWalletURL(
      payment_id);

  ledger_->LoadURL(
      url,
      headers,
      "",
      "application/json; charset=utf-8",
      ledger::UrlMethod::POST,
      url_callback);
}

void WalletClaim::OnTransferFunds(
    const ledger::UrlResponse& response,
    ledger::ResultCallback callback) {
  BLOG(6, ledger::UrlResponseToString(__func__, response));

  if (response.status_code == net::HTTP_OK) {
    ledger_->SetBooleanState(ledger::kStateAnonTransferChecked, true);
    callback(ledger::Result::LEDGER_OK);
    return;
  }

  if (response.status_code == net::HTTP_CONFLICT) {
    ledger_->SetBooleanState(ledger::kStateAnonTransferChecked, true);
    callback(ledger::Result::ALREADY_EXISTS);
    return;
  }

  callback(ledger::Result::LEDGER_ERROR);
}

}  // namespace braveledger_wallet
