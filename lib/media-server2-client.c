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

#define MS2_CLIENT_GET_PRIVATE(o)                                    \
  G_TYPE_INSTANCE_GET_PRIVATE((o), MS2_TYPE_CLIENT, MS2ClientPrivate)

struct _MS2ClientPrivate {
  DBusGProxy *proxy_provider;
};

G_DEFINE_TYPE (MS2Client, ms2_client, G_TYPE_OBJECT);

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

static void
free_gvalue (GValue *v)
{
  g_value_unset (v);
  g_free (v);
}

GHashTable *
ms2_client_get_properties (MS2Client *client,
                           const gchar *id,
                           const gchar **properties,
                           GError **error)
{
  GHashTable *prop_result;
  GPtrArray *result = NULL;
  gint i;

  g_return_val_if_fail (MS2_IS_CLIENT (client), NULL);

  if (!org_gnome_UPnP_MediaServer2_get_properties (client->priv->proxy_provider,
                                                   id,
                                                   properties,
                                                   &result,
                                                   error)) {
    return NULL;
  }

  prop_result = g_hash_table_new_full (g_str_hash,
                                       g_str_equal,
                                       (GDestroyNotify) g_free,
                                       (GDestroyNotify) free_gvalue);
  for (i = 0; i < result->len; i++) {
    g_hash_table_insert (prop_result,
                         g_strdup (properties[i]),
                         g_ptr_array_index (result, i));
  }

  g_ptr_array_free (result, TRUE);

  return prop_result;
}
