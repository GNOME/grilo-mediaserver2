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

#ifndef _MEDIA_SERVER2_SERVER_H_
#define _MEDIA_SERVER2_SERVER_H_

#include <glib.h>
#include <glib-object.h>

#include <media-server2-common.h>

#define MS2_TYPE_SERVER                         \
  (ms2_server_get_type ())

#define MS2_SERVER(obj)                         \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj),           \
                               MS2_TYPE_SERVER, \
                               MS2Server))

#define MS2_IS_SERVER(obj)                              \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj),                   \
                               MS2_TYPE_SERVER))

#define MS2_SERVER_CLASS(klass)                 \
  (G_TYPE_CHECK_CLASS_CAST((klass),             \
                           MS2_TYPE_SERVER,     \
                           MS2ServerClass))

#define MS2_IS_SERVER_CLASS(klass)              \
  (G_TYPE_CHECK_CLASS_TYPE((klass),             \
                           MS2_TYPE_SERVER))

#define MS2_SERVER_GET_CLASS(obj)               \
  (G_TYPE_INSTANCE_GET_CLASS ((obj),            \
                              MS2_TYPE_SERVER,  \
                              MS2ServerClass))

typedef struct _MS2Server        MS2Server;
typedef struct _MS2ServerPrivate MS2ServerPrivate;

struct _MS2Server {

  GObject parent;

  /*< private >*/
  MS2ServerPrivate *priv;
};

typedef struct _MS2ServerClass MS2ServerClass;

struct _MS2ServerClass {

  GObjectClass parent_class;

  void (*updated) (MS2Server *server,
                   const gchar *id);
};

typedef GHashTable * (*GetPropertiesFunc) (MS2Server *server,
                                           const gchar *id,
                                           const gchar **properties,
                                           gpointer data,
                                           GError **error);

typedef GList * (*GetChildrenFunc) (MS2Server *server,
                                    const gchar *id,
                                    guint offset,
                                    gint max_count,
                                    const gchar **properties,
                                    gpointer data,
                                    GError **error);

GType ms2_server_get_type (void);

MS2Server *ms2_server_new (const gchar *name,
                           gpointer data);

void ms2_server_set_get_properties_func (MS2Server *server,
                                         GetPropertiesFunc get_properties_func);

void ms2_server_set_get_children_func (MS2Server *server,
                                       GetChildrenFunc get_children_func);

void ms2_server_updated (MS2Server *server,
                         const gchar *id);

const gchar *ms2_server_get_name (MS2Server *server);

GHashTable *ms2_server_new_properties_hashtable (void);

void ms2_server_set_id (MS2Server *server,
                        GHashTable *properties,
                        const gchar *id,
                        gboolean is_container);

void ms2_server_set_parent (MS2Server *server,
                            GHashTable *properties,
                            const gchar *parent);

void ms2_server_set_display_name (MS2Server *server,
                                  GHashTable *properties,
                                  const gchar *display_name);

void ms2_server_set_item_type (MS2Server *server,
                               GHashTable *properties,
                               MS2ItemType type);

void ms2_server_set_icon (MS2Server *server,
                          GHashTable *properties,
                          const gchar *icon);

void ms2_server_set_mime_type (MS2Server *server,
                               GHashTable *properties,
                               const gchar *mime_type);

void ms2_server_set_artist (MS2Server *server,
                            GHashTable *properties,
                            const gchar *artist);

void ms2_server_set_album (MS2Server *server,
                           GHashTable *properties,
                           const gchar *album);

void ms2_server_set_date (MS2Server *server,
                          GHashTable *properties,
                          const gchar *date);

void ms2_server_set_dlna_profile (MS2Server *server,
                                  GHashTable *properties,
                                  const gchar *dlna_profile);

void ms2_server_set_thumbnail (MS2Server *server,
                               GHashTable *properties,
                               const gchar *thumbnail);

void ms2_server_set_genre (MS2Server *server,
                           GHashTable *properties,
                           const gchar *genre);

void ms2_server_set_child_count (MS2Server *server,
                                 GHashTable *properties,
                                 gint child_count);

void ms2_server_set_size (MS2Server *server,
                          GHashTable *properties,
                          gint size);

void ms2_server_set_duration (MS2Server *server,
                              GHashTable *properties,
                              gint duration);

void ms2_server_set_bitrate (MS2Server *server,
                             GHashTable *properties,
                             gint bitrate);

void ms2_server_set_sample_rate (MS2Server *server,
                                 GHashTable *properties,
                                 gint sample_rate);

void ms2_server_set_bits_per_sample (MS2Server *server,
                                     GHashTable *properties,
                                     gint bits_per_sample);

void ms2_server_set_width (MS2Server *server,
                           GHashTable *properties,
                           gint width);

void ms2_server_set_height (MS2Server *server,
                            GHashTable *properties,
                            gint height);

void ms2_server_set_color_depth (MS2Server *server,
                                 GHashTable *properties,
                                 gint depth);

void ms2_server_set_pixel_width (MS2Server *server,
                                 GHashTable *properties,
                                 gint pixel_width);

void ms2_server_set_pixel_height (MS2Server *server,
                                  GHashTable *properties,
                                  gint pixel_height);

void ms2_server_set_urls (MS2Server *server,
                          GHashTable *properties,
                          gchar **urls);

#endif /* _MEDIA_SERVER2_SERVER_H_ */
