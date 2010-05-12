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

#include "media-server1-server.h"
#include "media-server1-private.h"

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

/* Puts an uint in a gvalue */
static GValue *
uint_to_value (guint number)
{
  GValue *val = NULL;

  val = g_new0 (GValue, 1);
  g_value_init (val, G_TYPE_UINT);
  g_value_set_uint (val, number);

  return val;
}

/* Puts a boolean in a gvalue */
static GValue *
bool_to_value (gboolean boolean)
{
  GValue *val = NULL;

  val = g_new0 (GValue, 1);
  g_value_init (val, G_TYPE_BOOLEAN);
  g_value_set_boolean (val, boolean);

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

/* Returns an object path from an id */
static gchar *
id_to_object_path (MS1Server *server,
                   const gchar *id,
                   gboolean is_container)
{
  gchar *object_path;

  /* Root container */
  if (g_strcmp0 (id, MS1_ROOT) == 0) {
    object_path = g_strconcat (MS1_DBUS_PATH_PREFIX,
                               ms1_server_get_name (server),
                               NULL);
  } else {
    if (is_container) {
      object_path = g_strdup_printf (MS1_DBUS_PATH_PREFIX "%s/containers/%d",
                                     ms1_server_get_name (server),
                                     g_quark_from_string (id));
    } else {
      object_path = g_strdup_printf (MS1_DBUS_PATH_PREFIX "%s/items/%d",
                                     ms1_server_get_name (server),
                                     g_quark_from_string (id));
    }
  }

  return object_path;
}

/* Returns a GPtrArray of object_paths obtained from list of hashtable
   properties */
static GPtrArray *
get_object_paths (GList *items)
{
  GList *item;
  GPtrArray *op;
  gchar *path;

  op = g_ptr_array_sized_new (g_list_length (items));
  for (item = items; item; item = g_list_next (item)) {
    path = g_strdup (ms1_client_get_path (item->data));
    if (path) {
      g_ptr_array_add (op, path);
    }
  }

  return op;
}

/********************* PUBLIC API *********************/

/**
 * ms1_server_new_properties_hashtable:
 *
 * Creates a new #GHashTable suitable to store items properties.
 *
 * Returns: a new #GHashTable
 **/
GHashTable *
ms1_server_new_properties_hashtable ()
{
  GHashTable *properties;

  properties = g_hash_table_new_full (g_str_hash,
                                      g_str_equal,
                                      NULL,
                                      (GDestroyNotify) free_value);

  return properties;
}

/**
 * ms1_server_set_path:
 * @server: a #MS1Server
 * @properties: a #GHashTable
 * @id: identifier value
 * @is_container: @TRUE if the @id identifies a container
 *
 * Sets the "Path" property.
 *
 * @id will be transformed in an object path
 **/
void
ms1_server_set_path (MS1Server *server,
                     GHashTable *properties,
                     const gchar *id,
                     gboolean is_container)
{
  gchar *object_path;

  g_return_if_fail (MS1_IS_SERVER (server));
  g_return_if_fail (properties);

  if (id) {
    object_path = id_to_object_path (server, id, is_container);
    g_hash_table_insert (properties, MS1_PROP_PATH, str_to_value (object_path));
    g_free (object_path);
  }
}

/**
 * ms1_server_set_parent:
 * @server: a #MS1Server
 * @properties: a #GHashTable
 * @parent: parent value
 *
 * Sets the "Parent" property.
 *
 * @parent will be transformed in an object path.
 **/
void
ms1_server_set_parent (MS1Server *server,
                       GHashTable *properties,
                       const gchar *parent)
{
  gchar *object_path;

  g_return_if_fail (MS1_IS_SERVER (server));
  g_return_if_fail (properties);

  if (parent) {
    object_path = id_to_object_path (server, parent, TRUE);
    g_hash_table_insert (properties,
                         MS1_PROP_PARENT,
                         str_to_value (object_path));
    g_free (object_path);
  }
}

/**
 * ms1_server_set_display_name:
 * @server: a #MS1Server
 * @properties: a #GHashTable
 * @display_name: display name value
 *
 * Sets the "DisplayName" property.
 **/
void
ms1_server_set_display_name (MS1Server *server,
                             GHashTable *properties,
                             const gchar *display_name)
{
  g_return_if_fail (properties);

  if (display_name) {
    g_hash_table_insert (properties,
                         MS1_PROP_DISPLAY_NAME,
                         str_to_value (display_name));
  }
}

/**
 * ms1_server_set_item_type:
 * @server: a #MS1Server
 * @properties: a #GHashTable
 * @type: type of item
 *
 * Sets the "Type" property.
 *
 * Tells what kind of object we are dealing with.
 **/
void
ms1_server_set_item_type (MS1Server *server,
                          GHashTable *properties,
                          MS1ItemType type)
{
  g_return_if_fail (properties);

  switch (type) {
  case MS1_ITEM_TYPE_UNKNOWN:
    /* Do not handle unknown values */
    break;
  case MS1_ITEM_TYPE_CONTAINER:
    g_hash_table_insert (properties,
                         MS1_PROP_TYPE,
                         str_to_value (MS1_TYPE_CONTAINER));
    break;
  case MS1_ITEM_TYPE_ITEM:
    g_hash_table_insert (properties,
                         MS1_PROP_TYPE,
                         str_to_value (MS1_TYPE_ITEM));
    break;
  case MS1_ITEM_TYPE_VIDEO:
    g_hash_table_insert (properties,
                         MS1_PROP_TYPE,
                         str_to_value (MS1_TYPE_VIDEO));
    break;
  case MS1_ITEM_TYPE_MOVIE:
    g_hash_table_insert (properties,
                         MS1_PROP_TYPE,
                         str_to_value (MS1_TYPE_MOVIE));
    break;
  case MS1_ITEM_TYPE_AUDIO:
    g_hash_table_insert (properties,
                         MS1_PROP_TYPE,
                         str_to_value (MS1_TYPE_AUDIO));
    break;
  case MS1_ITEM_TYPE_MUSIC:
    g_hash_table_insert (properties,
                         MS1_PROP_TYPE,
                         str_to_value (MS1_TYPE_MUSIC));
    break;
  case MS1_ITEM_TYPE_IMAGE:
    g_hash_table_insert (properties,
                         MS1_PROP_TYPE,
                         str_to_value (MS1_TYPE_IMAGE));
    break;
  case MS1_ITEM_TYPE_PHOTO:
    g_hash_table_insert (properties,
                         MS1_PROP_TYPE,
                         str_to_value (MS1_TYPE_PHOTO));
    break;
  }
}

/**
 * ms1_server_set_mime_type:
 * @server: a #MS1Server
 * @properties: a #GHashTable
 * @mime_type: mime type value
 *
 * Sets the "MIMEType" property.
 **/
void
ms1_server_set_mime_type (MS1Server *server,
                          GHashTable *properties,
                          const gchar *mime_type)
{
  g_return_if_fail (properties);

  if (mime_type) {
    g_hash_table_insert (properties,
                         MS1_PROP_MIME_TYPE,
                         str_to_value (mime_type));
  }
}

/**
 * ms1_server_set_artist:
 * @server: a #MS1Server
 * @properties: a #GHashTable
 * @artist: artist value
 *
 * Sets the "Artist" property.
 **/
void
ms1_server_set_artist (MS1Server *server,
                       GHashTable *properties,
                       const gchar *artist)
{
  g_return_if_fail (properties);

  if (artist) {
    g_hash_table_insert (properties,
                         MS1_PROP_ARTIST,
                         str_to_value (artist));
  }
}

/**
 * ms1_server_set_album:
 * @server: a #MS1Server
 * @properties: a #GHashTable
 * @album: album value
 *
 * Sets the "Album" property.
 **/
void
ms1_server_set_album (MS1Server *server,
                      GHashTable *properties,
                      const gchar *album)
{
  g_return_if_fail (properties);

  if (album) {
    g_hash_table_insert (properties,
                         MS1_PROP_ALBUM,
                         str_to_value (album));
  }
}

/**
 * ms1_server_set_date:
 * @server: a #MS1Server
 * @properties: a #GHashTable
 * @date: date value
 *
 * Sets the "Date" property.
 *
 * This date can be date of creation or release. Must be compliant to ISO-8601
 * and RFC-3339.
 **/
void
ms1_server_set_date (MS1Server *server,
                     GHashTable *properties,
                     const gchar *date)
{
  g_return_if_fail (properties);

  if (date) {
    g_hash_table_insert (properties,
                         MS1_PROP_ALBUM,
                         str_to_value (date));
  }
}

/**
 * ms1_server_set_dlna_profile:
 * @server: a #MS1Server
 * @properties: a #GHashTable
 * @dlna_profile: DLNA value
 *
 * Sets the "DLNAProfile" property.
 *
 * If you provide a value for this property, it will greatly help avoiding
 * guessing of its value by UPnP consumers.
 **/
void
ms1_server_set_dlna_profile (MS1Server *server,
                             GHashTable *properties,
                             const gchar *dlna_profile)
{
  g_return_if_fail (properties);

  if (dlna_profile) {
    g_hash_table_insert (properties,
                         MS1_PROP_DLNA_PROFILE,
                         str_to_value (dlna_profile));
  }
}

/**
 * ms1_server_set_thumbnail:
 * @server: a #MS1Server
 * @properties: a #GHashTable
 * @thumbnail: thumbnail identifier value
 *
 * Sets the "Thumbnail" property.
 **/
void
ms1_server_set_thumbnail (MS1Server *server,
                          GHashTable *properties,
                          const gchar *thumbnail)
{
  g_return_if_fail (properties);

  if (thumbnail) {
    g_hash_table_insert (properties,
                         MS1_PROP_THUMBNAIL,
                         str_to_value (thumbnail));
  }
}

/**
 * ms1_server_set_album_art
 * @server: a #MS1Server
 * @properties: a #GHashTable
 * @album_art: albumart identifier value
 *
 * Sets the "AlbumArt" property.
 **/
void
ms1_server_set_album_art (MS1Server *server,
                          GHashTable *properties,
                          const gchar *album_art)
{
  g_return_if_fail (properties);

  if (album_art) {
    g_hash_table_insert (properties,
                         MS1_PROP_ALBUM_ART,
                         str_to_value (album_art));
  }
}

/**
 * ms1_server_set_genre:
 * @server: a #MS1Server
 * @properties: a #GHashTable
 * @genre: genre value
 *
 * Sets the "Genre" property. Optional property for audio/music items.
 **/
void
ms1_server_set_genre (MS1Server *server,
                      GHashTable *properties,
                      const gchar *genre)
{
  g_return_if_fail (properties);

  if (genre) {
    g_hash_table_insert (properties,
                         MS1_PROP_GENRE,
                         str_to_value (genre));
  }
}

/**
 * ms1_server_set_size:
 * @server: a #MS1Server
 * @properties: a #GHashTable
 * @size: size value
 *
 * Sets the "Size" property.
 *
 * It is the resource size in bytes.
 **/
void
ms1_server_set_size (MS1Server *server,
                     GHashTable *properties,
                     gint size)
{
  g_return_if_fail (properties);

  g_hash_table_insert (properties,
                       MS1_PROP_SIZE,
                       int_to_value (size));
}

/**
 * ms1_server_set_duration:
 * @server: a #MS1Server
 * @properties: a #GHashTable
 * @duration: duration (in seconds) value
 *
 * Sets the "Duration" property.
 **/
void
ms1_server_set_duration (MS1Server *server,
                         GHashTable *properties,
                         gint duration)
{
  g_return_if_fail (properties);

  g_hash_table_insert (properties,
                       MS1_PROP_DURATION,
                       int_to_value (duration));
}

/**
 * ms1_server_set_bitrate:
 * @server: a #MS1Server
 * @properties: a #GHashTable
 * @bitrate: bitrate value
 *
 * Sets the "Bitrate" property.
 **/
void
ms1_server_set_bitrate (MS1Server *server,
                        GHashTable *properties,
                        gint bitrate)
{
  g_return_if_fail (properties);

  g_hash_table_insert (properties,
                       MS1_PROP_BITRATE,
                       int_to_value (bitrate));
}

/**
 * ms1_server_set_sample_rate:
 * @server: a #MS1Server
 * @properties: a #GHashTable
 * @sample_rate: sample rate value
 *
 * Sets the "SampleRate" property.
 * items.
 **/
void
ms1_server_set_sample_rate (MS1Server *server,
                            GHashTable *properties,
                            gint sample_rate)
{
  g_return_if_fail (properties);

  g_hash_table_insert (properties,
                       MS1_PROP_SAMPLE_RATE,
                       int_to_value (sample_rate));
}

/**
 * ms1_server_set_bits_per_sample:
 * @server: a #MS1Server
 * @properties: a #GHashTable
 * @bits_per_sample: bits per sample value
 *
 * Sets the "BitsPerSample" property.
 * items.
 **/
void
ms1_server_set_bits_per_sample (MS1Server *server,
                                GHashTable *properties,
                                gint bits_per_sample)
{
  g_return_if_fail (properties);

  g_hash_table_insert (properties,
                       MS1_PROP_BITS_PER_SAMPLE,
                       int_to_value (bits_per_sample));
}

/**
 * ms1_server_set_width:
 * @server: a #MS1Server
 * @properties: a #GHashTable
 * @width: width (in pixels) value
 *
 * Sets the "Width" property.
 **/
void
ms1_server_set_width (MS1Server *server,
                      GHashTable *properties,
                      gint width)
{
  g_return_if_fail (properties);

  g_hash_table_insert (properties,
                       MS1_PROP_WIDTH,
                       int_to_value (width));
}

/**
 * ms1_server_set_height:
 * @server: a #MS1Server
 * @properties: a #GHashTable
 * @height: height (in pixels) value
 *
 * Sets the "Height" property.
 **/
void
ms1_server_set_height (MS1Server *server,
                       GHashTable *properties,
                       gint height)
{
  g_return_if_fail (properties);

  g_hash_table_insert (properties,
                       MS1_PROP_HEIGHT,
                       int_to_value (height));
}

/**
 * ms1_server_set_color_depth:
 * @server: a #MS1Server
 * @properties: a #GHashTable
 * @depth: color depth value
 *
 * Sets the "ColorDepth" property.
 **/
void
ms1_server_set_color_depth (MS1Server *server,
                            GHashTable *properties,
                            gint depth)
{
  g_return_if_fail (properties);

  g_hash_table_insert (properties,
                       MS1_PROP_COLOR_DEPTH,
                       int_to_value (depth));
}

/**
 * ms1_server_set_pixel_width:
 * @server: a #MS1Server
 * @properties: a #GHashTable
 * @pixel_width: pixel width value
 *
 * Sets the "PixelWidth" property.
 **/
void
ms1_server_set_pixel_width (MS1Server *server,
                            GHashTable *properties,
                            gint pixel_width)
{
  g_return_if_fail (properties);

  g_hash_table_insert (properties,
                       MS1_PROP_PIXEL_WIDTH,
                       int_to_value (pixel_width));
}

/**
 * ms1_server_set_pixel_height:
 * @server: a #MS1Server
 * @properties: a #GHashTable
 * @pixel_height: pixel height value
 *
 * Sets the "PixelHeight" property.
 **/
void
ms1_server_set_pixel_height (MS1Server *server,
                             GHashTable *properties,
                             gint pixel_height)
{
  g_return_if_fail (properties);

  g_hash_table_insert (properties,
                       MS1_PROP_PIXEL_HEIGHT,
                       int_to_value (pixel_height));
}

/**
 * ms1_server_set_urls:
 * @server: a #MS1Server
 * @properties: a #GHashTable
 * @urls: @NULL-terminated array of URLs values
 *
 * Sets the "URLs" property.
 **/
void
ms1_server_set_urls (MS1Server *server,
                     GHashTable *properties,
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
                         MS1_PROP_URLS,
                         ptrarray_to_value (url_array));
  }
}

/**
 * ms1_server_set_searchable:
 * @server: a #MS1Server
 * @properties: a #GHashTable
 * @searchable: @TRUE if item is searchable
 *
 * Sets the "Searchable" property.
 **/
void
ms1_server_set_searchable (MS1Server *server,
                           GHashTable *properties,
                           gint searchable)
{
  g_return_if_fail (properties);

  g_hash_table_insert (properties,
                       MS1_PROP_SEARCHABLE,
                       bool_to_value (searchable));
}

/**
 * ms1_server_set_items:
 * @server: a #MS1Server
 * @properties: a #GHashTable
 * @items: a list of children
 *
 * Sets the "Items" property.
 **/
void
ms1_server_set_items (MS1Server *server,
                      GHashTable *properties,
                      GList *items)
{
  GPtrArray *object_paths;

  g_return_if_fail (properties);

  if (items) {
    object_paths = get_object_paths (items);
    g_hash_table_insert (properties,
                         MS1_PROP_ITEMS,
                         ptrarray_to_value (object_paths));
  }
}

/**
 * ms1_server_set_item_count:
 * @server: a #MS1Server
 * @properties: a #GHashTable
 * @item_count: how many items have this container
 *
 * Sets the "ItemCount" property.
 **/
void
ms1_server_set_item_count (MS1Server *server,
                           GHashTable *properties,
                           guint item_count)
{
  g_return_if_fail (properties);

  g_hash_table_insert (properties,
                       MS1_PROP_ITEM_COUNT,
                       uint_to_value (item_count));
}

/**
 * ms1_server_set_containers:
 * @server: a #MS1Server
 * @properties: a #GHashTable
 * @containers: a list of children
 *
 * Sets the "Containers" property.
 **/
void
ms1_server_set_containers (MS1Server *server,
                           GHashTable *properties,
                           GList *containers)
{
  GPtrArray *object_paths;

  g_return_if_fail (properties);

  if (containers) {
    object_paths = get_object_paths (containers);
    g_hash_table_insert (properties,
                         MS1_PROP_CONTAINERS,
                         ptrarray_to_value (object_paths));
  }
}

/**
 * ms1_server_set_container_count:
 * @server: a #MS1Server
 * @properties: a #GHashTable
 * @container_count: how many containers have this container
 *
 * Sets the "ContainerCount" property.
 **/
void
ms1_server_set_container_count (MS1Server *server,
                                GHashTable *properties,
                                guint container_count)
{
  g_return_if_fail (properties);

  g_hash_table_insert (properties,
                       MS1_PROP_CONTAINER_COUNT,
                       uint_to_value (container_count));
}
