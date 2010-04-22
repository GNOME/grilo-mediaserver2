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

#include <dbus/dbus-glib-bindings.h>
#include <dbus/dbus-glib.h>
#include <string.h>

#include "media-server2-server-private.h"
#include "media-server2-server-glue.h"
#include "media-server2-server.h"

#define MS2_DBUS_SERVICE_PREFIX "org.gnome.UPnP.MediaServer2."
#define MS2_DBUS_PATH_PREFIX    "/org/gnome/UPnP/MediaServer2/"

#define DBUS_TYPE_G_ARRAY_OF_STRING                             \
  (dbus_g_type_get_collection ("GPtrArray", G_TYPE_STRING))

#define MS2_SERVER_GET_PRIVATE(o)                                       \
  G_TYPE_INSTANCE_GET_PRIVATE((o), MS2_TYPE_SERVER, MS2ServerPrivate)

enum {
  UPDATED,
  LAST_SIGNAL
};

/*
 * Private MS2Server structure
 *   name: provider name
 *   data: holds stuff for owner
 *   get_children: function to get children
 *   get_properties: function to get properties
 */
struct _MS2ServerPrivate {
  gchar *name;
  gpointer *data;
  GetChildrenFunc get_children;
  GetPropertiesFunc get_properties;
};

static guint32 signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE (MS2Server, ms2_server, G_TYPE_OBJECT);

/******************** PRIVATE API ********************/

/* Free gvalue */
static void
free_value (GValue *value)
{
  g_value_unset (value);
  g_free (value);
}

/* Puts a string in a gvalue */
static GValue *
str_to_value (const gchar *str)
{
  GValue *val = NULL;

  if (str) {
    val = g_new0 (GValue, 1);
    g_value_init (val, G_TYPE_STRING);
    g_value_set_string (val, str);
  }

  return val;
}

/* Puts an int in a gvalue */
static GValue *
int_to_value (gint number)
{
  GValue *val = NULL;

  val = g_new0 (GValue, 1);
  g_value_init (val, G_TYPE_INT);
  g_value_set_int (val, number);

  return val;
}

/* Puts a gptrarray in a gvalue */
static GValue *
ptrarray_to_value (GPtrArray *array)
{
  GValue *val = NULL;

  val = g_new0 (GValue, 1);
  g_value_init (val, DBUS_TYPE_G_ARRAY_OF_STRING);
  g_value_take_boxed (val, array);

  return val;
}

/* Returns the unknown value for the property */
static GValue *
get_unknown_value (const gchar *property)
{
  GValue *val;
  GPtrArray *ptrarray;

  if (g_strcmp0 (property, MS2_PROP_URLS) == 0) {
    ptrarray = g_ptr_array_sized_new (1);
    g_ptr_array_add (ptrarray, g_strdup (MS2_UNKNOWN_STR));
    val = ptrarray_to_value (ptrarray);
  } else if (g_strcmp0 (property, MS2_PROP_CHILD_COUNT) == 0 ||
             g_strcmp0 (property, MS2_PROP_SIZE) == 0 ||
             g_strcmp0 (property, MS2_PROP_DURATION) == 0 ||
             g_strcmp0 (property, MS2_PROP_BITRATE) == 0 ||
             g_strcmp0 (property, MS2_PROP_SAMPLE_RATE) == 0 ||
             g_strcmp0 (property, MS2_PROP_BITS_PER_SAMPLE) == 0 ||
             g_strcmp0 (property, MS2_PROP_WIDTH) == 0 ||
             g_strcmp0 (property, MS2_PROP_HEIGHT) == 0 ||
             g_strcmp0 (property, MS2_PROP_COLOR_DEPTH) == 0 ||
             g_strcmp0 (property, MS2_PROP_PIXEL_WIDTH) == 0 ||
             g_strcmp0 (property, MS2_PROP_PIXEL_HEIGHT) == 0) {
    val = int_to_value (MS2_UNKNOWN_INT);
  } else {
    val = str_to_value (MS2_UNKNOWN_STR);
  }

  return val;
}

/* Returns an array of properties values suitable to send as dbus reply */
static GPtrArray *
get_array_properties (GHashTable *properties,
                      const gchar **filter)
{
  GPtrArray *prop_array;
  gint i;
  GValue *val;

  prop_array = g_ptr_array_sized_new (g_strv_length ((gchar **) filter));
  for (i = 0; filter[i]; i++) {
    if (properties) {
      val = g_hash_table_lookup (properties, filter[i]);
      if (val) {
        g_ptr_array_add (prop_array, val);
      } else {
        val = get_unknown_value (filter[i]);
        g_hash_table_insert (properties, (gchar *) filter[i], val);
        g_ptr_array_add (prop_array, val);
      }
    } else {
      val = get_unknown_value (filter[i]);
      g_ptr_array_add (prop_array, val);
    }
  }

  return prop_array;
}

/* Returns an array of children, which consist of arrays of properties, suitable
   to send as dbus reply */
static GPtrArray *
get_array_children (GList *children,
                    const gchar **filter)
{
  GList *child;
  GPtrArray *prop_array;
  GPtrArray *children_array;

  children_array = g_ptr_array_sized_new (g_list_length (children));
  for (child = children; child; child = g_list_next (child)) {
    prop_array = get_array_properties (child->data, filter);
    g_ptr_array_add (children_array, prop_array);
  }

  return children_array;
}

/* Looks up for wrong keys, and report them */
static const gchar *
check_properties (const gchar **filter)
{
  const gchar **p;

  for (p = filter; *p; p++) {
    if (g_strcmp0 (*p, MS2_PROP_ID) != 0 &&
        g_strcmp0 (*p, MS2_PROP_PARENT) != 0 &&
        g_strcmp0 (*p, MS2_PROP_DISPLAY_NAME) != 0 &&
        g_strcmp0 (*p, MS2_PROP_TYPE) != 0 &&
        g_strcmp0 (*p, MS2_PROP_CHILD_COUNT) != 0 &&
        g_strcmp0 (*p, MS2_PROP_ICON) != 0 &&
        g_strcmp0 (*p, MS2_PROP_URLS) != 0 &&
        g_strcmp0 (*p, MS2_PROP_MIME_TYPE) != 0 &&
        g_strcmp0 (*p, MS2_PROP_SIZE) != 0 &&
        g_strcmp0 (*p, MS2_PROP_ARTIST) != 0 &&
        g_strcmp0 (*p, MS2_PROP_ALBUM) != 0 &&
        g_strcmp0 (*p, MS2_PROP_DATE) != 0 &&
        g_strcmp0 (*p, MS2_PROP_DLNA_PROFILE) != 0 &&
        g_strcmp0 (*p, MS2_PROP_DURATION) != 0 &&
        g_strcmp0 (*p, MS2_PROP_BITRATE) != 0 &&
        g_strcmp0 (*p, MS2_PROP_SAMPLE_RATE) != 0 &&
        g_strcmp0 (*p, MS2_PROP_BITS_PER_SAMPLE) != 0 &&
        g_strcmp0 (*p, MS2_PROP_WIDTH) != 0 &&
        g_strcmp0 (*p, MS2_PROP_HEIGHT) != 0 &&
        g_strcmp0 (*p, MS2_PROP_COLOR_DEPTH) != 0 &&
        g_strcmp0 (*p, MS2_PROP_PIXEL_WIDTH) != 0 &&
        g_strcmp0 (*p, MS2_PROP_PIXEL_HEIGHT) != 0 &&
        g_strcmp0 (*p, MS2_PROP_THUMBNAIL) != 0 &&
        g_strcmp0 (*p, MS2_PROP_GENRE) != 0) {
      return *p;
    }
  }

  return NULL;
}

/* Registers the MS2Server object in dbus */
static gboolean
ms2_server_dbus_register (MS2Server *server,
                          const gchar *name)
{
  DBusGConnection *connection;
  DBusGProxy *gproxy;
  GError *error = NULL;
  gchar *dbus_name;
  gchar *dbus_path;
  guint request_name_result;

  connection = dbus_g_bus_get (DBUS_BUS_SESSION, &error);
  if (!connection) {
    g_printerr ("Could not connect to session bus, %s\n", error->message);
    g_error_free (error);
    return FALSE;
  }

  gproxy = dbus_g_proxy_new_for_name (connection,
                                      DBUS_SERVICE_DBUS,
                                      DBUS_PATH_DBUS,
                                      DBUS_INTERFACE_DBUS);

  /* Request name */
  dbus_name = g_strconcat (MS2_DBUS_SERVICE_PREFIX, name, NULL);
  if (!org_freedesktop_DBus_request_name (gproxy,
                                          dbus_name,
                                          DBUS_NAME_FLAG_DO_NOT_QUEUE,
                                          &request_name_result,
                                          NULL))  {
    g_free (dbus_name);
    return FALSE;
  }
  g_free (dbus_name);
  g_object_unref (gproxy);

  dbus_path = g_strconcat (MS2_DBUS_PATH_PREFIX, name, NULL);

  /* Register object */
  dbus_g_connection_register_g_object (connection,
                                       dbus_path,
                                       G_OBJECT (server));
  g_free (dbus_path);

  return TRUE;
}

static void
ms2_server_dbus_unregister (MS2Server *server,
                            const gchar *name)
{
  DBusGConnection *connection;
  DBusGProxy *gproxy;
  GError *error = NULL;
  gchar *dbus_name;
  guint request_name_result;

  connection = dbus_g_bus_get (DBUS_BUS_SESSION, &error);
  if (!connection) {
    g_printerr ("Could not connect to session bus, %s\n", error->message);
    g_error_free (error);
    return;
  }

  gproxy = dbus_g_proxy_new_for_name (connection,
                                      DBUS_SERVICE_DBUS,
                                      DBUS_PATH_DBUS,
                                      DBUS_INTERFACE_DBUS);

  /* Release name */
  dbus_name = g_strconcat (MS2_DBUS_SERVICE_PREFIX, name, NULL);
  org_freedesktop_DBus_release_name (gproxy,
                                     dbus_name,
                                     &request_name_result,
                                     NULL);
  g_free (dbus_name);
  g_object_unref (gproxy);
}

static void
ms2_server_finalize (GObject *object)
{
  MS2Server *server = MS2_SERVER (object);

  ms2_server_dbus_unregister (server, server->priv->name);
  g_free (server->priv->name);

  G_OBJECT_CLASS (ms2_server_parent_class)->finalize (object);
}

/* Class init function */
static void
ms2_server_class_init (MS2ServerClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MS2ServerPrivate));

  gobject_class->finalize = ms2_server_finalize;

  signals[UPDATED] = g_signal_new ("updated",
                                   G_TYPE_FROM_CLASS (klass),
                                   G_SIGNAL_RUN_FIRST | G_SIGNAL_NO_RECURSE,
                                   G_STRUCT_OFFSET (MS2ServerClass, updated),
                                   NULL,
                                   NULL,
                                   g_cclosure_marshal_VOID__STRING,
                                   G_TYPE_NONE,
                                   1,
                                   G_TYPE_STRING);

  /* Register introspection */
  dbus_g_object_type_install_info (MS2_TYPE_SERVER,
                                   &dbus_glib_ms2_server_object_info);
}

/* Object init function */
static void
ms2_server_init (MS2Server *server)
{
  server->priv = MS2_SERVER_GET_PRIVATE (server);
}

/****************** INTERNAL PUBLIC API (NOT TO BE EXPORTED) ******************/

/*
 * ms2_server_get_properties:
 * @server: a #MS2Server
 * @id: media identifier to obtain properties from
 * @filter: @NULL-terminated array of requested properties
 * @context: DBus context to send the reply through
 * @error: a #GError location to store the error ocurring, or @NULL to ignore
 *
 * Sends a #GPtrArray of properties values through DBus.
 *
 * Returns: @TRUE if success
 */
gboolean
ms2_server_get_properties (MS2Server *server,
                           const gchar *id,
                           const gchar **filter,
                           DBusGMethodInvocation *context,
                           GError **error)
{
  GError *prop_error = NULL;
  GError *send_error = NULL;
  GHashTable *properties = NULL;
  GPtrArray *prop_array = NULL;
  const gchar *wrong_prop;

  /* Check if peer has defined a function to retrieve properties */
  if (!server->priv->get_properties) {
    send_error = g_error_new_literal (MS2_ERROR,
                                      MS2_ERROR_GENERAL,
                                      "Unable to get properties");
  } else {
    /* Validate filter */
    wrong_prop = check_properties (filter);

    if (!wrong_prop) {
      properties = server->priv->get_properties (id,
                                                 filter,
                                                 server->priv->data,
                                                 &prop_error);
      if (prop_error) {
        send_error = g_error_new_literal (MS2_ERROR,
                                          MS2_ERROR_GENERAL,
                                          prop_error->message);
        g_error_free (prop_error);
      }
    } else {
      send_error = g_error_new (MS2_ERROR,
                                MS2_ERROR_GENERAL,
                                "Wrong property \"%s\"",
                                wrong_prop);
    }
  }

  /* Check if there was an error */
  if (send_error) {
    if (error) {
      *error = g_error_copy (send_error);
    }

    dbus_g_method_return_error (context, send_error);
    g_error_free (send_error);
    return TRUE;
  }

  /* Convert properties table in a type suitable to send through DBus (in this
     case, in a gptrarray of gvalues) */
  prop_array = get_array_properties (properties, filter);
  dbus_g_method_return (context, prop_array);

  /* Free content */
  if (properties) {
    g_hash_table_unref (properties);
  } else {
    g_ptr_array_foreach (prop_array, (GFunc) free_value, NULL);
  }
  g_ptr_array_free (prop_array, TRUE);

  return TRUE;
}

/*
 * ms2_server_get_children:
 * @server: a #MS2Server
 * @id: container identifier to get children from
 * @offset: number of children to skip in the result
 * @max_count: maximum number of children to return, or -1 for no limit
 * @filter: @NULL-terminated array of requested properties
 * @context: DBus context to send the reply through
 * @error: a #GError location to store the error ocurring, or @NULL to ignore
 *
 * Sends a #GPtrArray of children through DBus. Each child is in its turn a
 * #GPtrArray of properties values.
 *
 * Returns: @TRUE if success
 */
gboolean
ms2_server_get_children (MS2Server *server,
                         const gchar *id,
                         guint offset,
                         gint max_count,
                         const gchar **filter,
                         DBusGMethodInvocation *context,
                         GError **error)
{
  GError *children_error = NULL;
  GError *send_error = NULL;
  GPtrArray *children_array = NULL;
  GList *children = NULL;
  const gchar *wrong_prop;

  /* Check if peer has defined a function to retrieve children */
  if (!server->priv->get_children) {
    send_error = g_error_new_literal (MS2_ERROR,
                                      MS2_ERROR_GENERAL,
                                      "Unable to get children");
  } else {
    /* Validate filter */
    wrong_prop = check_properties (filter);

    if (!wrong_prop) {
      children =
        server->priv->get_children (id,
                                    offset,
                                    max_count < 0? G_MAXINT: max_count,
                                    filter,
                                    server->priv->data,
                                    &children_error);
      if (children_error) {
        send_error = g_error_new_literal (MS2_ERROR,
                                          MS2_ERROR_GENERAL,
                                          children_error->message);
        g_error_free (children_error);
      }
    } else {
      send_error = g_error_new (MS2_ERROR,
                                MS2_ERROR_GENERAL,
                                "Wrong property \"%s\"",
                                wrong_prop);
    }
  }

  /* Check if there was an error */
  if (send_error) {
    if (error) {
      *error = g_error_copy (send_error);
    }

    dbus_g_method_return_error (context, send_error);
    g_error_free (send_error);
    return TRUE;
  }

  /* Convert children list in a type suitable to send through DBUS (in this
     case, in a gptrarray of gptrarray o gvalues) */
  children_array = get_array_children (children, filter);
  dbus_g_method_return (context, children_array);

  /* Free content */
  g_ptr_array_free (children_array, TRUE);
  g_list_foreach (children, (GFunc) g_hash_table_unref, NULL);
  g_list_free (children);

  return TRUE;
}

/********************* PUBLIC API *********************/

/********** SERVER API **********/

/**
 * ms2_server_new:
 * @name: the name used when registered in DBus
 * @data: user defined data
 *
 * Creates a new #MS2Server that will be registered in DBus under name
 * "org.gnome.UPnP.MediaServer2.<name>".
 *
 * @data will be used as parameter when invoking the functions to retrieve
 * properties or children.
 *
 * Returns: a new #MS2Server registed in DBus, or @NULL if fails
 **/
MS2Server *
ms2_server_new (const gchar *name,
                gpointer data)
{
  MS2Server *server;

  g_return_val_if_fail (name, NULL);

  server = g_object_new (MS2_TYPE_SERVER, NULL);

  server->priv->data = data;
  server->priv->name = g_strdup (name);

  /* Register object in DBus */
  if (!ms2_server_dbus_register (server, name)) {
    g_object_unref (server);
    return NULL;
  } else {
    return server;
  }
}

/**
 * ms2_server_set_get_properties_func:
 * @server: a #MS2Server
 * @get_properties_func: user-defined function to request properties
 *
 * Defines which function must be used when requesting properties.
 **/
void
ms2_server_set_get_properties_func (MS2Server *server,
                                    GetPropertiesFunc get_properties_func)
{
  g_return_if_fail (MS2_IS_SERVER (server));

  server->priv->get_properties = get_properties_func;
}

/**
 * ms2_server_set_get_children_func:
 * @server: a #MS2Server
 * @get_children_func: user-defined function to request children
 *
 * Defines which function must be used when requesting children.
 **/
void
ms2_server_set_get_children_func (MS2Server *server,
                                  GetChildrenFunc get_children_func)
{
  g_return_if_fail (MS2_IS_SERVER (server));

  server->priv->get_children = get_children_func;
}

/**
 * ms2_server_updated:
 * @server: a #MS2Server
 * @id: item identifier that has changed
 *
 * Emit a signal notifying an item has changed.
 *
 * Which shall be triggered when a new child item is created or removed from
 * container, or one of the existing child items is modified, or any of the
 * properties of the container itself are modified. While the signal should be
 * emitted when child containers are created or removed, it shall not be emitted
 * when child containers are modified: instead the signal should be emitted on
 * the child in this case. It up to client to follow these rules when invoking
 * this method
 **/
void
ms2_server_updated (MS2Server *server,
                    const gchar *id)
{
  g_return_if_fail (MS2_IS_SERVER (server));

  g_signal_emit (server, signals[UPDATED], 0, id);
}

/********** PROPERTIES TABLE API **********/

/**
 * ms2_server_new_properties_hashtable:
 * @id: identifier of item which properties will be stored in the table
 *
 * Creates a new #GHashTable suitable to store items properties.
 *
 * For root container, identifier should be "0".
 *
 * Returns: a new #GHashTable
 **/
GHashTable *
ms2_server_new_properties_hashtable (const gchar *id)
{
  GHashTable *properties;

  properties = g_hash_table_new_full (g_str_hash,
                                      g_str_equal,
                                      NULL,
                                      (GDestroyNotify) free_value);
  if (id) {
    g_hash_table_insert (properties, MS2_PROP_ID, str_to_value (id));
  }

  return properties;
}

/**
 * ms2_server_set_parent:
 * @properties: a #GHashTable
 * @parent: parent value
 *
 * Sets the "parent" property. Mandatory property.
 **/
void
ms2_server_set_parent (GHashTable *properties,
                       const gchar *parent)
{
  g_return_if_fail (properties);

  if (parent) {
    g_hash_table_insert (properties,
                         MS2_PROP_PARENT,
                         str_to_value (parent));
  }
}

/**
 * ms2_server_set_display_name:
 * @properties: a #GHashTable
 * @display_name: display name value
 *
 * Sets the "display-name" property. Mandatory property.
 **/
void
ms2_server_set_display_name (GHashTable *properties,
                             const gchar *display_name)
{
  g_return_if_fail (properties);

  if (display_name) {
    g_hash_table_insert (properties,
                         MS2_PROP_DISPLAY_NAME,
                         str_to_value (display_name));
  }
}

/**
 * ms2_server_set_item_type:
 * @properties: a #GHashTable
 * @type: type of item
 *
 * Sets the "type" property. Mandatory property.
 *
 * Tells what kind of object we are dealing with.
 **/
void
ms2_server_set_item_type (GHashTable *properties,
                          MS2ItemType type)
{
  g_return_if_fail (properties);

  switch (type) {
  case MS2_ITEM_TYPE_UNKNOWN:
    /* Do not handle unknown values */
    break;
  case MS2_ITEM_TYPE_CONTAINER:
    g_hash_table_insert (properties,
                         MS2_PROP_TYPE,
                         str_to_value (MS2_TYPE_CONTAINER));
    break;
  case MS2_ITEM_TYPE_VIDEO:
    g_hash_table_insert (properties,
                         MS2_PROP_TYPE,
                         str_to_value (MS2_TYPE_VIDEO));
    break;
  case MS2_ITEM_TYPE_MOVIE:
    g_hash_table_insert (properties,
                         MS2_PROP_TYPE,
                         str_to_value (MS2_TYPE_MOVIE));
    break;
  case MS2_ITEM_TYPE_AUDIO:
    g_hash_table_insert (properties,
                         MS2_PROP_TYPE,
                         str_to_value (MS2_TYPE_AUDIO));
    break;
  case MS2_ITEM_TYPE_MUSIC:
    g_hash_table_insert (properties,
                         MS2_PROP_TYPE,
                         str_to_value (MS2_TYPE_MUSIC));
    break;
  case MS2_ITEM_TYPE_IMAGE:
    g_hash_table_insert (properties,
                         MS2_PROP_TYPE,
                         str_to_value (MS2_TYPE_IMAGE));
    break;
  case MS2_ITEM_TYPE_PHOTO:
    g_hash_table_insert (properties,
                         MS2_PROP_TYPE,
                         str_to_value (MS2_TYPE_PHOTO));
    break;
  }
}

/**
 * ms2_server_set_icon:
 * @properties: a #GHashTable
 * @icon: icon identifier value
 *
 * Sets the "icon" property. Recommended property for containers.
 *
 * Use this to provide an icon to be used by consumer UIs to represent the
 * provider. This is only relevant to root container.
 **/
void
ms2_server_set_icon (GHashTable *properties,
                     const gchar *icon)
{
  g_return_if_fail (properties);

  if (icon) {
    g_hash_table_insert (properties,
                         MS2_PROP_ICON,
                         str_to_value (icon));
  }
}

/**
 * ms2_server_set_mime_type:
 * @properties: a #GHashTable
 * @mime_type: mime type value
 *
 * Sets the "mime-type" property. Mandatory property for items.
 **/
void
ms2_server_set_mime_type (GHashTable *properties,
                          const gchar *mime_type)
{
  g_return_if_fail (properties);

  if (mime_type) {
    g_hash_table_insert (properties,
                         MS2_PROP_MIME_TYPE,
                         str_to_value (mime_type));
  }
}

/**
 * ms2_server_set_artist:
 * @properties: a #GHashTable
 * @artist: artist value
 *
 * Sets the "artist" property. Recommended property for items.
 **/
void
ms2_server_set_artist (GHashTable *properties,
                       const gchar *artist)
{
  g_return_if_fail (properties);

  if (artist) {
    g_hash_table_insert (properties,
                         MS2_PROP_ARTIST,
                         str_to_value (artist));
  }
}

/**
 * ms2_server_set_album:
 * @properties: a #GHashTable
 * @album: album value
 *
 * Sets the "album" property. Recommended property for items.
 **/
void
ms2_server_set_album (GHashTable *properties,
                      const gchar *album)
{
  g_return_if_fail (properties);

  if (album) {
    g_hash_table_insert (properties,
                         MS2_PROP_ALBUM,
                         str_to_value (album));
  }
}

/**
 * ms2_server_set_date:
 * @properties: a #GHashTable
 * @date: date value
 *
 * Sets the "date" property. Recommended property for items.
 *
 * This date can be date of creation or release. Must be compliant to ISO-8601
 * and RFC-3339.
 **/
void
ms2_server_set_date (GHashTable *properties,
                     const gchar *date)
{
  g_return_if_fail (properties);

  if (date) {
    g_hash_table_insert (properties,
                         MS2_PROP_ALBUM,
                         str_to_value (date));
  }
}

/**
 * ms2_server_set_dlna_profile:
 * @properties: a #GHashTable
 * @dlna_profile: DLNA value
 *
 * Sets the "dlna-profile" property. Optional property for items.
 *
 * If you provide a value for this property, it will greatly help avoiding
 * guessing of its value by UPnP consumers.
 **/
void
ms2_server_set_dlna_profile (GHashTable *properties,
                             const gchar *dlna_profile)
{
  g_return_if_fail (properties);

  if (dlna_profile) {
    g_hash_table_insert (properties,
                         MS2_PROP_DLNA_PROFILE,
                         str_to_value (dlna_profile));
  }
}

/**
 * ms2_server_set_thumbnail:
 * @properties: a #GHashTable
 * @thumbnail: thumbnail identifier value
 *
 * Sets the "thumbnail" property. Optional property for video/image items.
 **/
void
ms2_server_set_thumbnail (GHashTable *properties,
                          const gchar *thumbnail)
{
  g_return_if_fail (properties);

  if (thumbnail) {
    g_hash_table_insert (properties,
                         MS2_PROP_THUMBNAIL,
                         str_to_value (thumbnail));
  }
}

/**
 * ms2_server_set_genre:
 * @properties: a #GHashTable
 * @genre: genre value
 *
 * Sets the "genre" property. Optional property for audio/music items.
 **/
void
ms2_server_set_genre (GHashTable *properties,
                      const gchar *genre)
{
  g_return_if_fail (properties);

  if (genre) {
    g_hash_table_insert (properties,
                         MS2_PROP_GENRE,
                         str_to_value (genre));
  }
}

/**
 * ms2_server_set_child_count:
 * @properties: a #GHashTable
 * @child_count: childcount value
 *
 * Sets the "child-count" property. Recommended property for containers.
 *
 * It is the number of media objects directly under this container.
 **/
void
ms2_server_set_child_count (GHashTable *properties,
                            gint child_count)
{
  g_return_if_fail (properties);

  g_hash_table_insert (properties,
                       MS2_PROP_CHILD_COUNT,
                       int_to_value (child_count));
}

/**
 * ms2_server_set_size:
 * @properties: a #GHashTable
 * @size: size value
 *
 * Sets the "size" property. Recommended property for items.
 *
 * It is the resource size in bytes.
 **/
void
ms2_server_set_size (GHashTable *properties,
                     gint size)
{
  g_return_if_fail (properties);

  g_hash_table_insert (properties,
                       MS2_PROP_SIZE,
                       int_to_value (size));
}

/**
 * ms2_server_set_duration:
 * @properties: a #GHashTable
 * @duration: duration (in seconds) value
 *
 * Sets the "duration" property. Optional property for audio/video/music items.
 **/
void
ms2_server_set_duration (GHashTable *properties,
                         gint duration)
{
  g_return_if_fail (properties);

  g_hash_table_insert (properties,
                       MS2_PROP_DURATION,
                       int_to_value (duration));
}

/**
 * ms2_server_set_bitrate:
 * @properties: a #GHashTable
 * @bitrate: bitrate value
 *
 * Sets the "bitrate" property. Optional property for audio/video/music items.
 **/
void
ms2_server_set_bitrate (GHashTable *properties,
                        gint bitrate)
{
  g_return_if_fail (properties);

  g_hash_table_insert (properties,
                       MS2_PROP_BITRATE,
                       int_to_value (bitrate));
}

/**
 * ms2_server_set_sample_rate:
 * @properties: a #GHashTable
 * @sample_rate: sample rate value
 *
 * Sets the "sample-rate" property. Optional property for audio/video/music
 * items.
 **/
void
ms2_server_set_sample_rate (GHashTable *properties,
                            gint sample_rate)
{
  g_return_if_fail (properties);

  g_hash_table_insert (properties,
                       MS2_PROP_SAMPLE_RATE,
                       int_to_value (sample_rate));
}

/**
 * ms2_server_set_bits_per_sample:
 * @properties: a #GHashTable
 * @bits_per_sample: bits per sample value
 *
 * Sets the "bits-per-sample" property. Optional property for audio/video/music
 * items.
 **/
void
ms2_server_set_bits_per_sample (GHashTable *properties,
                                gint bits_per_sample)
{
  g_return_if_fail (properties);

  g_hash_table_insert (properties,
                       MS2_PROP_BITS_PER_SAMPLE,
                       int_to_value (bits_per_sample));
}

/**
 * ms2_server_set_width:
 * @properties: a #GHashTable
 * @width: width (in pixels) value
 *
 * Sets the "width" property. Recommended property for video/image items.
 **/
void
ms2_server_set_width (GHashTable *properties,
                      gint width)
{
  g_return_if_fail (properties);

  g_hash_table_insert (properties,
                       MS2_PROP_WIDTH,
                       int_to_value (width));
}

/**
 * ms2_server_set_height:
 * @properties: a #GHashTable
 * @height: height (in pixels) value
 *
 * Sets the "height" property. Recommended property for video/image items.
 **/
void
ms2_server_set_height (GHashTable *properties,
                       gint height)
{
  g_return_if_fail (properties);

  g_hash_table_insert (properties,
                       MS2_PROP_HEIGHT,
                       int_to_value (height));
}

/**
 * ms2_server_set_color_depth:
 * @properties: a #GHashTable
 * @depth: color depth value
 *
 * Sets the "color-depth" property. Recommended property for video/image items.
 **/
void
ms2_server_set_color_depth (GHashTable *properties,
                            gint depth)
{
  g_return_if_fail (properties);

  g_hash_table_insert (properties,
                       MS2_PROP_COLOR_DEPTH,
                       int_to_value (depth));
}

/**
 * ms2_server_set_pixel_width:
 * @properties: a #GHashTable
 * @pixel_width: pixel width value
 *
 * Sets the "pixel-width" property. Optional property for video/image items.
 **/
void
ms2_server_set_pixel_width (GHashTable *properties,
                            gint pixel_width)
{
  g_return_if_fail (properties);

  g_hash_table_insert (properties,
                       MS2_PROP_PIXEL_WIDTH,
                       int_to_value (pixel_width));
}

/**
 * ms2_server_set_pixel_height:
 * @properties: a #GHashTable
 * @pixel_height: pixel height value
 *
 * Sets the "pixel-height" property. Optional property for video/image items.
 **/
void
ms2_server_set_pixel_height (GHashTable *properties,
                             gint pixel_height)
{
  g_return_if_fail (properties);

  g_hash_table_insert (properties,
                       MS2_PROP_PIXEL_HEIGHT,
                       int_to_value (pixel_height));
}

/**
 * ms2_server_set_urls:
 * @properties: a #GHashTable
 * @urls: @NULL-terminated array of URLs values
 *
 * Sets the "URLs" property. Mandatory property for items.
 **/
void
ms2_server_set_urls (GHashTable *properties,
                     gchar **urls)
{
  GPtrArray *url_array;
  gint i;

  g_return_if_fail (properties);

  /* Check if there is at least one URL */
  if (urls && urls[0]) {
    url_array = g_ptr_array_sized_new (g_strv_length (urls));
    for (i = 0; urls[i]; i++) {
      g_ptr_array_add (url_array, g_strdup (urls[i]));
    }

    g_hash_table_insert (properties,
                         MS2_PROP_URLS,
                         ptrarray_to_value (url_array));
  }
}
