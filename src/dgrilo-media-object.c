/*
 * Copyright (C) 2010 Igalia S.L.
 *
 * Contact: Iago Toral Quiroga <itoral@igalia.com>
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
#include "dgrilo-media-object.h"
#include "dgrilo-media-object-glue.h"

#define DGRILO_PATH "/org/gnome/UPnP/MediaObject1/DGrilo"

#define DGRILO_MEDIA_OBJECT_GET_PRIVATE(o)      \
  G_TYPE_INSTANCE_GET_PRIVATE((o), DGRILO_MEDIA_OBJECT_TYPE, DGriloMediaObjectPrivate)

enum {
  PROP_0,
  PROP_PARENT,
  PROP_DISPLAY_NAME,
  PROP_DBUS_PATH,
  PROP_GRL_MEDIA,
  PROP_PARENT_MEDIA,
  LAST_PROP
};

typedef struct {
  gchar *dbus_path;
  gchar *parent_path;
  DGriloMediaObject *parent_media;
  GrlMedia *grl_media;
} DGriloMediaObjectPrivate;

G_DEFINE_TYPE (DGriloMediaObject, dgrilo_media_object, G_TYPE_OBJECT);

const gchar *
dgrilo_media_object_get_dbus_path (DGriloMediaObject *obj)
{
  return DGRILO_MEDIA_OBJECT_GET_PRIVATE (obj)->dbus_path;
}

static void
dgrilo_media_object_get_property (GObject *object,
                                  guint prop_id,
                                  GValue *value,
                                  GParamSpec *pspec)
{
  DGriloMediaObject *self = DGRILO_MEDIA_OBJECT (object);
  DGriloMediaObjectPrivate *priv = DGRILO_MEDIA_OBJECT_GET_PRIVATE (self);
  const gchar *name;

  switch (prop_id) {
  case PROP_PARENT:
    g_value_set_string (value, priv->parent_path);
    break;
  case PROP_DISPLAY_NAME:
    name = priv->grl_media? grl_media_get_title (priv->grl_media): "Unknown";
    name = name? name: "Unknown";
    g_value_set_string (value, name);
    break;
  case PROP_DBUS_PATH:
    g_value_set_string (value, priv->dbus_path);
    break;
  case PROP_GRL_MEDIA:
    g_value_set_object (value, priv->grl_media);
    break;
  case PROP_PARENT_MEDIA:
    g_value_set_object (value, priv->parent_media);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    break;
  }
}

static void
dgrilo_media_object_set_property (GObject *object,
                                  guint prop_id,
                                  const GValue *value,
                                  GParamSpec *pspec)
{
  DGriloMediaObject *self = DGRILO_MEDIA_OBJECT (object);
  DGriloMediaObjectPrivate *priv = DGRILO_MEDIA_OBJECT_GET_PRIVATE (self);

  switch (prop_id) {
  case PROP_PARENT:
    priv->parent_path = g_value_dup_string (value);
    break;
  case PROP_DBUS_PATH:
    priv->dbus_path = g_value_dup_string (value);
    break;
  case PROP_GRL_MEDIA:
    priv->grl_media = g_value_get_object (value);
    break;
  case PROP_PARENT_MEDIA:
    priv->parent_media = g_value_get_object (value);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    break;
  }
}

static void
dgrilo_media_object_dispose (GObject *object)
{
  DGriloMediaObject *self = DGRILO_MEDIA_OBJECT (object);
  DGriloMediaObjectPrivate *priv = DGRILO_MEDIA_OBJECT_GET_PRIVATE (self);

  g_free (priv->parent_path);
  g_free (priv->dbus_path);
  g_object_unref (priv->grl_media);

  G_OBJECT_CLASS (dgrilo_media_object_parent_class)->dispose (object);
}

static void
dgrilo_media_object_class_init (DGriloMediaObjectClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (DGriloMediaObjectPrivate));

  klass->index = 1;

  object_class->get_property = dgrilo_media_object_get_property;
  object_class->set_property = dgrilo_media_object_set_property;
  object_class->dispose = dgrilo_media_object_dispose;

  g_object_class_install_property (object_class,
                                   PROP_PARENT,
                                   g_param_spec_string ("parent",
                                                        "Parent",
                                                        "Parent",
                                                        NULL,
                                                        G_PARAM_READABLE | G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class,
                                   PROP_DISPLAY_NAME,
                                   g_param_spec_string ("display-name",
                                                        "DisplayName",
                                                        "Pretty name",
                                                        NULL,
                                                        G_PARAM_READABLE));

  g_object_class_install_property (object_class,
                                   PROP_DBUS_PATH,
                                   g_param_spec_string ("dbus-path",
                                                        "DBusPath",
                                                        "DBus Path where object is registered",
                                                        NULL,
                                                        G_PARAM_READABLE | G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class,
                                   PROP_GRL_MEDIA,
                                   g_param_spec_object ("grl-media",
                                                        "GrlMedia",
                                                        "Grilo Media",
                                                        GRL_TYPE_MEDIA,
                                                        G_PARAM_READABLE | G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class,
                                   PROP_PARENT_MEDIA,
                                   g_param_spec_object ("parent-media",
                                                        "ParentMedia",
                                                        "MediaObject parent",
                                                        DGRILO_MEDIA_OBJECT_TYPE,
                                                        G_PARAM_READABLE | G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY));

  /* Register introspection */
  dbus_g_object_type_install_info (DGRILO_MEDIA_OBJECT_TYPE,
                                   &dbus_glib_dgrilo_media_object_object_info);
}

static void
dgrilo_media_object_init (DGriloMediaObject *obj)
{
}

void
dgrilo_media_object_dbus_register (DGriloMediaObject *obj)
{
  DBusGConnection *connection;
  DGriloMediaObjectPrivate *priv =
    DGRILO_MEDIA_OBJECT_GET_PRIVATE (obj);

  connection = dbus_g_bus_get (DBUS_BUS_SESSION, NULL);
  g_assert (connection);

  dbus_g_connection_register_g_object (connection,
                                       priv->dbus_path,
                                       G_OBJECT (obj));
}

