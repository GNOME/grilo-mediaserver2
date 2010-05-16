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

#ifndef _MEDIA_SERVER1_CLIENT_H_
#define _MEDIA_SERVER1_CLIENT_H_

#include <glib-object.h>
#include <glib.h>

#include "media-server1-common.h"

#define MS1_TYPE_CLIENT                         \
  (ms1_client_get_type ())

#define MS1_CLIENT(obj)                         \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj),           \
                               MS1_TYPE_CLIENT, \
                               MS1Client))

#define MS1_IS_CLIENT(obj)                              \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj),                   \
                               MS1_TYPE_CLIENT))

#define MS1_CLIENT_CLASS(klass)                 \
  (G_TYPE_CHECK_CLASS_CAST((klass),             \
                           MS1_TYPE_CLIENT,     \
                           MS1ClientClass))

#define MS1_IS_CLIENT_CLASS(klass)              \
  (G_TYPE_CHECK_CLASS_TYPE((klass),             \
                           MS1_TYPE_CLIENT))

#define MS1_CLIENT_GET_CLASS(obj)               \
  (G_TYPE_INSTANCE_GET_CLASS ((obj),            \
                              MS1_TYPE_CLIENT,  \
                              MS1ClientClass))

typedef struct _MS1Client        MS1Client;
typedef struct _MS1ClientPrivate MS1ClientPrivate;

struct _MS1Client {

  GObject parent;

  /*< private >*/
  MS1ClientPrivate *priv;
};

typedef struct _MS1ClientClass MS1ClientClass;

struct _MS1ClientClass {

  GObjectClass parent_class;

  void (*updated) (MS1Client *client,
                   const gchar *id);

  void (*destroy) (MS1Client *client);
};

GType ms1_client_get_type (void);

gchar **ms1_client_get_providers (void);

MS1Client *ms1_client_new (const gchar *provider);

const gchar *ms1_client_get_provider_name (MS1Client *client);

GHashTable *ms1_client_get_properties (MS1Client *client,
                                       const gchar *object_path,
                                       gchar **properties,
                                       GError **error);

GList *ms1_client_list_children (MS1Client *client,
                                 const gchar *object_path,
                                 guint offset,
                                 guint max_count,
                                 const gchar **properties,
                                 GError **error);

GList *ms1_client_search_objects (MS1Client *client,
                                  const gchar *object_path,
                                  const gchar *query,
                                  guint offset,
                                  guint max_count,
                                  const gchar **properties,
                                  GError **error);

const gchar *ms1_client_get_root_path (MS1Client *client);

const gchar *ms1_client_get_path (GHashTable *properties);

const gchar *ms1_client_get_parent (GHashTable *properties);

const gchar *ms1_client_get_display_name (GHashTable *properties);

MS1ItemType ms1_client_get_item_type (GHashTable *properties);

const gchar *ms1_client_get_item_type_string (GHashTable *properties);

const gchar *ms1_client_get_mime_type (GHashTable *properties);

const gchar *ms1_client_get_artist (GHashTable *properties);

const gchar *ms1_client_get_album (GHashTable *properties);

const gchar *ms1_client_get_date (GHashTable *properties);

const gchar *ms1_client_get_dlna_profile (GHashTable *properties);

const gchar *ms1_client_get_thumbnail (GHashTable *properties);

const gchar *ms1_client_get_album_art (GHashTable *properties);

const gchar *ms1_client_get_genre (GHashTable *properties);

gint  ms1_client_get_size (GHashTable *properties);

gint  ms1_client_get_duration (GHashTable *properties);

gint  ms1_client_get_bitrate (GHashTable *properties);

gint  ms1_client_get_sample_rate (GHashTable *properties);

gint  ms1_client_get_bits_per_sample (GHashTable *properties);

gint  ms1_client_get_width (GHashTable *properties);

gint ms1_client_get_height (GHashTable *properties);

gint ms1_client_get_color_depth (GHashTable *properties);

gint ms1_client_get_pixel_width (GHashTable *properties);

gint ms1_client_get_pixel_height (GHashTable *properties);

gchar **ms1_client_get_urls (GHashTable *properties);

#endif /* _MEDIA_SERVER1_CLIENT_H_ */
