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

#include "media-server2-client-glue.h"
#include "media-server2-client.h"

#define MS2_DBUS_SERVICE_PREFIX "org.gnome.UPnP.MediaServer2."
#define MS2_DBUS_PATH_PREFIX    "/org/gnome/UPnP/MediaServer2/"
#define MS2_DBUS_IFACE          "org.gnome.UPnP.MediaServer2"

#define ENTRY_POINT_IFACE "/org/gnome/UPnP/MediaServer2/"
#define ENTRY_POINT_NAME  "org.gnome.UPnP.MediaServer2."

#define DBUS_TYPE_G_ARRAY_OF_STRING                             \
  (dbus_g_type_get_collection ("GPtrArray", G_TYPE_STRING))

#define DBUS_TYPE_PROPERTIES                                    \
  dbus_g_type_get_collection ("GPtrArray", G_TYPE_VALUE)

#define DBUS_TYPE_CHILDREN                                              \
  dbus_g_type_get_collection ("GPtrArray", DBUS_TYPE_PROPERTIES)

#define MS2_CLIENT_GET_PRIVATE(o)                                       \
  G_TYPE_INSTANCE_GET_PRIVATE((o), MS2_TYPE_CLIENT, MS2ClientPrivate)

/*
 * AsyncData: used to pack needed data when dealing with async functions
 *   properties_result: when using get_properties_async() functions, it will
 *                      store the properties table result
 *   children_result: when using get_children_async() functions, it will store
 *                    the list of children result
 *   client: a reference to MS2Client
 *   properties: list of properties requested
 *   id: id of MediaObject to get properties/children from
 */
typedef struct {
  GHashTable *properties_result;
  GList *children_result;
  MS2Client *client;
  gchar **properties;
  gchar *id;
} AsyncData;

/*
 * Private MS2Client structure
 *   proxy_provider: a dbus proxy of content provider
 */
struct _MS2ClientPrivate {
  DBusGProxy *proxy_provider;
};

G_DEFINE_TYPE (MS2Client, ms2_client, G_TYPE_OBJECT);

/******************** PRIVATE API ********************/

/* Free gvalue */
static void
free_gvalue (GValue *v)
{
  g_value_unset (v);
  g_free (v);
}

/* Free AsyncData struct */
static void
free_async_data (AsyncData *adata)
{
  g_object_unref (adata->client);
  g_free (adata->id);
  g_strfreev (adata->properties);
  g_slice_free (AsyncData, adata);
}

/* Given a GPtrArray result (dbus answer of getting properties), returns a
   ghashtable with pairs <keys, values> */
static GHashTable *
get_properties_table (GPtrArray *result,
                      const gchar **properties)
{
  GHashTable *table;
  gint i;

  table = g_hash_table_new_full (g_str_hash,
                                 g_str_equal,
                                 (GDestroyNotify) g_free,
                                 (GDestroyNotify) free_gvalue);

  for (i = 0; i < result->len; i++) {
    g_hash_table_insert (table,
                         g_strdup (properties[i]),
                         g_boxed_copy (G_TYPE_VALUE,
                                       g_ptr_array_index (result, i)));
  }

  return table;
}

/* Given a GPtrArray result (dbus answer of getting children), returns a list
   of children, which in turn are tables with pairs <keys, values>. Note that
   child id is included in those pairs */
static GList *
get_children_list (GPtrArray *result,
                   const gchar **properties)
{
  GList *children = NULL;
  GPtrArray *prop_array;
  gint i;

  if (!result || result->len == 0) {
    return NULL;
  }

  for (i = 0; i < result->len; i++) {
    prop_array = g_ptr_array_index (result, i);
    children = g_list_prepend (children,
                               get_properties_table (prop_array,
                                                     properties));
  }

  return children;
}

/* Callback invoked by dbus as answer to get_properties_async() */
static void
get_properties_async_reply (DBusGProxy *proxy,
                            GPtrArray *result,
                            GError *error,
                            gpointer data)
{
  GSimpleAsyncResult *res = G_SIMPLE_ASYNC_RESULT (data);
  AsyncData *adata;

  adata = g_simple_async_result_get_op_res_gpointer (res);

  adata->properties_result =
    get_properties_table (result,
                          (const gchar **) adata->properties);
  g_boxed_free (DBUS_TYPE_PROPERTIES, result);

  g_simple_async_result_complete (res);
  g_object_unref (res);
}

/* Callback invoked by dbus as answer to get_children_async() */
static void
get_children_async_reply (DBusGProxy *proxy,
                          GPtrArray *result,
                          GError *error,
                          gpointer data)
{
  GSimpleAsyncResult *res = G_SIMPLE_ASYNC_RESULT (data);
  AsyncData *adata;

  adata = g_simple_async_result_get_op_res_gpointer (res);

  adata->children_result = get_children_list (result,
                                              (const gchar **) adata->properties);

  g_boxed_free (DBUS_TYPE_CHILDREN, result);

  g_simple_async_result_complete (res);
  g_object_unref (res);
}

/* Class init function */
static void
ms2_client_class_init (MS2ClientClass *klass)
{
  g_type_class_add_private (klass, sizeof (MS2ClientPrivate));
}

/* Object init function */
static void
ms2_client_init (MS2Client *client)
{
  client->priv = MS2_CLIENT_GET_PRIVATE (client);
}

/******************** PUBLIC API ********************/

/**
 * ms2_client_get_providers:
 *
 * Returns a list of content providers following MediaServer2 specification.
 *
 * Returns: a new @NULL-terminated array of strings
 **/
gchar **
ms2_client_get_providers ()
{
  DBusGConnection *connection;
  DBusGProxy *gproxy;
  GError *error = NULL;
  GPtrArray *providers;
  gchar **dbus_names;
  gchar **list_providers;
  gchar **p;
  gint i;
  gint prefix_size = strlen (MS2_DBUS_SERVICE_PREFIX);

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

  /* Filter the list to obtain those services that fulfils MediaServer2 spec */
  providers = g_ptr_array_new ();
  for (p = dbus_names; *p; p++) {
    if (g_str_has_prefix (*p, MS2_DBUS_SERVICE_PREFIX)) {
      g_ptr_array_add (providers, *p);
    }
  }

  /* Put them in a NULL-terminated array */
  list_providers = g_new (gchar *, providers->len + 1);
  for (i = 0; i < providers->len; i++) {
    list_providers[i] = g_strdup (g_ptr_array_index (providers, i) + prefix_size);
  }

  list_providers[i] = NULL;

  g_strfreev (dbus_names);
  g_ptr_array_free (providers, TRUE);

  return list_providers;
}

/**
 * ms2_client_new:
 * @provider: provider name.
 *
 * Create a new #MS2Client that will be used to obtain content from the provider specified.
 *
 * Providers can be obtained with ms2_client_get_providers().
 *
 * Returns: a new #MS2Client
 **/
MS2Client *ms2_client_new (const gchar *provider)
{
  DBusGConnection *connection;
  DBusGProxy *gproxy;
  GError *error = NULL;
  MS2Client *client;
  gchar *service_provider;
  gchar *path_provider;

  connection = dbus_g_bus_get (DBUS_BUS_SESSION, &error);
  if (!connection) {
    g_printerr ("Could not connect to session bus, %s\n", error->message);
    g_clear_error (&error);
    return NULL;
  }

  service_provider = g_strconcat (MS2_DBUS_SERVICE_PREFIX, provider, NULL);
  path_provider = g_strconcat (MS2_DBUS_PATH_PREFIX, provider, NULL);

  gproxy = dbus_g_proxy_new_for_name_owner (connection,
                                            service_provider,
                                            path_provider,
                                            MS2_DBUS_IFACE,
                                            &error);

  g_free (service_provider);
  g_free (path_provider);

  if (!gproxy) {
    g_printerr ("Could not connect to %s provider, %s\n",
                provider,
                error->message);
    g_clear_error (&error);
    return NULL;
  }

  client = g_object_new (MS2_TYPE_CLIENT, NULL);
  client->priv->proxy_provider = gproxy;

  return client;
}

/**
 * ms2_client_get_properties:
 * @client: a #MS2Client
 * @id: media identifier to obtain properties from
 * @properties: @NULL-terminated array of properties to request
 * @error: a #GError location to store the error ocurring, or @NULL to ignore
 *
 * Gets the properties of media id. Properties will be returned in a hash table
 * of <prop_id, prop_gvalue> pairs.
 *
 * Returns: a new #GHashTable
 **/
GHashTable *
ms2_client_get_properties (MS2Client *client,
                           const gchar *id,
                           const gchar **properties,
                           GError **error)
{
  GHashTable *prop_result;
  GPtrArray *result = NULL;

  g_return_val_if_fail (MS2_IS_CLIENT (client), NULL);

  if (!org_gnome_UPnP_MediaServer2_get_properties (client->priv->proxy_provider,
                                                   id,
                                                   properties,
                                                   &result,
                                                   error)) {
    return NULL;
  }

  prop_result = get_properties_table (result, properties);
  g_boxed_free (DBUS_TYPE_PROPERTIES, result);

  return prop_result;
}

/**
 * ms2_client_get_properties_async:
 * @client: a #MS2Client
 * @id: media identifier to obtain properties from
 * @properties: @NULL-terminated array of properties to request
 * @callback: a #GAsyncReadyCallback to call when request is satisfied
 * @user_data: the data to pass to callback function
 *
 * Starts an asynchronous get properties.
 *
 * For more details, see ms2_client_get_properties(), which is the synchronous
 * version of this call.
 *
 * When the properties have been obtained, @callback will be called with
 * @user_data. To finish the operation, call ms2_client_get_properties_finish()
 * with the #GAsyncResult returned by the @callback.
 **/
void ms2_client_get_properties_async (MS2Client *client,
                                      const gchar *id,
                                      const gchar **properties,
                                      GAsyncReadyCallback callback,
                                      gpointer user_data)
{
  AsyncData *adata;
  GSimpleAsyncResult *res;

  g_return_if_fail (MS2_IS_CLIENT (client));

  adata = g_slice_new0 (AsyncData);

  res = g_simple_async_result_new (G_OBJECT (client),
                                   callback,
                                   user_data,
                                   ms2_client_get_properties_async);

  adata->client = g_object_ref (client);
  adata->id = g_strdup (id);
  adata->properties = g_strdupv ((gchar **) properties);

  g_simple_async_result_set_op_res_gpointer (res,
                                             adata,
                                             (GDestroyNotify) free_async_data);

  org_gnome_UPnP_MediaServer2_get_properties_async (client->priv->proxy_provider,
                                                    id,
                                                    properties,
                                                    get_properties_async_reply,
                                                    res);
}

/**
 * ms2_client_get_properties_finish:
 * @client: a #MS2Client
 * @res: a #GAsyncResult
 * @error: a #GError location to store the error ocurring, or @NULL to ignore
 *
 * Finishes an asynchronous getting properties operation. Properties are
 * returned in a #GHashTable.
 *
 * Returns: a new #GHashTable
 **/
GHashTable *
ms2_client_get_properties_finish (MS2Client *client,
                                  GAsyncResult *res,
                                  GError **error)
{
  AsyncData *adata;

  g_return_val_if_fail (g_simple_async_result_get_source_tag (G_SIMPLE_ASYNC_RESULT (res)) ==
                        ms2_client_get_properties_async, NULL);

  adata = g_simple_async_result_get_op_res_gpointer (G_SIMPLE_ASYNC_RESULT (res));
  return adata->properties_result;
}

/**
 * ms2_client_get_children:
 * @client: a #MS2Client
 * @id: container identifier to get children from
 * @offset: number of children to skip
 * @max_count: maximum number of children to return, or -1 for no limit
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
ms2_client_get_children (MS2Client *client,
                         const gchar *id,
                         guint offset,
                         gint max_count,
                         const gchar **properties,
                         GError **error)
{
  GPtrArray *result = NULL;
  GList *children = NULL;

  g_return_val_if_fail (MS2_IS_CLIENT (client), NULL);

  if (!org_gnome_UPnP_MediaServer2_get_children (client->priv->proxy_provider,
                                                 id,
                                                 offset,
                                                 max_count,
                                                 properties,
                                                 &result,
                                                 error)) {
    return NULL;
  }


  children = get_children_list (result, properties);

  g_boxed_free (DBUS_TYPE_CHILDREN, result);

  return children;
}

/**
 * ms2_client_get_children_async:
 * @client: a #MS2Client
 * @id: container identifier to get children from
 * @offset: number of children to skip
 * @max_count: maximum number of children to return, or -1 for no limit
 * @properties: @NULL-terminated array of properties to request for each child
 * @callback: a #GAsyncReadyCallback to call when request is satisfied
 * @user_data: the data to pass to callback function
 *
 * Starts an asynchronous get children.
 *
 * For more details, see ms2_client_get_children(), which is the synchronous
 * version of this call.
 *
 * When the children have been obtained, @callback will be called with
 * @user_data. To finish the operation, call ms2_client_get_children_finish()
 * with the #GAsyncResult returned by the @callback.
 **/
void ms2_client_get_children_async (MS2Client *client,
                                    const gchar *id,
                                    guint offset,
                                    gint max_count,
                                    const gchar **properties,
                                    GAsyncReadyCallback callback,
                                    gpointer user_data)
{
  AsyncData *adata;
  GSimpleAsyncResult *res;

  g_return_if_fail (MS2_IS_CLIENT (client));

  adata = g_slice_new0 (AsyncData);

  res = g_simple_async_result_new (G_OBJECT (client),
                                   callback,
                                   user_data,
                                   ms2_client_get_children_async);

  adata->client = g_object_ref (client);
  adata->id = g_strdup (id);
  adata->properties = g_strdupv ((gchar **) properties);

  g_simple_async_result_set_op_res_gpointer (res,
                                             adata,
                                             (GDestroyNotify) free_async_data);

  org_gnome_UPnP_MediaServer2_get_children_async (client->priv->proxy_provider,
                                                  id,
                                                  offset,
                                                  max_count,
                                                  properties,
                                                  get_children_async_reply,
                                                  res);
}

/**
 * ms2_client_get_children_finish:
 * @client: a #MS2Client
 * @res: a #GAsyncResult
 * @error: a #GError location to store the error ocurring, or @NULL to ignore
 *
 * Finishes an asynchronous getting children operation.
 *
 * Returns: a new #GList of #GHashTAble. To free it, free first each element
 * (g_hash_table_unref()) and finally the list itself (g_list_free())
 **/
GList *
ms2_client_get_children_finish (MS2Client *client,
                                GAsyncResult *res,
                                GError **error)
{
  AsyncData *adata;

  g_return_val_if_fail (g_simple_async_result_get_source_tag (G_SIMPLE_ASYNC_RESULT (res)) ==
                        ms2_client_get_children_async, NULL);

  adata = g_simple_async_result_get_op_res_gpointer (G_SIMPLE_ASYNC_RESULT (res));
  return adata->children_result;
}

/******************** PROPERTIES TABLE API ********************/

/**
 * ms2_client_get_id:
 * @properties: a #GHashTable
 *
 * Returns "id" property value.
 *
 * Returns: property value or @NULL if it is not available
 **/
const gchar *
ms2_client_get_id (GHashTable *properties)
{
  GValue *val;

  g_return_val_if_fail (properties, NULL);

  val = g_hash_table_lookup (properties, MS2_PROP_ID);
  if (!val || !G_VALUE_HOLDS_STRING (val)) {
    return NULL;
  }

  return g_value_get_string (val);
}

/**
 * ms2_client_get_parent:
 * @properties: a #GHashTable
 *
 * Returns "parent" property value.
 *
 * Returns: property value or @NULL if it is not available
 **/
const gchar *
ms2_client_get_parent (GHashTable *properties)
{
  GValue *val;

  g_return_val_if_fail (properties, NULL);

  val = g_hash_table_lookup (properties, MS2_PROP_PARENT);
  if (!val || !G_VALUE_HOLDS_STRING (val)) {
    return NULL;
  }

  return g_value_get_string (val);
}

/**
 * ms2_client_get_display_name:
 * @properties: a #GHashTable
 *
 * Returns "display-name" property value.
 *
 * Returns: property value or @NULL if it is not available
 **/
const gchar *
ms2_client_get_display_name (GHashTable *properties)
{
  GValue *val;

  g_return_val_if_fail (properties, NULL);

  val = g_hash_table_lookup (properties, MS2_PROP_DISPLAY_NAME);
  if (!val || !G_VALUE_HOLDS_STRING (val)) {
    return NULL;
  }

  return g_value_get_string (val);
}

/**
 * ms2_client_get_item_type:
 * @properties: a #GHashTable
 *
 * Returns "type" property value.
 *
 * Returns: property value
 **/
MS2ItemType
ms2_client_get_item_type (GHashTable *properties)
{
  GValue *val;
  const gchar *type;

  g_return_val_if_fail (properties, MS2_ITEM_TYPE_UNKNOWN);

  val = g_hash_table_lookup (properties, MS2_PROP_DISPLAY_NAME);
  if (!val || !G_VALUE_HOLDS_STRING (val)) {
    return MS2_ITEM_TYPE_UNKNOWN;
  }

  type = g_value_get_string (val);

  if (g_strcmp0 (type, MS2_TYPE_CONTAINER) == 0) {
    return MS2_ITEM_TYPE_CONTAINER;
  } else if (g_strcmp0 (type, MS2_TYPE_VIDEO) == 0) {
    return MS2_ITEM_TYPE_VIDEO;
  } else if (g_strcmp0 (type, MS2_TYPE_MOVIE) == 0) {
    return MS2_ITEM_TYPE_MOVIE;
  } else if (g_strcmp0 (type, MS2_TYPE_AUDIO) == 0) {
    return MS2_ITEM_TYPE_AUDIO;
  } else if (g_strcmp0 (type, MS2_TYPE_MUSIC) == 0) {
    return MS2_ITEM_TYPE_MUSIC;
  } else if (g_strcmp0 (type, MS2_TYPE_IMAGE) == 0) {
    return MS2_ITEM_TYPE_IMAGE;
  } else if (g_strcmp0 (type, MS2_TYPE_PHOTO) == 0) {
    return MS2_ITEM_TYPE_PHOTO;
  } else {
    return MS2_ITEM_TYPE_UNKNOWN;
  }
}

/**
 * ms2_client_get_icon:
 * @properties: a #GHashTable
 *
 * Returns "icon" property value.
 *
 * Returns: property value or @NULL if it is not available
 **/
const gchar *
ms2_client_get_icon (GHashTable *properties)
{
  GValue *val;

  g_return_val_if_fail (properties, NULL);

  val = g_hash_table_lookup (properties, MS2_PROP_ICON);
  if (!val || !G_VALUE_HOLDS_STRING (val)) {
    return NULL;
  }

  return g_value_get_string (val);
}

/**
 * ms2_client_get_mime_type:
 * @properties: a #GHashTable
 *
 * Returns "mime-type" property value.
 *
 * Returns: property value or @NULL if it is not available
 **/
const gchar *
ms2_client_get_mime_type (GHashTable *properties)
{
  GValue *val;

  g_return_val_if_fail (properties, NULL);

  val = g_hash_table_lookup (properties, MS2_PROP_MIME_TYPE);
  if (!val || !G_VALUE_HOLDS_STRING (val)) {
    return NULL;
  }

  return g_value_get_string (val);
}

/**
 * ms2_client_get_artist:
 * @properties: a #GHashTable
 *
 * Returns "artist" property value.
 *
 * Returns: property value or @NULL if it is not available
 **/
const gchar *
ms2_client_get_artist (GHashTable *properties)
{
  GValue *val;

  g_return_val_if_fail (properties, NULL);

  val = g_hash_table_lookup (properties, MS2_PROP_ARTIST);
  if (!val || !G_VALUE_HOLDS_STRING (val)) {
    return NULL;
  }

  return g_value_get_string (val);
}

/**
 * ms2_client_get_album:
 * @properties: a #GHashTable
 *
 * Returns "album" property value.
 *
 * Returns: property value or @NULL if it is not available
 **/
const gchar *
ms2_client_get_album (GHashTable *properties)
{
  GValue *val;

  g_return_val_if_fail (properties, NULL);

  val = g_hash_table_lookup (properties, MS2_PROP_ALBUM);
  if (!val || !G_VALUE_HOLDS_STRING (val)) {
    return NULL;
  }

  return g_value_get_string (val);
}

/**
 * ms2_client_get_date:
 * @properties: a #GHashTable
 *
 * Returns "date" property value.
 *
 * Returns: property value or @NULL if it is not available
 **/
const gchar *
ms2_client_get_date (GHashTable *properties)
{
  GValue *val;

  g_return_val_if_fail (properties, NULL);

  val = g_hash_table_lookup (properties, MS2_PROP_DATE);
  if (!val || !G_VALUE_HOLDS_STRING (val)) {
    return NULL;
  }

  return g_value_get_string (val);
}

/**
 * ms2_client_get_dlna_profile:
 * @properties: a #GHashTable
 *
 * Returns "dlna-profile" property value.
 *
 * Returns: property value or @NULL if it is not available
 **/
const gchar *
ms2_client_get_dlna_profile (GHashTable *properties)
{
  GValue *val;

  g_return_val_if_fail (properties, NULL);

  val = g_hash_table_lookup (properties, MS2_PROP_DLNA_PROFILE);
  if (!val || !G_VALUE_HOLDS_STRING (val)) {
    return NULL;
  }

  return g_value_get_string (val);
}

/**
 * ms2_client_get_thumbnail:
 * @properties: a #GHashTable
 *
 * Returns "thumbanil" property value.
 *
 * Returns: property value or @NULL if it is not available
 **/
const gchar *
ms2_client_get_thumbnail (GHashTable *properties)
{
  GValue *val;

  g_return_val_if_fail (properties, NULL);

  val = g_hash_table_lookup (properties, MS2_PROP_THUMBNAIL);
  if (!val || !G_VALUE_HOLDS_STRING (val)) {
    return NULL;
  }

  return g_value_get_string (val);
}

/**
 * ms2_client_get_genre:
 * @properties: a #GHashTable
 *
 * Returns "genre" property value.
 *
 * Returns: property value or @NULL if it is not available
 **/
const gchar *
ms2_client_get_genre (GHashTable *properties)
{
  GValue *val;

  g_return_val_if_fail (properties, NULL);

  val = g_hash_table_lookup (properties, MS2_PROP_GENRE);
  if (!val || !G_VALUE_HOLDS_STRING (val)) {
    return NULL;
  }

  return g_value_get_string (val);
}

/**
 * ms2_client_get_child_count:
 * @properties: a #GHashTable
 *
 * Returns "child-count" property value.
 *
 * Returns: property value or -1 if it is not available
 **/
gint
ms2_client_get_child_count (GHashTable *properties)
{
  GValue *val;

  g_return_val_if_fail (properties, -1);

  val = g_hash_table_lookup (properties, MS2_PROP_CHILD_COUNT);
  if (!val || !G_VALUE_HOLDS_INT (val)) {
    return -1;
  }

  return g_value_get_int (val);
}

/**
 * ms2_client_get_size:
 * @properties: a #GHashTable
 *
 * Returns "size" property value.
 *
 * Returns: property value or -1 if it is not available
 **/
gint
ms2_client_get_size (GHashTable *properties)
{
  GValue *val;

  g_return_val_if_fail (properties, -1);

  val = g_hash_table_lookup (properties, MS2_PROP_SIZE);
  if (!val || !G_VALUE_HOLDS_INT (val)) {
    return -1;
  }

  return g_value_get_int (val);
}

/**
 * ms2_client_get_duration:
 * @properties: a #GHashTable
 *
 * Returns "duration" property value.
 *
 * Returns: property value or -1 if it is not available
 **/
gint
ms2_client_get_duration (GHashTable *properties)
{
  GValue *val;

  g_return_val_if_fail (properties, -1);

  val = g_hash_table_lookup (properties, MS2_PROP_DURATION);
  if (!val || !G_VALUE_HOLDS_INT (val)) {
    return -1;
  }

  return g_value_get_int (val);
}

/**
 * ms2_client_get_bitrate:
 * @properties: a #GHashTable
 *
 * Returns "bitrate" property value.
 *
 * Returns: property value or -1 if it is not available
 **/
gint
ms2_client_get_bitrate (GHashTable *properties)
{
  GValue *val;

  g_return_val_if_fail (properties, -1);

  val = g_hash_table_lookup (properties, MS2_PROP_BITRATE);
  if (!val || !G_VALUE_HOLDS_INT (val)) {
    return -1;
  }

  return g_value_get_int (val);
}

/**
 * ms2_client_get_sample_rate:
 * @properties: a #GHashTable
 *
 * Returns "sample-rate" property value.
 *
 * Returns: property value or -1 if it is not available
 **/
gint
ms2_client_get_sample_rate (GHashTable *properties)
{
  GValue *val;

  g_return_val_if_fail (properties, -1);

  val = g_hash_table_lookup (properties, MS2_PROP_SAMPLE_RATE);
  if (!val || !G_VALUE_HOLDS_INT (val)) {
    return -1;
  }

  return g_value_get_int (val);
}

/**
 * ms2_client_get_bits_per_sample:
 * @properties: a #GHashTable
 *
 * Returns "bits-per-sample" property value.
 *
 * Returns: property value of -1 if it is not available
 **/
gint
ms2_client_get_bits_per_sample (GHashTable *properties)
{
  GValue *val;

  g_return_val_if_fail (properties, -1);

  val = g_hash_table_lookup (properties, MS2_PROP_BITS_PER_SAMPLE);
  if (!val || !G_VALUE_HOLDS_INT (val)) {
    return -1;
  }

  return g_value_get_int (val);
}

/**
 * ms2_client_get_width:
 * @properties: a #GHashTable
 *
 * Returns "width" property value.
 *
 * Returns: property value or -1 if it is not available
 **/
gint
ms2_client_get_width (GHashTable *properties)
{
  GValue *val;

  g_return_val_if_fail (properties, -1);

  val = g_hash_table_lookup (properties, MS2_PROP_WIDTH);
  if (!val || !G_VALUE_HOLDS_INT (val)) {
    return -1;
  }

  return g_value_get_int (val);
}

/**
 * ms2_client_get_height:
 * @properties: a #GHashTable
 *
 * Returns "height" property value.
 *
 * Returns: property value or -1 if it is not available
 **/
gint
ms2_client_get_height (GHashTable *properties)
{
  GValue *val;

  g_return_val_if_fail (properties, -1);

  val = g_hash_table_lookup (properties, MS2_PROP_HEIGHT);
  if (!val || !G_VALUE_HOLDS_INT (val)) {
    return -1;
  }

  return g_value_get_int (val);
}

/**
 * ms2_client_get_color_depth:
 * @properties: a #GHashTable
 *
 * Returns "color-depth" property value.
 *
 * Returns: property value or -1 if it is not available
 **/
gint
ms2_client_get_color_depth (GHashTable *properties)
{
  GValue *val;

  g_return_val_if_fail (properties, -1);

  val = g_hash_table_lookup (properties, MS2_PROP_COLOR_DEPTH);
  if (!val || !G_VALUE_HOLDS_INT (val)) {
    return -1;
  }

  return g_value_get_int (val);
}

/**
 * ms2_client_get_pixel_width:
 * @properties: a #GHashTable
 *
 * Returns "pixel-width" property value.
 *
 * Returns: property value or -1 if it is not available
 **/
gint
ms2_client_get_pixel_width (GHashTable *properties)
{
  GValue *val;

  g_return_val_if_fail (properties, -1);

  val = g_hash_table_lookup (properties, MS2_PROP_PIXEL_WIDTH);
  if (!val || !G_VALUE_HOLDS_INT (val)) {
    return -1;
  }

  return g_value_get_int (val);
}

/**
 * ms2_client_get_pixel_height:
 * @properties: a #GHashTable
 *
 * Returns "pixel-height" property value.
 *
 * Returns: property value or -1 if it is not available
 **/
gint
ms2_client_get_pixel_height (GHashTable *properties)
{
  GValue *val;

  g_return_val_if_fail (properties, -1);

  val = g_hash_table_lookup (properties, MS2_PROP_PIXEL_HEIGHT);
  if (!val || !G_VALUE_HOLDS_INT (val)) {
    return -1;
  }

  return g_value_get_int (val);
}

/**
 * ms2_client_get_urls:
 * @properties: a #GHashTable
 *
 * Returns "URLs" property value.
 *
 * Returns: a new @NULL-terminated array of strings or @NULL if it is not
 * available
 **/
gchar **
ms2_client_get_urls (GHashTable *properties)
{
  GValue *val;

  g_return_val_if_fail (properties, NULL);

  val = g_hash_table_lookup (properties, MS2_PROP_URLS);
  if (!val || !G_VALUE_HOLDS_BOXED (val)) {
    return NULL;
  }

  return g_strdupv (g_value_get_boxed (val));
}
