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

#include <dbus/dbus-glib-bindings.h>

#include "media-server2-server.h"

#define DBUS_TYPE_G_ARRAY_OF_STRING                             \
  (dbus_g_type_get_collection ("GPtrArray", G_TYPE_STRING))

/******************** PRIVATE API ********************/

/* Free gvalue */
static void
free_value (GValue *value)
{
  g_value_unset (value);
  g_free (value);
}

/* Puts a string in a gvalue */
static GValue *
str_to_value (const gchar *str)
{
  GValue *val = NULL;

  if (str) {
    val = g_new0 (GValue, 1);
    g_value_init (val, G_TYPE_STRING);
    g_value_set_string (val, str);
  }

  return val;
}

/* Puts an int in a gvalue */
static GValue *
int_to_value (gint number)
{
  GValue *val = NULL;

  val = g_new0 (GValue, 1);
  g_value_init (val, G_TYPE_INT);
  g_value_set_int (val, number);

  return val;
}

/* Puts a gptrarray in a gvalue */
static GValue *
ptrarray_to_value (GPtrArray *array)
{
  GValue *val = NULL;

  val = g_new0 (GValue, 1);
  g_value_init (val, DBUS_TYPE_G_ARRAY_OF_STRING);
  g_value_take_boxed (val, array);

  return val;
}

/********************* PUBLIC API *********************/

/**
 * ms2_server_new_properties_hashtable:
 * @id: identifier of item which properties will be stored in the table
 *
 * Creates a new #GHashTable suitable to store items properties.
 *
 * For root container, identifier should be "0".
 *
 * Returns: a new #GHashTable
 **/
GHashTable *
ms2_server_new_properties_hashtable (const gchar *id)
{
  GHashTable *properties;

  properties = g_hash_table_new_full (g_str_hash,
                                      g_str_equal,
                                      NULL,
                                      (GDestroyNotify) free_value);
  if (id) {
    g_hash_table_insert (properties, MS2_PROP_ID, str_to_value (id));
  }

  return properties;
}

/**
 * ms2_server_set_parent:
 * @properties: a #GHashTable
 * @parent: parent value
 *
 * Sets the "parent" property. Mandatory property.
 **/
void
ms2_server_set_parent (GHashTable *properties,
                       const gchar *parent)
{
  g_return_if_fail (properties);

  if (parent) {
    g_hash_table_insert (properties,
                         MS2_PROP_PARENT,
                         str_to_value (parent));
  }
}

/**
 * ms2_server_set_display_name:
 * @properties: a #GHashTable
 * @display_name: display name value
 *
 * Sets the "display-name" property. Mandatory property.
 **/
void
ms2_server_set_display_name (GHashTable *properties,
                             const gchar *display_name)
{
  g_return_if_fail (properties);

  if (display_name) {
    g_hash_table_insert (properties,
                         MS2_PROP_DISPLAY_NAME,
                         str_to_value (display_name));
  }
}

/**
 * ms2_server_set_item_type:
 * @properties: a #GHashTable
 * @type: type of item
 *
 * Sets the "type" property. Mandatory property.
 *
 * Tells what kind of object we are dealing with.
 **/
void
ms2_server_set_item_type (GHashTable *properties,
                          MS2ItemType type)
{
  g_return_if_fail (properties);

  switch (type) {
  case MS2_ITEM_TYPE_UNKNOWN:
    /* Do not handle unknown values */
    break;
  case MS2_ITEM_TYPE_CONTAINER:
    g_hash_table_insert (properties,
                         MS2_PROP_TYPE,
                         str_to_value (MS2_TYPE_CONTAINER));
    break;
  case MS2_ITEM_TYPE_VIDEO:
    g_hash_table_insert (properties,
                         MS2_PROP_TYPE,
                         str_to_value (MS2_TYPE_VIDEO));
    break;
  case MS2_ITEM_TYPE_MOVIE:
    g_hash_table_insert (properties,
                         MS2_PROP_TYPE,
                         str_to_value (MS2_TYPE_MOVIE));
    break;
  case MS2_ITEM_TYPE_AUDIO:
    g_hash_table_insert (properties,
                         MS2_PROP_TYPE,
                         str_to_value (MS2_TYPE_AUDIO));
    break;
  case MS2_ITEM_TYPE_MUSIC:
    g_hash_table_insert (properties,
                         MS2_PROP_TYPE,
                         str_to_value (MS2_TYPE_MUSIC));
    break;
  case MS2_ITEM_TYPE_IMAGE:
    g_hash_table_insert (properties,
                         MS2_PROP_TYPE,
                         str_to_value (MS2_TYPE_IMAGE));
    break;
  case MS2_ITEM_TYPE_PHOTO:
    g_hash_table_insert (properties,
                         MS2_PROP_TYPE,
                         str_to_value (MS2_TYPE_PHOTO));
    break;
  }
}

/**
 * ms2_server_set_icon:
 * @properties: a #GHashTable
 * @icon: icon identifier value
 *
 * Sets the "icon" property. Recommended property for containers.
 *
 * Use this to provide an icon to be used by consumer UIs to represent the
 * provider. This is only relevant to root container.
 **/
void
ms2_server_set_icon (GHashTable *properties,
                     const gchar *icon)
{
  g_return_if_fail (properties);

  if (icon) {
    g_hash_table_insert (properties,
                         MS2_PROP_ICON,
                         str_to_value (icon));
  }
}

/**
 * ms2_server_set_mime_type:
 * @properties: a #GHashTable
 * @mime_type: mime type value
 *
 * Sets the "mime-type" property. Mandatory property for items.
 **/
void
ms2_server_set_mime_type (GHashTable *properties,
                          const gchar *mime_type)
{
  g_return_if_fail (properties);

  if (mime_type) {
    g_hash_table_insert (properties,
                         MS2_PROP_MIME_TYPE,
                         str_to_value (mime_type));
  }
}

/**
 * ms2_server_set_artist:
 * @properties: a #GHashTable
 * @artist: artist value
 *
 * Sets the "artist" property. Recommended property for items.
 **/
void
ms2_server_set_artist (GHashTable *properties,
                       const gchar *artist)
{
  g_return_if_fail (properties);

  if (artist) {
    g_hash_table_insert (properties,
                         MS2_PROP_ARTIST,
                         str_to_value (artist));
  }
}

/**
 * ms2_server_set_album:
 * @properties: a #GHashTable
 * @album: album value
 *
 * Sets the "album" property. Recommended property for items.
 **/
void
ms2_server_set_album (GHashTable *properties,
                      const gchar *album)
{
  g_return_if_fail (properties);

  if (album) {
    g_hash_table_insert (properties,
                         MS2_PROP_ALBUM,
                         str_to_value (album));
  }
}

/**
 * ms2_server_set_date:
 * @properties: a #GHashTable
 * @date: date value
 *
 * Sets the "date" property. Recommended property for items.
 *
 * This date can be date of creation or release. Must be compliant to ISO-8601
 * and RFC-3339.
 **/
void
ms2_server_set_date (GHashTable *properties,
                     const gchar *date)
{
  g_return_if_fail (properties);

  if (date) {
    g_hash_table_insert (properties,
                         MS2_PROP_ALBUM,
                         str_to_value (date));
  }
}

/**
 * ms2_server_set_dlna_profile:
 * @properties: a #GHashTable
 * @dlna_profile: DLNA value
 *
 * Sets the "dlna-profile" property. Optional property for items.
 *
 * If you provide a value for this property, it will greatly help avoiding
 * guessing of its value by UPnP consumers.
 **/
void
ms2_server_set_dlna_profile (GHashTable *properties,
                             const gchar *dlna_profile)
{
  g_return_if_fail (properties);

  if (dlna_profile) {
    g_hash_table_insert (properties,
                         MS2_PROP_DLNA_PROFILE,
                         str_to_value (dlna_profile));
  }
}

/**
 * ms2_server_set_thumbnail:
 * @properties: a #GHashTable
 * @thumbnail: thumbnail identifier value
 *
 * Sets the "thumbnail" property. Optional property for video/image items.
 **/
void
ms2_server_set_thumbnail (GHashTable *properties,
                          const gchar *thumbnail)
{
  g_return_if_fail (properties);

  if (thumbnail) {
    g_hash_table_insert (properties,
                         MS2_PROP_THUMBNAIL,
                         str_to_value (thumbnail));
  }
}

/**
 * ms2_server_set_genre:
 * @properties: a #GHashTable
 * @genre: genre value
 *
 * Sets the "genre" property. Optional property for audio/music items.
 **/
void
ms2_server_set_genre (GHashTable *properties,
                      const gchar *genre)
{
  g_return_if_fail (properties);

  if (genre) {
    g_hash_table_insert (properties,
                         MS2_PROP_GENRE,
                         str_to_value (genre));
  }
}

/**
 * ms2_server_set_child_count:
 * @properties: a #GHashTable
 * @child_count: childcount value
 *
 * Sets the "child-count" property. Recommended property for containers.
 *
 * It is the number of media objects directly under this container.
 **/
void
ms2_server_set_child_count (GHashTable *properties,
                            gint child_count)
{
  g_return_if_fail (properties);

  g_hash_table_insert (properties,
                       MS2_PROP_CHILD_COUNT,
                       int_to_value (child_count));
}

/**
 * ms2_server_set_size:
 * @properties: a #GHashTable
 * @size: size value
 *
 * Sets the "size" property. Recommended property for items.
 *
 * It is the resource size in bytes.
 **/
void
ms2_server_set_size (GHashTable *properties,
                     gint size)
{
  g_return_if_fail (properties);

  g_hash_table_insert (properties,
                       MS2_PROP_SIZE,
                       int_to_value (size));
}

/**
 * ms2_server_set_duration:
 * @properties: a #GHashTable
 * @duration: duration (in seconds) value
 *
 * Sets the "duration" property. Optional property for audio/video/music items.
 **/
void
ms2_server_set_duration (GHashTable *properties,
                         gint duration)
{
  g_return_if_fail (properties);

  g_hash_table_insert (properties,
                       MS2_PROP_DURATION,
                       int_to_value (duration));
}

/**
 * ms2_server_set_bitrate:
 * @properties: a #GHashTable
 * @bitrate: bitrate value
 *
 * Sets the "bitrate" property. Optional property for audio/video/music items.
 **/
void
ms2_server_set_bitrate (GHashTable *properties,
                        gint bitrate)
{
  g_return_if_fail (properties);

  g_hash_table_insert (properties,
                       MS2_PROP_BITRATE,
                       int_to_value (bitrate));
}

/**
 * ms2_server_set_sample_rate:
 * @properties: a #GHashTable
 * @sample_rate: sample rate value
 *
 * Sets the "sample-rate" property. Optional property for audio/video/music
 * items.
 **/
void
ms2_server_set_sample_rate (GHashTable *properties,
                            gint sample_rate)
{
  g_return_if_fail (properties);

  g_hash_table_insert (properties,
                       MS2_PROP_SAMPLE_RATE,
                       int_to_value (sample_rate));
}

/**
 * ms2_server_set_bits_per_sample:
 * @properties: a #GHashTable
 * @bits_per_sample: bits per sample value
 *
 * Sets the "bits-per-sample" property. Optional property for audio/video/music
 * items.
 **/
void
ms2_server_set_bits_per_sample (GHashTable *properties,
                                gint bits_per_sample)
{
  g_return_if_fail (properties);

  g_hash_table_insert (properties,
                       MS2_PROP_BITS_PER_SAMPLE,
                       int_to_value (bits_per_sample));
}

/**
 * ms2_server_set_width:
 * @properties: a #GHashTable
 * @width: width (in pixels) value
 *
 * Sets the "width" property. Recommended property for video/image items.
 **/
void
ms2_server_set_width (GHashTable *properties,
                      gint width)
{
  g_return_if_fail (properties);

  g_hash_table_insert (properties,
                       MS2_PROP_WIDTH,
                       int_to_value (width));
}

/**
 * ms2_server_set_height:
 * @properties: a #GHashTable
 * @height: height (in pixels) value
 *
 * Sets the "height" property. Recommended property for video/image items.
 **/
void
ms2_server_set_height (GHashTable *properties,
                       gint height)
{
  g_return_if_fail (properties);

  g_hash_table_insert (properties,
                       MS2_PROP_HEIGHT,
                       int_to_value (height));
}

/**
 * ms2_server_set_color_depth:
 * @properties: a #GHashTable
 * @depth: color depth value
 *
 * Sets the "color-depth" property. Recommended property for video/image items.
 **/
void
ms2_server_set_color_depth (GHashTable *properties,
                            gint depth)
{
  g_return_if_fail (properties);

  g_hash_table_insert (properties,
                       MS2_PROP_COLOR_DEPTH,
                       int_to_value (depth));
}

/**
 * ms2_server_set_pixel_width:
 * @properties: a #GHashTable
 * @pixel_width: pixel width value
 *
 * Sets the "pixel-width" property. Optional property for video/image items.
 **/
void
ms2_server_set_pixel_width (GHashTable *properties,
                            gint pixel_width)
{
  g_return_if_fail (properties);

  g_hash_table_insert (properties,
                       MS2_PROP_PIXEL_WIDTH,
                       int_to_value (pixel_width));
}

/**
 * ms2_server_set_pixel_height:
 * @properties: a #GHashTable
 * @pixel_height: pixel height value
 *
 * Sets the "pixel-height" property. Optional property for video/image items.
 **/
void
ms2_server_set_pixel_height (GHashTable *properties,
                             gint pixel_height)
{
  g_return_if_fail (properties);

  g_hash_table_insert (properties,
                       MS2_PROP_PIXEL_HEIGHT,
                       int_to_value (pixel_height));
}

/**
 * ms2_server_set_urls:
 * @properties: a #GHashTable
 * @urls: @NULL-terminated array of URLs values
 *
 * Sets the "URLs" property. Mandatory property for items.
 **/
void
ms2_server_set_urls (GHashTable *properties,
                     gchar **urls)
{
  GPtrArray *url_array;
  gint i;

  g_return_if_fail (properties);

  /* Check if there is at least one URL */
  if (urls && urls[0]) {
    url_array = g_ptr_array_sized_new (g_strv_length (urls));
    for (i = 0; urls[i]; i++) {
      g_ptr_array_add (url_array, g_strdup (urls[i]));
    }

    g_hash_table_insert (properties,
                         MS2_PROP_URLS,
                         ptrarray_to_value (url_array));
  }
}
