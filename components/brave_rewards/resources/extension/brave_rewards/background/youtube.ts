/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

const mediaType = 'youtube'

interface YoutubeWatchPathData {
  url: string
}

interface YoutubeUserPathData {
  url: string
  channelId: string
  publisherKey: string
  mediaKey: string
}

interface YoutubeChannelPathData {
  url: string
  channelId: string
  publisherKey: string
  favIconUrl: string
  title: string
}

export interface PublisherInfoYoutube {
  pathType: 'watch' | 'channel' | 'user',
  data: YoutubeWatchPathData | YoutubeUserPathData | YoutubeChannelPathData
}

const extractData = (data: string, matchAfter: string, matchUntil: string) => {
  if (data.length < matchAfter.length) {
    return ''
  }

  const matchPos = data.indexOf(matchAfter)
  if (matchPos === -1) {
    return ''
  }

  const startPos = matchPos + matchAfter.length
  const endPos = data.indexOf(matchUntil, startPos)

  let match = ''

  if (endPos !== startPos) {
    if (endPos !== -1 && endPos > startPos) {
      match = data.substring(startPos, endPos)
    } else if (endPos !== -1) {
      match = data.substring(startPos, endPos)
    } else {
      match = data.substring(startPos)
    }
  } else if (matchUntil === '') {
    match = data.substring(startPos)
  }

  return match
}

const getChannelUrl = (publisherKey: string) => {
  return `https://www.youtube.com/channel/${publisherKey}`
}

const getMediaIdFromUrl = (url: URL) => {
  const searchParams = new URLSearchParams(url.search)
  if (!searchParams) {
    return ''
  }
  return searchParams.get('v')
}

const getVideoUrl = (mediaId: string) => {
  return `https://www.youtube.com/watch?v=${mediaId}`
}

const getPublisherName = (data: string) => {
  const publisherNameJson = extractData(data, '"author":"', '"')
  if (!publisherNameJson) {
    return ''
  }

  let object = null
  try {
    object = JSON.parse(`{"brave_publisher":"${publisherNameJson}"}`)
  } catch (e) {
    console.error('Error parsing publisher name from response: ', e)
    return
  }

  return object.brave_publisher
}

const getFavIconUrl = (data: string) => {
  let match = extractData(data, '"avatar":{"thumbnails":[{"url":"', '"')
  if (match) {
    return match
  }
  match = extractData(data, '"width":88,"height":88},{"url":"', '"')
  if (match) {
    return match
  }
  return ''
}

const getChannelId = (data: string) => {
  let match = extractData(data, '"ucid":"', '"')
  if (match) {
    return match
  }
  match = extractData(data, 'HeaderRenderer":{"channelId":"', '"')
  if (match) {
    return match
  }
  match = extractData(data, '<link rel="canonical" href="https://www.youtube.com/channel/', '">')
  if (match) {
    return match
  }
  match = extractData(data, 'browseEndpoint":{"browseId":"', '"')
  if (match) {
    return match
  }
  return ''
}

const handleYoutubeChannelPathData = (data: YoutubeChannelPathData) => {
  console.info('Visited a channel url:')
  console.info(`  url=${data.url}`)
  console.info(`  channelId=${data.channelId}`)
  console.info(`  publisherKey=${data.publisherKey}`)
  console.info(`  favIconUrl=${data.favIconUrl}`)
  console.info(`  title=${data.title}`)
  chrome.braveRewards.saveMediaVisitYoutubeChannel(data.url, data.channelId, data.publisherKey, data.favIconUrl, data.title)
}

const handleYoutubeUserPathData = (data: YoutubeUserPathData) => {
  console.info('Visited a user url:')
  console.info(`  url=${data.url}`)
  console.info(`  channelId=${data.channelId}`)
  console.info(`  publisherKey=${data.publisherKey}`)
  console.info(`  mediaKey=${data.mediaKey}`)
  chrome.braveRewards.saveMediaVisitYoutubeUser(data.url, data.channelId, data.publisherKey, data.mediaKey)
}

const handleYoutubeWatchPathData = (data: YoutubeWatchPathData) => {
  const mediaId = getMediaIdFromUrl(new URL(data.url))
  if (!mediaId) {
    return
  }

  // const mediaKey = `${mediaType}_${mediaId}`

  const videoUrl = getVideoUrl(mediaId)
  const encodedVideoUrl = encodeURI(videoUrl)

  const fetchData: any = {}

  fetch(`https://www.youtube.com/oembed?format=json&url=${encodedVideoUrl}`)
    .then((response) => {
      if (!response.ok) {
        if (response.status === 401) {
          // FIXME: No authorization; need to scrape data from page instead
        } else {
          throw new Error(`YouTube oembed request failed: ${response.statusText} (${response.status})`)
        }
      }
      return response.json()
    })
    .then((responseJson) => {
      fetchData.publisherUrl = responseJson.author_url
      fetchData.publisherName = responseJson.author_name
      return fetch(fetchData.publisherUrl)
    })
    .then((response) => {
      if (!response.ok) {
        throw new Error(`YouTube publisher request failed: ${response.statusText} (${response.status})`)
      }
      return response.text()
    })
    .then((responseText) => {
      const favIconUrl = getFavIconUrl(responseText)
      const channelId = getChannelId(responseText)

      let publisherName = fetchData.publisherName
      if (!publisherName) {
        publisherName = getPublisherName(responseText)
      }

      let publisherUrl = fetchData.publisherUrl
      if (!publisherUrl) {
        publisherUrl = getChannelUrl(channelId)
      }

      console.info('Visited a watch url:')
      console.info(`  url=${data.url}`)
      console.info(`  favIconUrl=${favIconUrl}`)
      console.info(`  channelId=${channelId}`)
      console.info(`  publisherName=${publisherName}`)
      console.info(`  publisherUrl=${publisherUrl}`)
    })
    .catch((error) => {
      throw new Error(`YouTube fetch request failed: ${error}`)
    })

  chrome.braveRewards.saveMediaVisitYoutubeWatch(data.url)
}

export const handlePublisherInfoYoutube = (publisherInfo: PublisherInfoYoutube) => {
  switch (publisherInfo.pathType) {
    case 'channel': {
      const data = publisherInfo.data as YoutubeChannelPathData
      handleYoutubeChannelPathData(data)
      break
    }
    case 'user': {
      const data = publisherInfo.data as YoutubeUserPathData
      handleYoutubeUserPathData(data)
      break
    }
    case 'watch': {
      const data = publisherInfo.data as YoutubeWatchPathData
      handleYoutubeWatchPathData(data)
      break
    }
  }
}

const getMediaDurationFromParts = (searchParams: URLSearchParams) => {
  const stParam = searchParams.get('st')
  const etParam = searchParams.get('et')
  if (!stParam || !etParam) {
    return 0
  }

  const startTimes = stParam.split(',')
  if (!startTimes || startTimes.length === 0) {
    return 0
  }

  const endTimes = etParam.split(',')
  if (!endTimes || endTimes.length === 0) {
    return 0
  }

  if (startTimes.length !== endTimes.length) {
    return 0
  }

  // Combine all of the intervals (should only be one set if there were no seeks)
  let duration = 0
  for (let i = 0; i < startTimes.length; ++i) {
    const st = parseFloat(startTimes[i])
    const et = parseFloat(endTimes[i])
    duration += Math.round(et - st)
  }

  return duration
}

const getMediaIdFromParts = (searchParams: URLSearchParams) => {
  return searchParams.get('docid') || ''
}

const getMediaKey = (mediaId: string) => {
  return `${mediaType}_${mediaId}`
}

chrome.webRequest.onCompleted.addListener(
  // Listener
  function (details) {
    if (details) {
      const url = new URL(details.url)
      const searchParams = new URLSearchParams(url.search)

      const mediaId = getMediaIdFromParts(searchParams)
      console.info(`Media id is: ${mediaId}`)
      const mediaKey = getMediaKey(mediaId)
      console.info(`Media key is: ${mediaKey}`)
      const duration = getMediaDurationFromParts(searchParams)
      console.info(`Media duration is: ${duration}`)

      chrome.braveRewards.updateMediaDuration(mediaType, mediaId, mediaKey, details.url, duration)
    }
  },
  // Filters
  {
    types: [
      'image',
      'media',
      'script',
      'xmlhttprequest'
    ],
    urls: [
      'https://www.youtube.com/api/stats/watchtime?*'
    ]
  })
