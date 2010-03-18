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
#include "rygel-grilo-media-object.h"
#include "rygel-grilo-media-object-glue.h"

#define RYGEL_GRILO_MEDIA_OBJECT_GET_PRIVATE(o)                         \
  G_TYPE_INSTANCE_GET_PRIVATE((o), RYGEL_GRILO_MEDIA_OBJECT_TYPE, RygelGriloMediaObjectPrivate)

enum {
  PROP_0,
  PROP_PARENT,
  PROP_DISPLAY_NAME,
  PROP_DBUS_PATH,
  PROP_GRL_MEDIA,
  PROP_PARENT_MEDIA,
  LAST_PROP
};

/*
 * Private RygelGriloMediaObject
 *   dbus_path...: dbus path name where object is registered
 *   parent_path.: dbus path name where parent object is registered
 *   parent_media: parent object
 *   grl_media...: grilo media object being wrapped
 */
typedef struct {
  gchar *dbus_path;
  gchar *parent_path;
  RygelGriloMediaObject *parent_media;
  GrlMedia *grl_media;
} RygelGriloMediaObjectPrivate;

G_DEFINE_TYPE (RygelGriloMediaObject, rygel_grilo_media_object, G_TYPE_OBJECT);

/* Function to get a property from a RygelGriloMediaObject object */
static void
rygel_grilo_media_object_get_property (GObject *object,
                                       guint prop_id,
                                       GValue *value,
                                       GParamSpec *pspec)
{
  GrlMediaPlugin *source;
  GrlPluginRegistry *registry;
  RygelGriloMediaObject *self =
    RYGEL_GRILO_MEDIA_OBJECT (object);
  RygelGriloMediaObjectPrivate *priv =
    RYGEL_GRILO_MEDIA_OBJECT_GET_PRIVATE (self);
  const gchar *name;
  const gchar *source_id;

  switch (prop_id) {
  case PROP_PARENT:
    g_value_set_string (value, priv->parent_path);
    break;
  case PROP_DISPLAY_NAME:
    name = grl_media_get_title (priv->grl_media);

    /* Use source name if it has no title */
    if (!name) {
      source_id = grl_media_get_source (priv->grl_media);
      registry = grl_plugin_registry_get_instance ();
      source = grl_plugin_registry_lookup_source (registry, source_id);
      name = grl_metadata_source_get_name (GRL_METADATA_SOURCE (source));
    }

    /* If still does not have a name, use "Unknown" */
    if (!name) {
      name =  "Unknown";
    }

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

/* Function to set a property from a RygelGriloMediaObject object */
static void
rygel_grilo_media_object_set_property (GObject *object,
                                       guint prop_id,
                                       const GValue *value,
                                       GParamSpec *pspec)
{
  RygelGriloMediaObject *self =
    RYGEL_GRILO_MEDIA_OBJECT (object);
  RygelGriloMediaObjectPrivate *priv =
    RYGEL_GRILO_MEDIA_OBJECT_GET_PRIVATE (self);

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

/* Dispose a RygelGriloMediaObject and its resources */
static void
rygel_grilo_media_object_dispose (GObject *object)
{
  RygelGriloMediaObject *self =
    RYGEL_GRILO_MEDIA_OBJECT (object);
  RygelGriloMediaObjectPrivate *priv =
    RYGEL_GRILO_MEDIA_OBJECT_GET_PRIVATE (self);

  g_free (priv->parent_path);
  g_free (priv->dbus_path);
  g_object_unref (priv->grl_media);

  G_OBJECT_CLASS (rygel_grilo_media_object_parent_class)->dispose (object);
}

/* Class init function */
static void
rygel_grilo_media_object_class_init (RygelGriloMediaObjectClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (RygelGriloMediaObjectPrivate));

  klass->index = 1;

  object_class->get_property = rygel_grilo_media_object_get_property;
  object_class->set_property = rygel_grilo_media_object_set_property;
  object_class->dispose = rygel_grilo_media_object_dispose;

  g_object_class_install_property (object_class,
                                   PROP_PARENT,
                                   g_param_spec_string ("parent",
                                                        "Parent",
                                                        "Parent",
                                                        NULL,
                                                        G_PARAM_READABLE |
                                                        G_PARAM_WRITABLE |
                                                        G_PARAM_CONSTRUCT_ONLY));

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
                                                        G_PARAM_READABLE |
                                                        G_PARAM_WRITABLE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class,
                                   PROP_GRL_MEDIA,
                                   g_param_spec_object ("grl-media",
                                                        "GrlMedia",
                                                        "Grilo Media",
                                                        GRL_TYPE_MEDIA,
                                                        G_PARAM_READABLE |
                                                        G_PARAM_WRITABLE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class,
                                   PROP_PARENT_MEDIA,
                                   g_param_spec_object ("parent-media",
                                                        "ParentMedia",
                                                        "MediaObject parent",
                                                        RYGEL_GRILO_MEDIA_OBJECT_TYPE,
                                                        G_PARAM_READABLE |
                                                        G_PARAM_WRITABLE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  /* Register introspection */
  dbus_g_object_type_install_info (RYGEL_GRILO_MEDIA_OBJECT_TYPE,
                                   &dbus_glib_rygel_grilo_media_object_object_info);
}

/* Object init function */
static void
rygel_grilo_media_object_init (RygelGriloMediaObject *obj)
{
}

/**
 * rygel_grilo_media_object_get_dbus_path:
 * @obj: a RygelGrilo object
 *
 * Returns the path where the object is registered in dbus. Value shouldn't be modified.
 *
 * Returns: a dbus_path
 **/
const gchar *
rygel_grilo_media_object_get_dbus_path (RygelGriloMediaObject *obj)
{
  return RYGEL_GRILO_MEDIA_OBJECT_GET_PRIVATE (obj)->dbus_path;
}

/**
 * rygel_grilo_media_object_dbus_register:
 * @obj: a RygelGrilo object
 *
 * Registers the object in dbus.
 *
 * Returns: @TRUE if object was registered
 **/
gboolean
rygel_grilo_media_object_dbus_register (RygelGriloMediaObject *obj)
{
  DBusGConnection *connection;
  GError *error = NULL;
  RygelGriloMediaObjectPrivate *priv =
    RYGEL_GRILO_MEDIA_OBJECT_GET_PRIVATE (obj);

  connection = dbus_g_bus_get (DBUS_BUS_SESSION, &error);
  if (!connection) {
    g_printerr ("Could not connect to session bus, %s\n", error->message);
    g_clear_error (&error);
    return FALSE;
  }

  dbus_g_connection_register_g_object (connection,
                                       priv->dbus_path,
                                       G_OBJECT (obj));

  return TRUE;
}

