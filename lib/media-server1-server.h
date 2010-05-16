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

#ifndef _MEDIA_SERVER1_SERVER_H_
#define _MEDIA_SERVER1_SERVER_H_

#include <glib.h>
#include <glib-object.h>

#include "media-server1-common.h"

#define MS1_TYPE_SERVER                         \
  (ms1_server_get_type ())

#define MS1_SERVER(obj)                         \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj),           \
                               MS1_TYPE_SERVER, \
                               MS1Server))

#define MS1_IS_SERVER(obj)                              \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj),                   \
                               MS1_TYPE_SERVER))

#define MS1_SERVER_CLASS(klass)                 \
  (G_TYPE_CHECK_CLASS_CAST((klass),             \
                           MS1_TYPE_SERVER,     \
                           MS1ServerClass))

#define MS1_IS_SERVER_CLASS(klass)              \
  (G_TYPE_CHECK_CLASS_TYPE((klass),             \
                           MS1_TYPE_SERVER))

#define MS1_SERVER_GET_CLASS(obj)               \
  (G_TYPE_INSTANCE_GET_CLASS ((obj),            \
                              MS1_TYPE_SERVER,  \
                              MS1ServerClass))

typedef struct _MS1Server        MS1Server;
typedef struct _MS1ServerPrivate MS1ServerPrivate;

struct _MS1Server {

  GObject parent;

  /*< private >*/
  MS1ServerPrivate *priv;
};

typedef struct _MS1ServerClass MS1ServerClass;

struct _MS1ServerClass {

  GObjectClass parent_class;
};

typedef GHashTable * (*GetPropertiesFunc) (MS1Server *server,
                                           const gchar *id,
                                           const gchar **properties,
                                           gpointer data,
                                           GError **error);

typedef GList * (*ListChildrenFunc) (MS1Server *server,
                                     const gchar *id,
                                     guint offset,
                                     guint max_count,
                                     const gchar **properties,
                                     gpointer data,
                                     GError **error);

typedef GList * (*SearchObjectsFunc) (MS1Server *server,
                                      const gchar *id,
                                      const gchar *query,
                                      guint offset,
                                      guint max_count,
                                      const gchar **properties,
                                      gpointer data,
                                      GError **error);

GType ms1_server_get_type (void);

MS1Server *ms1_server_new (const gchar *name,
                           gpointer data);

void ms1_server_set_get_properties_func (MS1Server *server,
                                         GetPropertiesFunc get_properties_func);

void ms1_server_set_list_children_func (MS1Server *server,
                                        ListChildrenFunc list_children_func);

void ms1_server_set_search_objects_func (MS1Server *server,
                                         SearchObjectsFunc search_objects_func);

void ms1_server_updated (MS1Server *server,
                         const gchar *id);

const gchar *ms1_server_get_name (MS1Server *server);

GHashTable *ms1_server_new_properties_hashtable (void);

void ms1_server_set_path (MS1Server *server,
                          GHashTable *properties,
                          const gchar *id,
                          gboolean is_container);

void ms1_server_set_parent (MS1Server *server,
                            GHashTable *properties,
                            const gchar *parent);

void ms1_server_set_display_name (MS1Server *server,
                                  GHashTable *properties,
                                  const gchar *display_name);

void ms1_server_set_item_type (MS1Server *server,
                               GHashTable *properties,
                               MS1ItemType type);

void ms1_server_set_icon (MS1Server *server,
                          GHashTable *properties,
                          const gchar *icon);

void ms1_server_set_mime_type (MS1Server *server,
                               GHashTable *properties,
                               const gchar *mime_type);

void ms1_server_set_artist (MS1Server *server,
                            GHashTable *properties,
                            const gchar *artist);

void ms1_server_set_album (MS1Server *server,
                           GHashTable *properties,
                           const gchar *album);

void ms1_server_set_date (MS1Server *server,
                          GHashTable *properties,
                          const gchar *date);

void ms1_server_set_dlna_profile (MS1Server *server,
                                  GHashTable *properties,
                                  const gchar *dlna_profile);

void ms1_server_set_genre (MS1Server *server,
                           GHashTable *properties,
                           const gchar *genre);

void ms1_server_set_thumbnail (MS1Server *server,
                               GHashTable *properties,
                               const gchar *genre);

void ms1_server_set_album_art (MS1Server *server,
                               GHashTable *properties,
                               const gchar *album_art);

void ms1_server_set_size (MS1Server *server,
                          GHashTable *properties,
                          gint size);

void ms1_server_set_duration (MS1Server *server,
                              GHashTable *properties,
                              gint duration);

void ms1_server_set_bitrate (MS1Server *server,
                             GHashTable *properties,
                             gint bitrate);

void ms1_server_set_sample_rate (MS1Server *server,
                                 GHashTable *properties,
                                 gint sample_rate);

void ms1_server_set_bits_per_sample (MS1Server *server,
                                     GHashTable *properties,
                                     gint bits_per_sample);

void ms1_server_set_width (MS1Server *server,
                           GHashTable *properties,
                           gint width);

void ms1_server_set_height (MS1Server *server,
                            GHashTable *properties,
                            gint height);

void ms1_server_set_color_depth (MS1Server *server,
                                 GHashTable *properties,
                                 gint depth);

void ms1_server_set_pixel_width (MS1Server *server,
                                 GHashTable *properties,
                                 gint pixel_width);

void ms1_server_set_pixel_height (MS1Server *server,
                                  GHashTable *properties,
                                  gint pixel_height);

void ms1_server_set_urls (MS1Server *server,
                          GHashTable *properties,
                          gchar **urls);

void ms1_server_set_searchable (MS1Server *server,
                                GHashTable *properties,
                                gboolean searchable);

void ms1_server_set_items (MS1Server *server,
                           GHashTable *properties,
                           GList *items);

void ms1_server_set_item_count (MS1Server *server,
                                GHashTable *properties,
                                guint item_count);

void ms1_server_set_containers (MS1Server *server,
                                GHashTable *properties,
                                GList *containers);

void ms1_server_set_container_count (MS1Server *server,
                                     GHashTable *properties,
                                     guint container_count);

#endif /* _MEDIA_SERVER1_SERVER_H_ */
