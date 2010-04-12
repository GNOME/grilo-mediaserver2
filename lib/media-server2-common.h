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

#ifndef _MEDIA_SERVER2_COMMON_H_
#define _MEDIA_SERVER2_COMMON_H_

#define MS2_ERROR                                       \
  g_quark_from_static_string("media_server2_error")

/* Common properties */
#define MS2_PROP_ID           "id"
#define MS2_PROP_PARENT       "parent"
#define MS2_PROP_DISPLAY_NAME "display-name"
#define MS2_PROP_TYPE         "type"

/* Container properties */
#define MS2_PROP_CHILD_COUNT "child-count"
#define MS2_PROP_ICON        "icon"

/* Item properties */
#define MS2_PROP_URLS         "URLs"
#define MS2_PROP_MIME_TYPE    "mime-type"
#define MS2_PROP_SIZE         "size"
#define MS2_PROP_ARTIST       "artist"
#define MS2_PROP_ALBUM        "album"
#define MS2_PROP_DATE         "date"
#define MS2_PROP_DLNA_PROFILE "dlna-profile"

/* Audio/Video items properties */
#define MS2_PROP_DURATION        "duration"
#define MS2_PROP_BITRATE         "bitrate"
#define MS2_PROP_SAMPLE_RATE     "sample-rate"
#define MS2_PROP_BITS_PER_SAMPLE "bits-per-sample"

/* Video/Image items properties */
#define MS2_PROP_WIDTH        "width"
#define MS2_PROP_HEIGHT       "height"
#define MS2_PROP_COLOR_DEPTH  "depth"
#define MS2_PROP_PIXEL_WIDTH  "pixel-width"
#define MS2_PROP_PIXEL_HEIGHT "pixel-height"
#define MS2_PROP_THUMBNAIL    "thumbnail"

/* Audio items properties */
#define MS2_PROP_GENRE "genre"

/* Type items */
#define MS2_TYPE_CONTAINER "container"
#define MS2_TYPE_VIDEO     "video"
#define MS2_TYPE_MOVIE     "video.movie"
#define MS2_TYPE_AUDIO     "audio"
#define MS2_TYPE_MUSIC     "audio.music"
#define MS2_TYPE_IMAGE     "image"
#define MS2_TYPE_PHOTO     "image.photo"

/* Unknown values */
#define MS2_UNKNOWN_INT -1
#define MS2_UNKNOWN_STR ""

/* Root category */
#define MS2_ROOT "0"

typedef enum {
  MS2_ERROR_GENERAL = 1
} MS2Error;

#endif /* _MEDIA_SERVER2_COMMON_H_ */

