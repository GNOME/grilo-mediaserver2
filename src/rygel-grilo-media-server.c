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

#include <dbus/dbus-glib.h>
#include "rygel-grilo-media-server.h"
#include "rygel-grilo-media-server-glue.h"

#define ROOT_PARENT_ID "0"

#define MS_PROP_PARENT       "parent"
#define MS_PROP_DISPLAY_NAME "display-name"
#define MS_PROP_TYPE         "type"
#define MS_PROP_CHILD_COUNT  "child-count"
#define MS_PROP_URLS         "URLs"
#define MS_PROP_MIME_TYPE    "mime-type"
#define MS_PROP_ARTIST       "artist"
#define MS_PROP_ALBUM        "album"
#define MS_PROP_DURATION     "duration"
#define MS_PROP_BITRATE      "bitrate"
#define MS_PROP_WIDTH        "width"
#define MS_PROP_HEIGHT       "height"
#define MS_PROP_GENRE        "genre"

#define DBUS_TYPE_G_ARRAY_OF_STRING                             \
  (dbus_g_type_get_collection ("GPtrArray", G_TYPE_STRING))

G_DEFINE_TYPE (RygelGriloMediaServer, rygel_grilo_media_server, G_TYPE_OBJECT);

static void
rygel_grilo_media_server_dbus_register (RygelGriloMediaServer *obj,
                                        const gchar *dbus_path)
{
  DBusGConnection *connection;

  connection = dbus_g_bus_get (DBUS_BUS_SESSION, NULL);
  g_assert (connection);

  dbus_g_connection_register_g_object (connection,
                                       dbus_path,
                                       G_OBJECT (obj));
}

static void
rygel_grilo_media_server_dispose (GObject *object)
{
  G_OBJECT_CLASS (rygel_grilo_media_server_parent_class)->dispose (object);
}

static void
rygel_grilo_media_server_class_init (RygelGriloMediaServerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = rygel_grilo_media_server_dispose;

  /* Register introspection */
  dbus_g_object_type_install_info (RYGEL_GRILO_MEDIA_SERVER_TYPE,
                                   &dbus_glib_rygel_grilo_media_server_object_info);
}

static void
rygel_grilo_media_server_init (RygelGriloMediaServer *obj)
{
}

gboolean
rygel_grilo_media_server_get_properties (RygelGriloMediaServer *server,
                                         const gchar *id,
                                         GPtrArray *filter,
                                         DBusGMethodInvocation *context,
                                         GError **error)
{
  return TRUE;
}

gboolean
rygel_grilo_media_server_get_children (RygelGriloMediaServer *server,
                                       const gchar *id,
                                       guint offset,
                                       guint max_count,
                                       GPtrArray *filter,
                                       DBusGMethodInvocation *context,
                                       GError **error)
{
  return TRUE;
}

RygelGriloMediaServer *
rygel_grilo_media_server_new (const gchar *dbus_path)
{
  RygelGriloMediaServer *obj;

  obj = g_object_new (RYGEL_GRILO_MEDIA_SERVER_TYPE, NULL);

  rygel_grilo_media_server_dbus_register (obj, dbus_path);

  return obj;
}
