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
#include "dgrilo-media-container.h"
#include "dgrilo-media-container-glue.h"

#define DGRILO_PATH "/org/gnome/UPnP/MediaServer1/DGrilo"

#define DGRILO_MEDIA_CONTAINER_GET_PRIVATE(o)                           \
  G_TYPE_INSTANCE_GET_PRIVATE((o), DGRILO_MEDIA_CONTAINER_TYPE, DGriloMediaContainerPrivate)

enum {
  PROP_0,
  PROP_ITEM_COUNT,
  PROP_CONTAINER_COUNT,
  LAST_PROP
};

typedef struct {
  guint item_count;
  guint container_count;
  GList *items;
  GList *containers;
} DGriloMediaContainerPrivate;

G_DEFINE_TYPE (DGriloMediaContainer, dgrilo_media_container, DGRILO_MEDIA_OBJECT_TYPE);

static void
dgrilo_media_container_get_property (GObject *object,
                                     guint prop_id,
                                     GValue *value,
                                     GParamSpec *pspec)
{
  DGriloMediaContainer *self = DGRILO_MEDIA_CONTAINER (object);
  DGriloMediaContainerPrivate *priv = DGRILO_MEDIA_CONTAINER_GET_PRIVATE (self);

  switch (prop_id) {
  case PROP_ITEM_COUNT:
    g_value_set_uint (value, priv->item_count);
    break;
  case PROP_CONTAINER_COUNT:
    g_value_set_uint (value, priv->container_count);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    break;
  }
}

static void
dgrilo_media_container_set_property (GObject *object,
                                     guint prop_id,
                                     const GValue *value,
                                     GParamSpec *pspec)
{
  DGriloMediaContainer *self = DGRILO_MEDIA_CONTAINER (object);
  DGriloMediaContainerPrivate *priv = DGRILO_MEDIA_CONTAINER_GET_PRIVATE (self);

  switch (prop_id) {
  case PROP_ITEM_COUNT:
    priv->item_count = g_value_get_uint (value);
    break;
  case PROP_CONTAINER_COUNT:
    priv->container_count = g_value_get_uint (value);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    break;
  }
}

static void
dgrilo_media_container_dispose (GObject *object)
{
  G_OBJECT_CLASS (dgrilo_media_container_parent_class)->dispose (object);
}


gboolean
dgrilo_media_container_get (DGriloMediaContainer *obj,
                            const gchar *interface,
                            const gchar *property,
                            DBusGMethodInvocation *context,
                            GError **error)
{
  GValue val = { 0 };
  gchar *display_name = NULL;
  gchar *parent_path = NULL;
  guint container_count;
  guint item_count;

  if (g_strcmp0 (property, "DisplayName") == 0) {
    g_object_get (obj, "display-name", &display_name, NULL);
    g_value_init (&val, G_TYPE_STRING);
    g_value_take_string (&val, display_name);
  } else if (g_strcmp0 (property, "Parent") == 0) {
    g_object_get (obj, "parent", &parent_path, NULL);
    g_value_init (&val, G_TYPE_STRING);
    g_value_take_string (&val, parent_path);
  } else if (g_strcmp0 (property, "ContainerCount") == 0) {
    g_object_get (obj, "container-count", &container_count, NULL);
    g_value_init (&val, G_TYPE_UINT);
    g_value_set_uint (&val, container_count);
  } else if (g_strcmp0 (property, "ItemCount") == 0) {
    g_object_get (obj, "item-count", &item_count, NULL);
    g_value_init (&val, G_TYPE_UINT);
    g_value_set_uint (&val, item_count);
  }

  dbus_g_method_return (context, &val);
  g_value_unset (&val);

  return TRUE;
}

static void
dgrilo_media_container_class_init (DGriloMediaContainerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (DGriloMediaContainerPrivate));

  object_class->get_property = dgrilo_media_container_get_property;
  object_class->set_property = dgrilo_media_container_set_property;
  object_class->dispose = dgrilo_media_container_dispose;

  g_object_class_install_property (object_class,
                                   PROP_ITEM_COUNT,
                                   g_param_spec_uint ("item-count",
                                                      "ItemCount",
                                                      "Number of Items",
                                                      0, G_MAXUINT, 0,
                                                      G_PARAM_READABLE | G_PARAM_WRITABLE));

  g_object_class_install_property (object_class,
                                   PROP_CONTAINER_COUNT,
                                   g_param_spec_uint ("container-count",
                                                      "ContainerCount",
                                                      "Number of Containers",
                                                      0, G_MAXUINT, 0,
                                                      G_PARAM_READABLE | G_PARAM_WRITABLE));

  /* Register introspection */
  dbus_g_object_type_install_info (DGRILO_MEDIA_CONTAINER_TYPE,
                                   &dbus_glib_dgrilo_media_container_object_info);
}

static void
dgrilo_media_container_init (DGriloMediaContainer *server)
{
}

DGriloMediaContainer *
dgrilo_media_container_new_with_dbus_path (const gchar *dbus_path,
                                           GrlMedia *media)
{
  DGriloMediaContainer *obj;

  obj = g_object_new (DGRILO_MEDIA_CONTAINER_TYPE,
                      "parent", dbus_path,
                      "display-name", media? grl_media_get_title (media): "Unknown",
                      "dbus-path", dbus_path,
                      "grl-media", media,
                      NULL);

  dgrilo_media_object_dbus_register (DGRILO_MEDIA_OBJECT (obj));

  return obj;
}

DGriloMediaContainer *
dgrilo_media_container_new_with_parent (DGriloMediaObject *parent,
                                        GrlMedia *media)
{
  DGriloMediaContainer *obj;
  DGriloMediaObjectClass *klass;
  gchar *dbus_path;

  klass = DGRILO_MEDIA_OBJECT_GET_CLASS (parent);
  dbus_path = g_strdup_printf ("%s/%u",
                               dgrilo_media_object_get_dbus_path (parent),
                               klass->index);

  obj = g_object_new (DGRILO_MEDIA_CONTAINER_TYPE,
                      "parent", dgrilo_media_object_get_dbus_path (parent),
                      "display-name", media? grl_media_get_title (media): "Unknown",
                      "dbus-path", dbus_path,
                      "grl-media", media,
                      "parent-media", parent,
                      NULL);

  g_free (dbus_path);
  klass->index++;
  dgrilo_media_object_dbus_register (DGRILO_MEDIA_OBJECT (obj));

  return obj;
}
