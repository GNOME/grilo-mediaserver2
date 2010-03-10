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

#ifndef _RYGEL_GRILO_MEDIA_SERVER_H_
#define _RYGEL_GRILO_MEDIA_SERVER_H_

#include <glib.h>
#include <glib-object.h>
#include <grilo.h>

#define RYGEL_GRILO_MEDIA_SERVER_TYPE           \
  (rygel_grilo_media_server_get_type ())

#define RYGEL_GRILO_MEDIA_SERVER(obj)                           \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj),                           \
                               RYGEL_GRILO_MEDIA_SERVER_TYPE,   \
                               RygelGriloMediaServer))

#define RYGEL_GRILO_IS_MEDIA_SERVER(obj)                        \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj),                           \
                               RYGEL_GRILO_MEDIA_SERVER_TYPE))

#define RYGEL_GRILO_MEDIA_SERVER_CLASS(klass)                   \
  (G_TYPE_CHECK_CLASS_CAST((klass),                             \
                           RYGEL_GRILO_MEDIA_SERVER_TYPE,       \
                           RygelGriloMediaServerClass))

#define RYGEL_GRILO_IS_MEDIA_SERVER_CLASS(klass)                \
  (G_TYPE_CHECK_CLASS_TYPE((klass),                             \
                           RYGEL_GRILO_MEDIA_SERVER_TYPE))

#define RYGEL_GRILO_MEDIA_SERVER_GET_CLASS(obj)                 \
  (G_TYPE_INSTANCE_GET_CLASS ((obj),                            \
                              RYGEL_GRILO_MEDIA_SERVER_TYPE,    \
                              RygelGriloMediaServerClass))

typedef struct _RygelGriloMediaServer        RygelGriloMediaServer;
typedef struct _RygelGriloMediaServerPrivate RygelGriloMediaServerPrivate;

struct _RygelGriloMediaServer {

  GObject parent;

  /*< private >*/
  RygelGriloMediaServerPrivate *priv;
};

typedef struct _RygelGriloMediaServerClass RygelGriloMediaServerClass;

struct _RygelGriloMediaServerClass {

  GObjectClass parent_class;
};

GType rygel_grilo_media_server_get_type (void);

gboolean rygel_grilo_media_server_get_properties (RygelGriloMediaServer *server,
                                                  const gchar *id,
                                                  const gchar **filter,
                                                  DBusGMethodInvocation *context,
                                                  GError **error);

gboolean rygel_grilo_media_server_get_children (RygelGriloMediaServer *server,
                                                const gchar *id,
                                                guint offset,
                                                gint max_count,
                                                const gchar **filter,
                                                DBusGMethodInvocation *context,
                                                GError **error);

RygelGriloMediaServer *rygel_grilo_media_server_new (const gchar *dbus_path,
                                                     GrlMediaSource *source);

#endif /* _RYGEL_GRILO_MEDIA_SERVER_H_ */
