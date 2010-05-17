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

#include <dbus/dbus-glib-lowlevel.h>

#include "media-server1-private.h"
#include "media-server1-observer.h"
#include "media-server1-client.h"

#define ENTRY_POINT_NAME "org.gnome.UPnP.MediaServer1."
#define ENTRY_POINT_NAME_LENGTH 28

#define MS1_OBSERVER_GET_PRIVATE(o)                                     \
  G_TYPE_INSTANCE_GET_PRIVATE((o), MS1_TYPE_OBSERVER, MS1ObserverPrivate)

enum {
  NEW,
  LAST_SIGNAL
};

/*
 * Private MS1Observer structure
 *   clients: a table with the clients
 *   proxy: proxy to dbus service
 */
struct _MS1ObserverPrivate {
  GHashTable *clients;
  DBusGProxy *proxy;
};

static MS1Observer *observer_instance = NULL;
static guint32 signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE (MS1Observer, ms1_observer, G_TYPE_OBJECT);

/******************** PRIVATE API ********************/

/* Callback invoked when a NameOwner is changed in dbus */
static void
name_owner_changed (DBusGProxy *proxy,
                    const gchar *name,
                    const gchar *old_owner,
                    const gchar *new_owner,
                    MS1Observer *observer)
{
  GList *clients;

  /* Check if it has something to do with the spec */
  if (!g_str_has_prefix (name, MS1_DBUS_SERVICE_PREFIX)) {
    return;
  }

  name += MS1_DBUS_SERVICE_PREFIX_LENGTH;

  /* Check if it has been removed */
  if (*new_owner == '\0') {
    clients = g_hash_table_lookup (observer->priv->clients, name);
    g_list_foreach (clients, (GFunc) ms1_client_notify_destroy, NULL);
    return;
  }

  /* Check if it has been added */
  if (*old_owner == '\0') {
    g_signal_emit (observer, signals[NEW], 0, name);
  }
}

/* Check for Updated signal and notify appropriate MS1Client, which will send a
   signal */
static DBusHandlerResult
listen_updated_signal (DBusConnection *connection,
                       DBusMessage *message,
                       void *user_data)
{
  GList *clients;
  MS1Observer *observer = MS1_OBSERVER (user_data);
  gchar **path;

  if (dbus_message_is_signal (message,
                              "org.gnome.UPnP.MediaContainer1",
                              "Updated")) {
    dbus_message_get_path_decomposed (message, &path);

    if (g_strv_length (path) < 5) {
      g_printerr ("Wrong object path %s\n", dbus_message_get_path (message));
      dbus_free_string_array (path);
      return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    }

    /* Check if there is a client for this provider; if so, then notify all of
       them */
    clients = g_hash_table_lookup (observer->priv->clients, path[4]);
    dbus_free_string_array (path);

    g_list_foreach (clients,
                    (GFunc) ms1_client_notify_updated,
                    (gpointer) dbus_message_get_path (message));

    return DBUS_HANDLER_RESULT_HANDLED;
  }

  return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

/* Creates an instance of observer */
static MS1Observer *
create_instance ()
{
  DBusConnection *connection;
  DBusGConnection *gconnection;
  GError *error = NULL;
  MS1Observer *observer;

  gconnection = dbus_g_bus_get (DBUS_BUS_SESSION, &error);
  if (!gconnection) {
    g_printerr ("Could not connect to session bus, %s\n", error->message);
    g_error_free (error);
    return NULL;
  }

  observer = g_object_new (MS1_TYPE_OBSERVER, NULL);

  observer->priv->proxy = dbus_g_proxy_new_for_name (gconnection,
                                                     DBUS_SERVICE_DBUS,
                                                     DBUS_PATH_DBUS,
                                                     DBUS_INTERFACE_DBUS);

  /* Listen for name-owner-changed signal */
  dbus_g_proxy_add_signal (observer->priv->proxy,
                           "NameOwnerChanged",
                           G_TYPE_STRING,
                           G_TYPE_STRING,
                           G_TYPE_STRING,
                           G_TYPE_INVALID);
  dbus_g_proxy_connect_signal (observer->priv->proxy,
                               "NameOwnerChanged",
                               G_CALLBACK (name_owner_changed),
                               observer,
                               NULL);

  /* Listen for Updated signal */
  connection = dbus_g_connection_get_connection (gconnection);
  dbus_bus_add_match (connection,
                      "interface='org.gnome.UPnP.MediaContainer1',"
                      "member='Updated'",
                      NULL);
  dbus_connection_add_filter (connection,
                              listen_updated_signal,
                              observer,
                              NULL);
  return observer;
}

/* Class init function */
static void
ms1_observer_class_init (MS1ObserverClass *klass)
{
  g_type_class_add_private (klass, sizeof (MS1ObserverPrivate));

  /**
   * MS1Observer::observer:
   * @observer: the #MS1Observer
   * @provider: name of provider that has been added to dbus
   *
   * Notifies when a new provider comes up.
   **/
  signals[NEW] = g_signal_new ("new",
                               G_TYPE_FROM_CLASS (klass),
                               G_SIGNAL_RUN_FIRST | G_SIGNAL_NO_RECURSE,
                               G_STRUCT_OFFSET (MS1ObserverClass, new),
                               NULL,
                               NULL,
                               g_cclosure_marshal_VOID__STRING,
                               G_TYPE_NONE,
                               1,
                               G_TYPE_STRING);
}

/* Object init function */
static void
ms1_observer_init (MS1Observer *client)
{
  client->priv = MS1_OBSERVER_GET_PRIVATE (client);
  client->priv->clients = g_hash_table_new_full (g_str_hash,
                                                 g_str_equal,
                                                 g_free,
                                                 NULL);
}

/****************** INTERNAL PUBLIC API (NOT TO BE EXPORTED) ******************/

/* Register a client */
void
ms1_observer_add_client (MS1Client *client,
                         const gchar *provider)
{
  GList *clients;
  MS1Observer *observer;

  observer = ms1_observer_get_instance ();
  if (!observer) {
    return;
  }

  clients = g_hash_table_lookup (observer->priv->clients, provider);
  clients = g_list_prepend (clients, client);
  g_hash_table_insert (observer->priv->clients, g_strdup (provider), clients);
}

/* Remove a client */
void
ms1_observer_remove_client (MS1Client *client,
                            const gchar *provider)
{
  GList *clients;
  GList *remove_client;
  MS1Observer *observer;

  observer = ms1_observer_get_instance ();
  if (!observer) {
    return;
  }

  clients = g_hash_table_lookup (observer->priv->clients, provider);
  remove_client = g_list_find (clients, client);
  if (remove_client) {
    clients = g_list_delete_link (clients, remove_client);
    /* Check if there are more clients */
    if (clients) {
      g_hash_table_insert (observer->priv->clients, g_strdup (provider), clients);
    } else {
      g_hash_table_remove (observer->priv->clients, provider);
    }
  }
}

/******************** PUBLIC API ********************/

/**
 * ms1_observer_get_instance:
 *
 * Returns the observer instance
 *
 * Returns: the observer instance or @NULL if it could not be created
 **/
MS1Observer *ms1_observer_get_instance ()
{
  if (!observer_instance) {
    observer_instance = create_instance ();
  }

  return observer_instance;
}
