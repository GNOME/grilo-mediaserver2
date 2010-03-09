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

#define ROOT_PARENT_ID "-1"

#define RYGEL_GRILO_MEDIA_SERVER_GET_PRIVATE(o)                         \
  G_TYPE_INSTANCE_GET_PRIVATE((o), RYGEL_GRILO_MEDIA_SERVER_TYPE, RygelGriloMediaServerPrivate)

#define DBUS_TYPE_G_ARRAY_OF_STRING                             \
  (dbus_g_type_get_collection ("GPtrArray", G_TYPE_STRING))

typedef struct {
  GrlMedia *grl_media;
  gchar *parent_id;
} Media;

typedef struct {
  gchar *dbus_path;
  GHashTable *medias;
} RygelGriloMediaServerPrivate;

G_DEFINE_TYPE (RygelGriloMediaServer, rygel_grilo_media_server, G_TYPE_OBJECT);

static void
free_media (Media *m)
{
  g_object_unref (m->grl_media);
  g_free (m);
}

static void
rygel_grilo_media_server_add_media (RygelGriloMediaServer *server,
                                    GrlMedia *media,
                                    const gchar *parent_id)
{
  Media *m;
  RygelGriloMediaServerPrivate *priv =
    RYGEL_GRILO_MEDIA_SERVER_GET_PRIVATE (server);
  const gchar *media_id;

  m = g_new (Media, 1);
  m->grl_media = media;
  m->parent_id = (gchar *) parent_id;

  media_id = grl_media_get_id (media);
  if (!media_id) {
    media_id = ROOT_PARENT_ID;
  }

  g_hash_table_insert (priv->medias, (gpointer) media_id, m);
}

static void
rygel_grilo_media_server_dbus_register (RygelGriloMediaServer *obj)
{
  DBusGConnection *connection;
  RygelGriloMediaServerPrivate *priv =
    RYGEL_GRILO_MEDIA_SERVER_GET_PRIVATE (obj);

  connection = dbus_g_bus_get (DBUS_BUS_SESSION, NULL);
  g_assert (connection);

  dbus_g_connection_register_g_object (connection,
                                       priv->dbus_path,
                                       G_OBJECT (obj));
}

static void
rygel_grilo_media_server_dispose (GObject *object)
{
  RygelGriloMediaServer *self =
    RYGEL_GRILO_MEDIA_SERVER (object);
  RygelGriloMediaServerPrivate *priv =
    RYGEL_GRILO_MEDIA_SERVER_GET_PRIVATE (self);

  g_hash_table_unref (priv->medias);
  g_free (priv->dbus_path);

  G_OBJECT_CLASS (rygel_grilo_media_server_parent_class)->dispose (object);
}

static void
rygel_grilo_media_server_class_init (RygelGriloMediaServerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (RygelGriloMediaServerPrivate));

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
rygel_grilo_media_server_new (const gchar *dbus_path,
                              GrlMedia *media)
{
  RygelGriloMediaServer *obj;
  RygelGriloMediaServerPrivate *priv;

  obj = g_object_new (RYGEL_GRILO_MEDIA_SERVER_TYPE, NULL);

  priv = RYGEL_GRILO_MEDIA_SERVER_GET_PRIVATE (obj);
  priv->medias = g_hash_table_new_full (g_str_hash,
                                        g_str_equal,
                                        NULL,
                                        (GDestroyNotify) free_media);

  priv->dbus_path = g_strdup (dbus_path);
  rygel_grilo_media_server_add_media (obj, media, ROOT_PARENT_ID);

  rygel_grilo_media_server_dbus_register (obj);
  return obj;
}
