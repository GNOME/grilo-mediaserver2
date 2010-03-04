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
  LAST_PROP
};

typedef struct {
  gchar *parent;
  gchar *name;
} DGriloMediaObjectPrivate;

G_DEFINE_TYPE (DGriloMediaObject, dgrilo_media_object, G_TYPE_OBJECT);

static void
get_property (GObject *object,
              guint prop_id,
              GValue *value,
              GParamSpec *pspec)
{
  DGriloMediaObject *self = DGRILO_MEDIA_OBJECT (object);
  DGriloMediaObjectPrivate *priv = DGRILO_MEDIA_OBJECT_GET_PRIVATE (self);

  switch (prop_id) {
  case PROP_PARENT:
    g_value_set_string (value, priv->parent);
    break;
  case PROP_DISPLAY_NAME:
    g_value_set_string (value, priv->name);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    break;
  }
}

static void
dispose (GObject *object)
{
  DGriloMediaObject *self = DGRILO_MEDIA_OBJECT (object);
  DGriloMediaObjectPrivate *priv = DGRILO_MEDIA_OBJECT_GET_PRIVATE (self);

  g_free (priv->parent);
  g_free (priv->name);

  G_OBJECT_CLASS (dgrilo_media_object_parent_class)->dispose (object);
}

static void
set_property (GObject *object,
              guint prop_id,
              const GValue *value,
              GParamSpec *pspec)
{
  DGriloMediaObject *self = DGRILO_MEDIA_OBJECT (object);
  DGriloMediaObjectPrivate *priv = DGRILO_MEDIA_OBJECT_GET_PRIVATE (self);

  switch (prop_id) {
  case PROP_PARENT:
    priv->parent = g_value_dup_string (value);
    break;
  case PROP_DISPLAY_NAME:
    priv->name = g_value_dup_string (value);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    break;
  }
}

static gboolean
send_name_property_cb (DBusGMethodInvocation *context)
{
  dbus_g_method_return (context, "seria la propiedad");

  return FALSE;
}

static gboolean
send_constant_cb (DBusGMethodInvocation *context)
{
  dbus_g_method_return (context, "constante");

  return FALSE;
}

gboolean
dgrilo_media_object_get (DGriloMediaObject *obj,
                         const gchar *interface,
                         const gchar *property,
                         DBusGMethodInvocation *context,
                         GError **error)
{
  if (g_strcmp0 (property, "DisplayName") == 0) {
    g_idle_add ((GSourceFunc) send_name_property_cb, context);
  } else {
    g_idle_add ((GSourceFunc) send_constant_cb, context);
  }
  return TRUE;
}

static void
dgrilo_media_object_class_init (DGriloMediaObjectClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (DGriloMediaObjectPrivate));

  object_class->get_property = get_property;
  object_class->set_property = set_property;
  object_class->dispose = dispose;

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
                                                        G_PARAM_READABLE | G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY));

  /* Register introspection */
  dbus_g_object_type_install_info (DGRILO_MEDIA_OBJECT_TYPE,
                                   &dbus_glib_dgrilo_media_object_object_info);
}

static void
dgrilo_media_object_init (DGriloMediaObject *server)
{
}

DGriloMediaObject *
dgrilo_media_object_new (const gchar *parent, const gchar *display_name)
{
  DBusGConnection *connection;
  DGriloMediaObject *obj;

  obj = g_object_new (DGRILO_MEDIA_OBJECT_TYPE,
                      "parent", parent,
                      "display-name", display_name,
                      NULL);

  /* Register in dbus */
  connection = dbus_g_bus_get (DBUS_BUS_SESSION, NULL);
  g_assert (connection);

  if (parent) {
    dbus_g_connection_register_g_object (connection,
                                         DGRILO_PATH "/1",
                                         G_OBJECT (obj));
  } else {
    dbus_g_connection_register_g_object (connection,
                                         DGRILO_PATH,
                                         G_OBJECT (obj));
  }

  return obj;
}
