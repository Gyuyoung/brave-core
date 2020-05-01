/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { PublisherInfoYoutube, handlePublisherInfoYoutube } from './youtube'

interface PublisherInfoResponse {
  mediaType: 'youtube',
  errorMessage?: string,
  publisherInfo: PublisherInfoYoutube | null
}

const handlePublisherInfoResponse = (response: PublisherInfoResponse) => {
  if (!response.publisherInfo) {
    console.error(`Failed to retrieve publisher info: ${response.errorMessage}`)
    return
  }

  switch (response.mediaType) {
    case 'youtube':
      handlePublisherInfoYoutube(response.publisherInfo)
      break
    default:
      console.error(`Unsupported media type in publisher info response: ${response.mediaType}`)
      break
  }
}

chrome.runtime.onMessageExternal.addListener((msg, sender, sendResponse) => {
  handlePublisherInfoResponse(msg)
})
