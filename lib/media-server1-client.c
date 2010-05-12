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

#include "media-server1-private.h"
#include "media-server1-client.h"

#define IMEDIAOBJECT1_INDEX 0
#define IMEDIAITEM1_INDEX   1

#define MS1_CLIENT_GET_PRIVATE(o)                                       \
  G_TYPE_INSTANCE_GET_PRIVATE((o), MS1_TYPE_CLIENT, MS1ClientPrivate)

enum {
  UPDATED,
  DESTROY,
  LAST_SIGNAL
};

/*
 * Private MS1Client structure
 *   bus: connection to DBus session
 *   name: name of provider
 *   fullname: full dbus service name of provider
 *   root_path: object path to reach root category
 */
struct _MS1ClientPrivate {
  DBusGConnection *bus;
  gchar *name;
  gchar *fullname;
  gchar *root_path;
};

static guint32 signals[LAST_SIGNAL] = { 0 };

static gchar *IFACES[] = { "org.gnome.UPnP.MediaObject1",
                           "org.gnome.UPnP.MediaItem1" };

G_DEFINE_TYPE (MS1Client, ms1_client, G_TYPE_OBJECT);

/******************** PRIVATE API ********************/

#if 0
/* Callback invoked when "Updated" dbus signal is received */
static void
updated (DBusGProxy *proxy,
         const gchar *id,
         MS1Client *client)
{
  g_signal_emit (client, signals[UPDATED], 0, id);
}
#endif

/* Free gvalue */
static void
free_gvalue (GValue *v)
{
  g_value_unset (v);
  g_free (v);
}

static gboolean
collect_value (gpointer key,
               gpointer value,
               GHashTable *collection)
{
  g_hash_table_insert (collection, key, value);
  return TRUE;
}

static gchar ***
split_properties_by_interface (gchar **properties)
{
  gchar ***split;
  gint prop_length;
  gint mo_index = 0;
  gint mi_index = 0;
  gchar **property;

  prop_length = g_strv_length (properties) + 1;
  split = g_new (gchar **, 2);
  split[IMEDIAOBJECT1_INDEX] = g_new0 (gchar *, prop_length);
  split[IMEDIAITEM1_INDEX] = g_new0 (gchar *, prop_length);
  for (property = properties; *property; property++) {
    if (g_strcmp0 (*property, MS1_PROP_DISPLAY_NAME) == 0 ||
        g_strcmp0 (*property, MS1_PROP_PARENT) == 0 ||
        g_strcmp0 (*property, MS1_PROP_PATH) == 0) {
      split[IMEDIAOBJECT1_INDEX][mo_index++] = *property;
    } else {
      split[IMEDIAITEM1_INDEX][mi_index++] = *property;
    }
  }

  return split;
}

/* Converts GPtrArray in a GList */
static GList *
gptrarray_to_glist (GPtrArray *result)
{
  GList *list = NULL;
  gint i;

  if (!result) {
    return NULL;
  }

  for (i = 0; i < result->len; i++) {
    list = g_list_prepend (list, g_ptr_array_index (result, i));
  }

  return g_list_reverse (list);
}

/* Dispose function */
static void
ms1_client_dispose (GObject *object)
{
  MS1Client *client = MS1_CLIENT (object);

  ms1_observer_remove_client (client, client->priv->name);

  G_OBJECT_CLASS (ms1_client_parent_class)->dispose (object);
}

static void
ms1_client_finalize (GObject *object)
{
  MS1Client *client = MS1_CLIENT (object);

  g_free (client->priv->name);
  g_free (client->priv->fullname);
  g_free (client->priv->root_path);

  G_OBJECT_CLASS (ms1_client_parent_class)->finalize (object);
}

/* Class init function */
static void
ms1_client_class_init (MS1ClientClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MS1ClientPrivate));

  gobject_class->dispose = ms1_client_dispose;
  gobject_class->finalize = ms1_client_finalize;

  /**
   * MS1Client::updated:
   * @client: a #MS1Client
   * @id: identifier of item that has changed
   *
   * Notifies when an item in provider has changed.
   **/
  signals[UPDATED] = g_signal_new ("updated",
                                   G_TYPE_FROM_CLASS (klass),
                                   G_SIGNAL_RUN_FIRST | G_SIGNAL_NO_RECURSE,
                                   G_STRUCT_OFFSET (MS1ClientClass, updated),
                                   NULL,
                                   NULL,
                                   g_cclosure_marshal_VOID__STRING,
                                   G_TYPE_NONE,
                                   1,
                                   G_TYPE_STRING);

  /**
   * MS1Client::destroy:
   * @client: a #MS1Client
   *
   * Notifies when a client is going to be destroyed. Usually this happens when
   * provider goes away.
   *
   * User should not use the object, as provider is not present.
   *
   * After this, client will be automatically destroyed.
   **/
  signals[DESTROY] = g_signal_new ("destroy",
                                   G_TYPE_FROM_CLASS (klass),
                                   G_SIGNAL_RUN_LAST | G_SIGNAL_RUN_CLEANUP,
                                   G_STRUCT_OFFSET (MS1ClientClass, destroy),
                                   NULL,
                                   NULL,
                                   g_cclosure_marshal_VOID__VOID,
                                   G_TYPE_NONE,
                                   0);
}

/* Object init function */
static void
ms1_client_init (MS1Client *client)
{
  client->priv = MS1_CLIENT_GET_PRIVATE (client);
}

/****************** INTERNAL PUBLIC API (NOT TO BE EXPORTED) ******************/

/* Notify destruction of client, and unref it */
void
ms1_client_notify_unref (MS1Client *client)
{
  g_return_if_fail (MS1_IS_CLIENT (client));

  g_signal_emit (client, signals[DESTROY], 0);
  g_object_unref (client);
}

/******************** PUBLIC API ********************/

/**
 * ms1_client_get_providers:
 *
 * Returns a list of content providers following MediaServer1 specification.
 *
 * Returns: a new @NULL-terminated array of strings
 **/
gchar **
ms1_client_get_providers ()
{
  DBusGConnection *connection;
  DBusGProxy *gproxy;
  GError *error = NULL;
  GPtrArray *providers;
  gchar **dbus_names;
  gchar **list_providers;
  gchar **p;
  gint i;
  gint prefix_size = strlen (MS1_DBUS_SERVICE_PREFIX);

  connection = dbus_g_bus_get (DBUS_BUS_SESSION, &error);
  if (!connection) {
    g_printerr ("Could not connect to session bus, %s\n", error->message);
    g_clear_error (&error);
    return FALSE;
  }

  gproxy = dbus_g_proxy_new_for_name (connection,
                                      DBUS_SERVICE_DBUS,
                                      DBUS_PATH_DBUS,
                                      DBUS_INTERFACE_DBUS);

  /* Get a list of all DBUS services running */
  org_freedesktop_DBus_list_names (gproxy, &dbus_names, &error);
  g_object_unref (gproxy);

  if (error) {
    g_printerr ("Could not get list of dbus names, %s\n", error->message);
    g_clear_error (&error);
    return FALSE;
  }

  if (!dbus_names) {
    return FALSE;
  }

  /* Filter the list to obtain those services that fulfils MediaServer1 spec */
  providers = g_ptr_array_new ();
  for (p = dbus_names; *p; p++) {
    if (g_str_has_prefix (*p, MS1_DBUS_SERVICE_PREFIX)) {
      g_ptr_array_add (providers, *p);
    }
  }

  /* Put them in a NULL-terminated array */
  list_providers = g_new (gchar *, providers->len + 1);
  for (i = 0; i < providers->len; i++) {
    list_providers[i] =
      g_strdup (g_ptr_array_index (providers, i) + prefix_size);
  }

  list_providers[i] = NULL;

  g_strfreev (dbus_names);
  g_ptr_array_free (providers, TRUE);

  return list_providers;
}

/**
 * ms1_client_new:
 * @provider: provider name.
 *
 * Create a new #MS1Client that will be used to obtain content from the provider
 * specified.
 *
 * Providers can be obtained with ms1_client_get_providers().
 *
 * Returns: a new #MS1Client
 **/
MS1Client *ms1_client_new (const gchar *provider)
{
  DBusGConnection *connection;
  GError *error = NULL;
  MS1Client *client;

  connection = dbus_g_bus_get (DBUS_BUS_SESSION, &error);
  if (!connection) {
    g_printerr ("Could not connect to session bus, %s\n", error->message);
    g_clear_error (&error);
    return NULL;
  }

  client = g_object_new (MS1_TYPE_CLIENT, NULL);
  client->priv->bus = connection;
  client->priv->name = g_strdup (provider);
  client->priv->fullname = g_strconcat (MS1_DBUS_SERVICE_PREFIX, provider, NULL);
  client->priv->root_path = g_strconcat (MS1_DBUS_PATH_PREFIX, provider, NULL);

  ms1_observer_add_client (client, provider);

  return client;
}

/**
 * ms1_client_get_provider_name:
 * @client: a #MS1Client
 *
 * Returns name of provider which client is attending
 *
 * Returns: name of provider
 **/
const gchar *
ms1_client_get_provider_name (MS1Client *client)
{
  g_return_val_if_fail (MS1_IS_CLIENT (client), NULL);

  return client->priv->name;
}

/**
 * ms1_client_get_properties:
 * @client: a #MS1Client
 * @object_path: media identifier to obtain properties from
 * @properties: @NULL-terminated array of properties to request
 * @error: a #GError location to store the error ocurring, or @NULL to ignore
 *
 * Gets the properties of media id. Properties will be returned in a hash table
 * of <prop_id, prop_gvalue> pairs.
 *
 * Returns: a new #GHashTable
 **/
GHashTable *
ms1_client_get_properties (MS1Client *client,
                           const gchar *object_path,
                           gchar **properties,
                           GError **error)
{
  DBusGProxy *gproxy;
  GHashTable *collected_properties;
  GHashTable *prop_result;
  GValue *v;
  gboolean error_happened = FALSE;
  gchar ***prop_by_iface;
  gint i;
  gint num_props;

  g_return_val_if_fail (MS1_IS_CLIENT (client), NULL);
  g_return_val_if_fail (properties, NULL);

  gproxy = dbus_g_proxy_new_for_name (client->priv->bus,
                                      client->priv->fullname,
                                      object_path,
                                      "org.freedesktop.DBus.Properties");

  collected_properties = g_hash_table_new_full (g_str_hash,
                                                g_str_equal,
                                                (GDestroyNotify) g_free,
                                                (GDestroyNotify) free_gvalue);

  prop_by_iface = split_properties_by_interface (properties);
  for (i = 0; i < 2; i++) {
    num_props = g_strv_length (prop_by_iface[i]);
    /* If only one property is required, then invoke "Get" method */
    if (num_props == 1) {
      v = g_new0 (GValue, 1);
      if (dbus_g_proxy_call (gproxy,
                             "Get", error,
                             G_TYPE_STRING, IFACES[i],
                             G_TYPE_STRING, prop_by_iface[i][0],
                             G_TYPE_INVALID,
                             G_TYPE_VALUE, &v,
                             G_TYPE_INVALID)) {
        g_hash_table_insert (collected_properties,
                             g_strdup (prop_by_iface[i][0]),
                             v);
      } else {
        error_happened = TRUE;
        break;
      }
    } else if (num_props > 1) {
      /* If several properties are required, use "GetAll" method */
      if (dbus_g_proxy_call (gproxy,
                             "GetAll", error,
                             G_TYPE_STRING, IFACES[i],
                             G_TYPE_INVALID,
                             dbus_g_type_get_map ("GHashTable",
                                                  G_TYPE_STRING,
                                                  G_TYPE_VALUE), &prop_result,
                            G_TYPE_INVALID)) {
        g_hash_table_foreach_steal (prop_result,
                                    (GHRFunc) collect_value,
                                    collected_properties);
        g_hash_table_unref (prop_result);
      } else {
        error_happened = TRUE;
        break;
      }
    }
  }

  g_object_unref (gproxy);

  g_free (prop_by_iface[0]);
  g_free (prop_by_iface[1]);
  g_free (prop_by_iface);

  if (error_happened) {
    if (collected_properties) {
      g_hash_table_unref (collected_properties);
    }
    return NULL;
  } else {
    return collected_properties;
  }
}

/**
 * ms1_client_list_children:
 * @client: a #MS1Client
 * @object_path: container identifier to get children from
 * @offset: number of children to skip
 * @max_count: maximum number of children to return, or 0 for no limit
 * @properties: @NULL-terminated array of properties to request for each child
 * @error: a #GError location to store the error ocurring, or @NULL to ignore
 *
 * Gets the list of children directly under the container id. Each child consist
 * of a hash table of <prop_id, prop_gvalue> pairs.
 *
 * Returns: a new #GList of #GHashTable. To free it, free first each element
 * (g_hash_table_unref()) and finally the list itself (g_list_free())
 **/
GList *
ms1_client_list_children (MS1Client *client,
                          const gchar *object_path,
                          guint offset,
                          guint max_count,
                          const gchar **properties,
                          GError **error)
{
  DBusGProxy *gproxy;
  GList *children = NULL;
  GPtrArray *result = NULL;

  g_return_val_if_fail (MS1_IS_CLIENT (client), NULL);
  g_return_val_if_fail (properties, NULL);

  gproxy = dbus_g_proxy_new_for_name (client->priv->bus,
                                      client->priv->fullname,
                                      object_path,
                                      "org.gnome.UPnP.MediaContainer1");

  if (dbus_g_proxy_call (gproxy,
                         "ListChildren", error,
                         G_TYPE_UINT, offset,
                         G_TYPE_UINT, max_count,
                         G_TYPE_STRV, properties,
                         G_TYPE_INVALID,
                         dbus_g_type_get_collection ("GPtrArray",
                                                     dbus_g_type_get_map ("GHashTable",
                                                                          G_TYPE_STRING,
                                                                          G_TYPE_VALUE)), &result,
                         G_TYPE_INVALID)) {
    children = gptrarray_to_glist (result);
    g_ptr_array_free (result, TRUE);
  }

  g_object_unref (gproxy);

  return children;
}

const gchar *
ms1_client_get_root_path (MS1Client *client)
{
  g_return_val_if_fail (MS1_IS_CLIENT (client), NULL);

  return client->priv->root_path;
}

/******************** PROPERTIES TABLE API ********************/

/**
 * ms1_client_get_id:
 * @properties: a #GHashTable
 *
 * Returns "id" property value.
 *
 * Returns: property value or @NULL if it is not available
 **/
const gchar *
ms1_client_get_path (GHashTable *properties)
{
  GValue *val;

  g_return_val_if_fail (properties, NULL);

  val = g_hash_table_lookup (properties, MS1_PROP_PATH);
  if (!val || !G_VALUE_HOLDS_STRING (val)) {
    return NULL;
  }

  return g_value_get_string (val);
}

/**
 * ms1_client_get_parent:
 * @properties: a #GHashTable
 *
 * Returns "parent" property value.
 *
 * Returns: property value or @NULL if it is not available
 **/
const gchar *
ms1_client_get_parent (GHashTable *properties)
{
  GValue *val;

  g_return_val_if_fail (properties, NULL);

  val = g_hash_table_lookup (properties, MS1_PROP_PARENT);
  if (!val || !G_VALUE_HOLDS_STRING (val)) {
    return NULL;
  }

  return g_value_get_string (val);
}

/**
 * ms1_client_get_display_name:
 * @properties: a #GHashTable
 *
 * Returns "display-name" property value.
 *
 * Returns: property value or @NULL if it is not available
 **/
const gchar *
ms1_client_get_display_name (GHashTable *properties)
{
  GValue *val;

  g_return_val_if_fail (properties, NULL);

  val = g_hash_table_lookup (properties, MS1_PROP_DISPLAY_NAME);
  if (!val || !G_VALUE_HOLDS_STRING (val)) {
    return NULL;
  }

  return g_value_get_string (val);
}

/**
 * ms1_client_get_item_type:
 * @properties: a #GHashTable
 *
 * Returns "type" property value.
 *
 * Returns: property value
 **/
MS1ItemType
ms1_client_get_item_type (GHashTable *properties)
{
  GValue *val;
  const gchar *type;

  g_return_val_if_fail (properties, MS1_ITEM_TYPE_UNKNOWN);

  val = g_hash_table_lookup (properties, MS1_PROP_TYPE);
  if (!val || !G_VALUE_HOLDS_STRING (val)) {
    return MS1_ITEM_TYPE_UNKNOWN;
  }

  type = g_value_get_string (val);

  if (g_strcmp0 (type, MS1_TYPE_CONTAINER) == 0) {
    return MS1_ITEM_TYPE_CONTAINER;
  }   if (g_strcmp0 (type, MS1_TYPE_CONTAINER) == 0) {
    return MS1_ITEM_TYPE_CONTAINER;
  } else if (g_strcmp0 (type, MS1_TYPE_ITEM) == 0) {
    return MS1_ITEM_TYPE_ITEM;
  } else if (g_strcmp0 (type, MS1_TYPE_MOVIE) == 0) {
    return MS1_ITEM_TYPE_MOVIE;
  } else if (g_strcmp0 (type, MS1_TYPE_AUDIO) == 0) {
    return MS1_ITEM_TYPE_AUDIO;
  } else if (g_strcmp0 (type, MS1_TYPE_MUSIC) == 0) {
    return MS1_ITEM_TYPE_MUSIC;
  } else if (g_strcmp0 (type, MS1_TYPE_IMAGE) == 0) {
    return MS1_ITEM_TYPE_IMAGE;
  } else if (g_strcmp0 (type, MS1_TYPE_PHOTO) == 0) {
    return MS1_ITEM_TYPE_PHOTO;
  } else {
    return MS1_ITEM_TYPE_UNKNOWN;
  }
}

/**
 * ms1_client_get_mime_type:
 * @properties: a #GHashTable
 *
 * Returns "mime-type" property value.
 *
 * Returns: property value or @NULL if it is not available
 **/
const gchar *
ms1_client_get_mime_type (GHashTable *properties)
{
  GValue *val;

  g_return_val_if_fail (properties, NULL);

  val = g_hash_table_lookup (properties, MS1_PROP_MIME_TYPE);
  if (!val || !G_VALUE_HOLDS_STRING (val)) {
    return NULL;
  }

  return g_value_get_string (val);
}

/**
 * ms1_client_get_artist:
 * @properties: a #GHashTable
 *
 * Returns "artist" property value.
 *
 * Returns: property value or @NULL if it is not available
 **/
const gchar *
ms1_client_get_artist (GHashTable *properties)
{
  GValue *val;

  g_return_val_if_fail (properties, NULL);

  val = g_hash_table_lookup (properties, MS1_PROP_ARTIST);
  if (!val || !G_VALUE_HOLDS_STRING (val)) {
    return NULL;
  }

  return g_value_get_string (val);
}

/**
 * ms1_client_get_album:
 * @properties: a #GHashTable
 *
 * Returns "album" property value.
 *
 * Returns: property value or @NULL if it is not available
 **/
const gchar *
ms1_client_get_album (GHashTable *properties)
{
  GValue *val;

  g_return_val_if_fail (properties, NULL);

  val = g_hash_table_lookup (properties, MS1_PROP_ALBUM);
  if (!val || !G_VALUE_HOLDS_STRING (val)) {
    return NULL;
  }

  return g_value_get_string (val);
}

/**
 * ms1_client_get_date:
 * @properties: a #GHashTable
 *
 * Returns "date" property value.
 *
 * Returns: property value or @NULL if it is not available
 **/
const gchar *
ms1_client_get_date (GHashTable *properties)
{
  GValue *val;

  g_return_val_if_fail (properties, NULL);

  val = g_hash_table_lookup (properties, MS1_PROP_DATE);
  if (!val || !G_VALUE_HOLDS_STRING (val)) {
    return NULL;
  }

  return g_value_get_string (val);
}

/**
 * ms1_client_get_dlna_profile:
 * @properties: a #GHashTable
 *
 * Returns "dlna-profile" property value.
 *
 * Returns: property value or @NULL if it is not available
 **/
const gchar *
ms1_client_get_dlna_profile (GHashTable *properties)
{
  GValue *val;

  g_return_val_if_fail (properties, NULL);

  val = g_hash_table_lookup (properties, MS1_PROP_DLNA_PROFILE);
  if (!val || !G_VALUE_HOLDS_STRING (val)) {
    return NULL;
  }

  return g_value_get_string (val);
}

/**
 * ms1_client_get_thumbnail:
 * @properties: a #GHashTable
 *
 * Returns "thumbanil" property value.
 *
 * Returns: property value or @NULL if it is not available
 **/
const gchar *
ms1_client_get_thumbnail (GHashTable *properties)
{
  GValue *val;

  g_return_val_if_fail (properties, NULL);

  val = g_hash_table_lookup (properties, MS1_PROP_THUMBNAIL);
  if (!val || !G_VALUE_HOLDS_STRING (val)) {
    return NULL;
  }

  return g_value_get_string (val);
}

/**
 * ms1_client_get_genre:
 * @properties: a #GHashTable
 *
 * Returns "genre" property value.
 *
 * Returns: property value or @NULL if it is not available
 **/
const gchar *
ms1_client_get_genre (GHashTable *properties)
{
  GValue *val;

  g_return_val_if_fail (properties, NULL);

  val = g_hash_table_lookup (properties, MS1_PROP_GENRE);
  if (!val || !G_VALUE_HOLDS_STRING (val)) {
    return NULL;
  }

  return g_value_get_string (val);
}

/**
 * ms1_client_get_size:
 * @properties: a #GHashTable
 *
 * Returns "size" property value.
 *
 * Returns: property value or -1 if it is not available
 **/
gint
ms1_client_get_size (GHashTable *properties)
{
  GValue *val;

  g_return_val_if_fail (properties, -1);

  val = g_hash_table_lookup (properties, MS1_PROP_SIZE);
  if (!val || !G_VALUE_HOLDS_INT (val)) {
    return -1;
  }

  return g_value_get_int (val);
}

/**
 * ms1_client_get_duration:
 * @properties: a #GHashTable
 *
 * Returns "duration" property value.
 *
 * Returns: property value or -1 if it is not available
 **/
gint
ms1_client_get_duration (GHashTable *properties)
{
  GValue *val;

  g_return_val_if_fail (properties, -1);

  val = g_hash_table_lookup (properties, MS1_PROP_DURATION);
  if (!val || !G_VALUE_HOLDS_INT (val)) {
    return -1;
  }

  return g_value_get_int (val);
}

/**
 * ms1_client_get_bitrate:
 * @properties: a #GHashTable
 *
 * Returns "bitrate" property value.
 *
 * Returns: property value or -1 if it is not available
 **/
gint
ms1_client_get_bitrate (GHashTable *properties)
{
  GValue *val;

  g_return_val_if_fail (properties, -1);

  val = g_hash_table_lookup (properties, MS1_PROP_BITRATE);
  if (!val || !G_VALUE_HOLDS_INT (val)) {
    return -1;
  }

  return g_value_get_int (val);
}

/**
 * ms1_client_get_sample_rate:
 * @properties: a #GHashTable
 *
 * Returns "sample-rate" property value.
 *
 * Returns: property value or -1 if it is not available
 **/
gint
ms1_client_get_sample_rate (GHashTable *properties)
{
  GValue *val;

  g_return_val_if_fail (properties, -1);

  val = g_hash_table_lookup (properties, MS1_PROP_SAMPLE_RATE);
  if (!val || !G_VALUE_HOLDS_INT (val)) {
    return -1;
  }

  return g_value_get_int (val);
}

/**
 * ms1_client_get_bits_per_sample:
 * @properties: a #GHashTable
 *
 * Returns "bits-per-sample" property value.
 *
 * Returns: property value of -1 if it is not available
 **/
gint
ms1_client_get_bits_per_sample (GHashTable *properties)
{
  GValue *val;

  g_return_val_if_fail (properties, -1);

  val = g_hash_table_lookup (properties, MS1_PROP_BITS_PER_SAMPLE);
  if (!val || !G_VALUE_HOLDS_INT (val)) {
    return -1;
  }

  return g_value_get_int (val);
}

/**
 * ms1_client_get_width:
 * @properties: a #GHashTable
 *
 * Returns "width" property value.
 *
 * Returns: property value or -1 if it is not available
 **/
gint
ms1_client_get_width (GHashTable *properties)
{
  GValue *val;

  g_return_val_if_fail (properties, -1);

  val = g_hash_table_lookup (properties, MS1_PROP_WIDTH);
  if (!val || !G_VALUE_HOLDS_INT (val)) {
    return -1;
  }

  return g_value_get_int (val);
}

/**
 * ms1_client_get_height:
 * @properties: a #GHashTable
 *
 * Returns "height" property value.
 *
 * Returns: property value or -1 if it is not available
 **/
gint
ms1_client_get_height (GHashTable *properties)
{
  GValue *val;

  g_return_val_if_fail (properties, -1);

  val = g_hash_table_lookup (properties, MS1_PROP_HEIGHT);
  if (!val || !G_VALUE_HOLDS_INT (val)) {
    return -1;
  }

  return g_value_get_int (val);
}

/**
 * ms1_client_get_color_depth:
 * @properties: a #GHashTable
 *
 * Returns "color-depth" property value.
 *
 * Returns: property value or -1 if it is not available
 **/
gint
ms1_client_get_color_depth (GHashTable *properties)
{
  GValue *val;

  g_return_val_if_fail (properties, -1);

  val = g_hash_table_lookup (properties, MS1_PROP_COLOR_DEPTH);
  if (!val || !G_VALUE_HOLDS_INT (val)) {
    return -1;
  }

  return g_value_get_int (val);
}

/**
 * ms1_client_get_pixel_width:
 * @properties: a #GHashTable
 *
 * Returns "pixel-width" property value.
 *
 * Returns: property value or -1 if it is not available
 **/
gint
ms1_client_get_pixel_width (GHashTable *properties)
{
  GValue *val;

  g_return_val_if_fail (properties, -1);

  val = g_hash_table_lookup (properties, MS1_PROP_PIXEL_WIDTH);
  if (!val || !G_VALUE_HOLDS_INT (val)) {
    return -1;
  }

  return g_value_get_int (val);
}

/**
 * ms1_client_get_pixel_height:
 * @properties: a #GHashTable
 *
 * Returns "pixel-height" property value.
 *
 * Returns: property value or -1 if it is not available
 **/
gint
ms1_client_get_pixel_height (GHashTable *properties)
{
  GValue *val;

  g_return_val_if_fail (properties, -1);

  val = g_hash_table_lookup (properties, MS1_PROP_PIXEL_HEIGHT);
  if (!val || !G_VALUE_HOLDS_INT (val)) {
    return -1;
  }

  return g_value_get_int (val);
}

/**
 * ms1_client_get_urls:
 * @properties: a #GHashTable
 *
 * Returns "URLs" property value.
 *
 * Returns: a new @NULL-terminated array of strings or @NULL if it is not
 * available
 **/
gchar **
ms1_client_get_urls (GHashTable *properties)
{
  GValue *val;

  g_return_val_if_fail (properties, NULL);

  val = g_hash_table_lookup (properties, MS1_PROP_URLS);
  if (!val || !G_VALUE_HOLDS_BOXED (val)) {
    return NULL;
  }

  return g_strdupv (g_value_get_boxed (val));
}
