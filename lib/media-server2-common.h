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

/* MediaObject1 properties */
#define MS2_PROP_PARENT       "Parent"
#define MS2_PROP_TYPE         "Type"
#define MS2_PROP_PATH         "Path"
#define MS2_PROP_DISPLAY_NAME "DisplayName"

/* MediaItem1 properties */
#define MS2_PROP_URLS            "URLs"
#define MS2_PROP_MIME_TYPE       "MIMEType"
#define MS2_PROP_SIZE            "Size"
#define MS2_PROP_ARTIST          "Artist"
#define MS2_PROP_ALBUM           "Album"
#define MS2_PROP_DATE            "Date"
#define MS2_PROP_GENRE           "Genre"
#define MS2_PROP_DLNA_PROFILE    "DLNAProfile"
#define MS2_PROP_DURATION        "Duration"
#define MS2_PROP_BITRATE         "Bitrate"
#define MS2_PROP_SAMPLE_RATE     "SampleRate"
#define MS2_PROP_BITS_PER_SAMPLE "BitsPerSample"
#define MS2_PROP_WIDTH           "Width"
#define MS2_PROP_HEIGHT          "Height"
#define MS2_PROP_COLOR_DEPTH     "ColorDepth"
#define MS2_PROP_PIXEL_WIDTH     "PixelWidth"
#define MS2_PROP_PIXEL_HEIGHT    "PixelHeight"
#define MS2_PROP_THUMBNAIL       "Thumbnail"
#define MS2_PROP_ALBUM_ART       "AlbumArt"

/* MediaContainer1 properties */
#define MS2_PROP_CHILD_COUNT     "ChildCount"
#define MS2_PROP_ITEMS           "Items"
#define MS2_PROP_ITEM_COUNT      "ItemCount"
#define MS2_PROP_CONTAINERS      "Containers"
#define MS2_PROP_CONTAINER_COUNT "ContainerCount"
#define MS2_PROP_SEARCHABLE      "Searchable"

/* Type items */
#define MS2_TYPE_CONTAINER "container"
#define MS2_TYPE_ITEM      "item"
#define MS2_TYPE_VIDEO     "video"
#define MS2_TYPE_MOVIE     "video.movie"
#define MS2_TYPE_AUDIO     "audio"
#define MS2_TYPE_MUSIC     "audio.music"
#define MS2_TYPE_IMAGE     "image"
#define MS2_TYPE_PHOTO     "image.photo"

/* Unknown values */
#define MS2_UNKNOWN_INT -1
#define MS2_UNKNOWN_UINT 0
#define MS2_UNKNOWN_STR ""

/* Root category */
#define MS2_ROOT ""

/* Type items definition */
typedef enum {
  MS2_ITEM_TYPE_UNKNOWN = 0,
  MS2_ITEM_TYPE_ITEM,
  MS2_ITEM_TYPE_CONTAINER,
  MS2_ITEM_TYPE_VIDEO,
  MS2_ITEM_TYPE_MOVIE,
  MS2_ITEM_TYPE_AUDIO,
  MS2_ITEM_TYPE_MUSIC,
  MS2_ITEM_TYPE_IMAGE,
  MS2_ITEM_TYPE_PHOTO
} MS2ItemType;

#endif /* _MEDIA_SERVER2_COMMON_H_ */
