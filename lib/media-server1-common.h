/*
 * Copyright (C) 2010 Igalia S.L.
 *
 * Authors: Juan A. Suarez Romero <jasuarez@igalia.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 *
 */

#ifndef _MEDIA_SERVER1_COMMON_H_
#define _MEDIA_SERVER1_COMMON_H_

#define MS1_ERROR                                       \
  g_quark_from_static_string("media_server1_error")

/* MediaObject1 properties */
#define MS1_PROP_DISPLAY_NAME "DisplayName"
#define MS1_PROP_PARENT       "Parent"
#define MS1_PROP_PATH         "Path"

/* MediaItem1 properties */
#define MS1_PROP_ALBUM     "Album"
#define MS1_PROP_ARTIST    "Artist"
#define MS1_PROP_BITRATE   "Bitrate"
#define MS1_PROP_DURATION  "Duration"
#define MS1_PROP_GENRE     "Genre"
#define MS1_PROP_HEIGHT    "Height"
#define MS1_PROP_MIME_TYPE "MIMEType"
#define MS1_PROP_TYPE      "Type"
#define MS1_PROP_URLS      "URLs"
#define MS1_PROP_WIDTH     "Width"

/* Other undefined properties; pending to add to spec */
#define MS1_PROP_CHILD_COUNT     "child-count"
#define MS1_PROP_ICON            "icon"
#define MS1_PROP_SIZE            "size"
#define MS1_PROP_DATE            "date"
#define MS1_PROP_DLNA_PROFILE    "dlna-profile"
#define MS1_PROP_SAMPLE_RATE     "sample-rate"
#define MS1_PROP_BITS_PER_SAMPLE "bits-per-sample"
#define MS1_PROP_COLOR_DEPTH     "color-depth"
#define MS1_PROP_PIXEL_WIDTH     "pixel-width"
#define MS1_PROP_PIXEL_HEIGHT    "pixel-height"
#define MS1_PROP_THUMBNAIL       "thumbnail"

/* Type items */
#define MS1_TYPE_CONTAINER "container"
#define MS1_TYPE_VIDEO     "video"
#define MS1_TYPE_MOVIE     "video.movie"
#define MS1_TYPE_AUDIO     "audio"
#define MS1_TYPE_MUSIC     "audio.music"
#define MS1_TYPE_IMAGE     "image"
#define MS1_TYPE_PHOTO     "image.photo"

/* Unknown values */
#define MS1_UNKNOWN_INT -1
#define MS1_UNKNOWN_STR ""

/* Root category */
#define MS1_ROOT ""

typedef enum {
  MS1_ERROR_GENERAL = 1
} MS1Error;

/* Type items definition */
typedef enum {
  MS1_ITEM_TYPE_UNKNOWN = 0,
  MS1_ITEM_TYPE_CONTAINER,
  MS1_ITEM_TYPE_VIDEO,
  MS1_ITEM_TYPE_MOVIE,
  MS1_ITEM_TYPE_AUDIO,
  MS1_ITEM_TYPE_MUSIC,
  MS1_ITEM_TYPE_IMAGE,
  MS1_ITEM_TYPE_PHOTO
} MS1ItemType;

#endif /* _MEDIA_SERVER1_COMMON_H_ */

