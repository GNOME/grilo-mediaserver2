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

#define DBUS_TYPE_PROPERTIES                    \
  dbus_g_type_get_collection ("GPtrArray",      \
                              G_TYPE_VALUE)

#define DBUS_TYPE_CHILDREN                      \
  dbus_g_type_get_map ("GHashTable",            \
                       G_TYPE_STRING,           \
                       DBUS_TYPE_PROPERTIES)    \

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
get_properties_table (const gchar *id,
                      const gchar **properties,
                      GPtrArray *result)
{
  GHashTable *table;
  GValue *id_value;
  gint i;

  table = g_hash_table_new_full (g_str_hash,
                                 g_str_equal,
                                 (GDestroyNotify) g_free,
                                 (GDestroyNotify) free_gvalue);

  id_value = g_new0 (GValue, 1);
  g_value_init (id_value, G_TYPE_STRING);
  g_value_set_string (id_value, id);
  g_hash_table_insert (table,
                       g_strdup (MS2_PROP_ID),
                       id_value);

  for (i = 0; i < result->len; i++) {
    g_hash_table_insert (table,
                         g_strdup (properties[i]),
                         g_boxed_copy (G_TYPE_VALUE,
                                       g_ptr_array_index (result, i)));
  }

  return table;
}

/* Given a GHashTable result (dbus answer of getting children), returns a list
   of children, which in turn are tables with pairs <keys, values>. Note that
   child id is included in those pairs */
static GList *
get_children_list (GHashTable *result,
                   const gchar **properties)
{
  GList *child_id;
  GList *children = NULL;
  GList *children_id;
  GPtrArray *prop_array;

  if (!result || g_hash_table_size (result) == 0) {
    return NULL;
  }

  children_id = g_hash_table_get_keys (result);

  for (child_id = children_id; child_id; child_id = g_list_next (child_id)) {
    prop_array = g_hash_table_lookup (result, child_id->data);
    children = g_list_prepend (children,
                               get_properties_table (child_id->data,
                                                     properties,
                                                     prop_array));
  }

  g_list_free (children_id);

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
    get_properties_table (adata->id,
                          (const gchar **) adata->properties,
                          result);
  g_boxed_free (DBUS_TYPE_PROPERTIES, result);

  g_simple_async_result_complete (res);
  g_object_unref (res);
}

#if 0
/* Callback invoked by dbus as answer to get_children_async() */
static void
get_children_async_reply (DBusGProxy *proxy,
                          GHashTable *result,
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
#endif

/* Class init function */
static void
ms2_client_class_init (MS2ClientClass *klass)
{
  g_type_class_add_private (klass, sizeof (MS2ClientPrivate));
}

static void
ms2_client_init (MS2Client *client)
{
  client->priv = MS2_CLIENT_GET_PRIVATE (client);
}

/******************** PUBLIC API ********************/

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

  providers = g_ptr_array_new ();
  for (p = dbus_names; *p; p++) {
    if (g_str_has_prefix (*p, MS2_DBUS_SERVICE_PREFIX)) {
      g_ptr_array_add (providers, *p);
    }
  }

  list_providers = g_new (gchar *, providers->len + 1);
  for (i = 0; i < providers->len; i++) {
    list_providers[i] = g_strdup (g_ptr_array_index (providers, i) + prefix_size);
  }

  list_providers[i] = NULL;

  g_strfreev (dbus_names);
  g_ptr_array_free (providers, TRUE);

  return list_providers;
}

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

  prop_result = get_properties_table (id, properties, result);
  g_boxed_free (DBUS_TYPE_PROPERTIES, result);

  return prop_result;
}

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

GList *
ms2_client_get_children (MS2Client *client,
                         const gchar *id,
                         guint offset,
                         gint max_count,
                         const gchar **properties,
                         GError **error)
{
  GHashTable *result = NULL;
  GList *children = NULL;

  g_return_val_if_fail (MS2_IS_CLIENT (client), NULL);

#if 0
  if (!org_gnome_UPnP_MediaServer2_get_children (client->priv->proxy_provider,
                                                 id,
                                                 offset,
                                                 max_count,
                                                 properties,
                                                 &result,
                                                 error)) {
    return NULL;
  }
#endif

  children = get_children_list (result, properties);

  g_boxed_free (DBUS_TYPE_CHILDREN, result);

  return children;
}

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

#if 0
  org_gnome_UPnP_MediaServer2_get_children_async (client->priv->proxy_provider,
                                                  id,
                                                  offset,
                                                  max_count,
                                                  properties,
                                                  get_children_async_reply,
                                                  res);
#endif
}

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
