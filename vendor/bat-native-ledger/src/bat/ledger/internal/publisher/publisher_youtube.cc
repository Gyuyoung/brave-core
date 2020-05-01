/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/publisher/publisher_youtube.h"

#include <utility>

#include "bat/ledger/internal/ledger_impl.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"
#include "url/gurl.h"

using std::placeholders::_1;
using std::placeholders::_2;

namespace {

bool InitVisitData(
    ledger::VisitData* visit_data,
    const std::string& url) {
  DCHECK(visit_data);

  const GURL gurl(url);
  if (!gurl.is_valid()) {
    return false;
  }

  const GURL origin = gurl.GetOrigin();

  const std::string base_domain = GetDomainAndRegistry(
      origin.host(),
      net::registry_controlled_domains::INCLUDE_PRIVATE_REGISTRIES);
  if (base_domain.empty()) {
    return false;
  }

  visit_data->domain = visit_data->name = base_domain;
  visit_data->path = gurl.PathForRequest();
  visit_data->url = origin.spec();

  return true;
}

}  // namespace

namespace braveledger_publisher {

const char kProviderType[] = "youtube";

YouTube::YouTube(bat_ledger::LedgerImpl* ledger) : ledger_(ledger) {}

YouTube::~YouTube() {}

void YouTube::UpdateMediaDuration(
    const std::string& media_id,
    const std::string& media_key,
    const std::string& url,
    uint64_t duration) {
  BLOG(1, "Media key: " << media_key);
  BLOG(1, "Media duration: " << duration);

  ledger::VisitData visit_data;
  visit_data.url = url;

  ledger_->GetMediaPublisherInfo(
      media_key,
      std::bind(&YouTube::OnMediaPublisherInfo,
                this,
                media_id,
                media_key,
                duration,
                visit_data,
                0,
                _1,
                _2));
}

void YouTube::SaveMediaVisitYoutubeChannel(
    const std::string& url,
    const std::string& channel_id,
    const std::string& publisher_key,
    const std::string& favicon_url,
    const std::string& title) {
  ledger::VisitData visit_data;
  if (!InitVisitData(&visit_data, url)) {
    return;
  }

  visit_data.favicon_url = favicon_url;

  auto filter = ledger_->CreateActivityFilter(
      publisher_key,
      ledger::ExcludeFilter::FILTER_ALL,
      false,
      ledger_->GetReconcileStamp(),
      true,
      false);

  ledger_->GetPanelPublisherInfo(std::move(filter),
      std::bind(&YouTube::OnPanelPublisherInfo,
                this,
                1,  // window_id
                visit_data,
                channel_id,
                publisher_key,
                title,
                favicon_url,
                _1,
                _2));
}

void YouTube::SaveMediaVisitYoutubeUser(
    const std::string& url,
    const std::string& channel_id,
    const std::string& publisher_key,
    const std::string& media_key) {
  ledger::VisitData visit_data;
  if (!InitVisitData(&visit_data, url)) {
    return;
  }

#if 0
  ledger_->GetMediaPublisherInfo(
      media_key,
      std::bind(&YouTube::OnUserActivity,
          this,
          window_id,
          visit_data,
          media_key,
          _1,
          _2));
#endif
}

void YouTube::SaveMediaVisitYoutubeWatch(
    const std::string& url) {
}

void YouTube::SavePublisherInfo(
    const uint64_t duration,
    const std::string& media_key,
    const std::string& publisher_url,
    const std::string& publisher_name,
    const ledger::VisitData& visit_data,
    uint64_t window_id,
    const std::string& favicon_url,
    const std::string& channel_id) {
  if (channel_id.empty()) {
    BLOG(0, "Channel id is missing for: " << media_key);
    return;
  }

  const std::string publisher_id;  // FIXME! = GetPublisherKey(channel_id);
  if (publisher_id.empty()) {
    BLOG(0, "Publisher id is missing for: " << media_key);
    return;
  }

  ledger::VisitData new_visit_data;
  new_visit_data.provider = kProviderType;
  new_visit_data.name = publisher_name;
  new_visit_data.url = publisher_url + "/videos";
  if (!favicon_url.empty()) {
    new_visit_data.favicon_url = favicon_url;
  }

  ledger_->SaveMediaVisit(
      publisher_id,
      new_visit_data,
      duration,
      window_id,
      [](ledger::Result, ledger::PublisherInfoPtr){});
  if (!media_key.empty()) {
    ledger_->SaveMediaPublisherInfo(
        media_key,
        publisher_id,
        [](const ledger::Result _){});
  }
}

void YouTube::OnPanelPublisherInfo(
    uint64_t window_id,
    const ledger::VisitData& visit_data,
    const std::string& channel_id,
    const std::string& publisher_key,
    const std::string& title,
    const std::string& favicon_url,
    ledger::Result result,
    ledger::PublisherInfoPtr info) {
  if (info && result != ledger::Result::NOT_FOUND) {
    ledger_->OnPanelPublisherInfo(result, std::move(info), window_id);
    return;
  }

  SavePublisherInfo(
      0,
      std::string(),
      visit_data.url,
      title,
      visit_data,
      window_id,
      favicon_url,
      channel_id);
}

void YouTube::OnMediaPublisherInfo(
    const std::string& media_id,
    const std::string& media_key,
    const uint64_t duration,
    const ledger::VisitData& visit_data,
    const uint64_t window_id,
    ledger::Result result,
    ledger::PublisherInfoPtr publisher_info) {
  if (result != ledger::Result::LEDGER_OK &&
      result != ledger::Result::NOT_FOUND) {
    BLOG(0, "Failed to get publisher info");
    return;
  }

  if (!publisher_info) {
#if 0
    const std::string media_url = GetVideoUrl(media_id);
    auto callback = std::bind(
        &YouTube::OnEmbedResponse,
        this,
        duration,
        media_key,
        media_url,
        visit_data,
        window_id,
        _1,
        _2,
        _3);

    const std::string url = std::string(YOUTUBE_PROVIDER_URL) +
        "?format=json&url=" +
        ledger_->URIEncode(media_url);

    FetchDataFromUrl(url, callback);
#endif
  } else {
    ledger::VisitData new_visit_data;
    new_visit_data.name = publisher_info->name;
    new_visit_data.url = publisher_info->url;
    new_visit_data.provider = kProviderType;
    new_visit_data.favicon_url = publisher_info->favicon_url;
    const std::string id = publisher_info->id;

    ledger_->SaveMediaVisit(
        id,
        new_visit_data,
        duration,
        window_id,
        [](ledger::Result, ledger::PublisherInfoPtr){});
  }
}

}  // namespace braveledger_publisher
