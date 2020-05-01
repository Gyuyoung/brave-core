/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_PUBLISHER_YOUTUBE_H_
#define BRAVELEDGER_PUBLISHER_YOUTUBE_H_

#include <memory>
#include <string>

#include "bat/ledger/ledger.h"

namespace bat_ledger {
class LedgerImpl;
}

namespace braveledger_publisher {

class YouTube {
 public:
  explicit YouTube(bat_ledger::LedgerImpl* ledger);

  ~YouTube();

  void UpdateMediaDuration(
      const std::string& media_id,
      const std::string& media_key,
      const std::string& url,
      uint64_t duration);

  void SaveMediaVisitYoutubeChannel(
      const std::string& url,
      const std::string& channel_id,
      const std::string& publisher_key,
      const std::string& favicon_url,
      const std::string& title);

  void SaveMediaVisitYoutubeUser(
      const std::string& url,
      const std::string& channel_id,
      const std::string& publisher_key,
      const std::string& media_key);

  void SaveMediaVisitYoutubeWatch(
      const std::string& url);

 private:
  void SavePublisherInfo(
      const uint64_t duration,
      const std::string& media_key,
      const std::string& publisher_url,
      const std::string& publisher_name,
      const ledger::VisitData& visit_data,
      uint64_t window_id,
      const std::string& favicon_url,
      const std::string& channel_id);

  void OnPanelPublisherInfo(
      uint64_t window_id,
      const ledger::VisitData& visit_data,
      const std::string& channel_id,
      const std::string& publisher_key,
      const std::string& title,
      const std::string& favicon_url,
      ledger::Result result,
      ledger::PublisherInfoPtr info);

  void OnMediaPublisherInfo(
      const std::string& media_id,
      const std::string& media_key,
      const uint64_t duration,
      const ledger::VisitData& visit_data,
      const uint64_t window_id,
      ledger::Result result,
      ledger::PublisherInfoPtr publisher_info);

  bat_ledger::LedgerImpl* ledger_;  // NOT OWNED
};

}  // namespace braveledger_publisher

#endif  // BRAVELEDGER_PUBLISHER_YOUTUBE_H_
