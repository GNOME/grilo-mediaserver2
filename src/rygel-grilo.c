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

#include <stdio.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-bindings.h>

#include "rygel-grilo-media-server.h"

#define ENTRY_POINT_SERVICE "org.gnome.UPnP.MediaServer2"
#define ENTRY_POINT_PATH    "/org/gnome/UPnP/MediaServer2"

static gchar **args;
static DBusGProxy *gproxy = NULL;
static GHashTable *registered_sources = NULL;

static GOptionEntry entries[] = {
  { G_OPTION_REMAINING, '\0', 0,
    G_OPTION_ARG_FILENAME_ARRAY, &args,
    "Grilo module to load",
    NULL },
  { NULL }
};

/* Request and register a name in dbus */
static void
dbus_register_name (const gchar *name)
{
  GError *error = NULL;
  guint request_name_result;

  if (!org_freedesktop_DBus_request_name (gproxy,
                                          name,
                                          DBUS_NAME_FLAG_DO_NOT_QUEUE,
                                          &request_name_result,
                                          &error)) {
    g_warning ("Unable to register \"%s\" name in dbus: %s",
               name,
               error->message);
    g_error_free (error);
  }
}

/* Release a name in dbus */
static void
dbus_unregister_name (const gchar *name)
{
  GError *error = NULL;
  guint request_name_result;

  if(!org_freedesktop_DBus_release_name (gproxy,
                                         name,
                                         &request_name_result,
                                         &error)) {
    g_debug ("Unable to unregister \"%s\" name in dbus: %s",
             name,
             error->message);
    g_error_free (error);
  }
}

/* Fix invalid characters so string can be used in a dbus name */
static void
sanitize (gchar *string)
{
  gint i;
  g_return_if_fail (string);

  i=0;
  while (string[i]) {
    switch (string[i]) {
    case '-':
    case ':':
      string[i] = '_';
    break;
    }
    i++;
  }
}

/* Callback invoked whenever a new source comes up */
static void
source_added_cb (GrlPluginRegistry *registry, gpointer user_data)
{
  GrlSupportedOps supported_ops;
  RygelGriloMediaServer *server;
  gchar *dbus_path;
  gchar *dbus_service;
  gchar *source_id;

  /* Only sources that implement browse and metadata are of interest */
  supported_ops =
    grl_metadata_source_supported_operations (GRL_METADATA_SOURCE (user_data));
  if (supported_ops & GRL_OP_BROWSE &&
      supported_ops & GRL_OP_METADATA) {

    /* Register a new service name */
    source_id =
      g_strdup (grl_metadata_source_get_id (GRL_METADATA_SOURCE (user_data)));

    g_debug ("Registering %s source", source_id);

    sanitize (source_id);
    dbus_service = g_strconcat (ENTRY_POINT_SERVICE ".", source_id, NULL);
    dbus_path = g_strconcat (ENTRY_POINT_PATH "/", source_id, NULL);
    g_free (source_id);
    dbus_register_name (dbus_service);
    g_free (dbus_service);

    server = rygel_grilo_media_server_new (dbus_path,
                                           GRL_MEDIA_SOURCE (user_data));
    if (!server) {
      g_warning ("Cannot register %s", source_id);
    } else {
      g_hash_table_insert (registered_sources, dbus_path, server);
    }
  } else {
    g_debug ("%s source does not support either browse or metadata",
             grl_metadata_source_get_id (GRL_METADATA_SOURCE (user_data)));
  }
}

/* Callback invoked whenever a new source goes away */
static void
source_removed_cb (GrlPluginRegistry *registry, gpointer user_data)
{
  gchar *dbus_name;
  gchar *dbus_path;
  gchar *source_id;

  source_id =
    g_strdup (grl_metadata_source_get_id (GRL_METADATA_SOURCE (user_data)));
  sanitize (source_id);

  dbus_name = g_strconcat (ENTRY_POINT_SERVICE ".", source_id, NULL);
  dbus_unregister_name (dbus_name);
  g_free (dbus_name);

  /* Remove source */
  dbus_path = g_strconcat (ENTRY_POINT_PATH "/", source_id, NULL);
  g_hash_table_remove (registered_sources, dbus_path);
  g_free (dbus_path);
}

/* Main program */
gint
main (gint argc, gchar **argv)
{
  DBusGConnection *connection;
  GError *error = NULL;
  GOptionContext *context = NULL;
  GrlPluginRegistry *registry;
  gint i;

  g_type_init ();

  context = g_option_context_new ("- run Grilo plugin as UPnP service");
  g_option_context_add_main_entries (context, entries, NULL);
  g_option_context_parse (context, &argc, &argv, &error);
  g_option_context_free (context);

  if (error) {
    g_printerr ("Invalid arguments, %s\n", error->message);
    g_clear_error (&error);
    return -1;
  }

  /* Get DBus */
  connection = dbus_g_bus_get (DBUS_BUS_SESSION, &error);
  if (!connection) {
    g_printerr ("Could not connect to session bus, %s\n", error->message);
    g_clear_error (&error);
    return -1;
  }

  gproxy = dbus_g_proxy_new_for_name (connection,
                                      DBUS_SERVICE_DBUS,
                                      DBUS_PATH_DBUS,
                                      DBUS_INTERFACE_DBUS);

  /* Initialize registered sources table */
  registered_sources = g_hash_table_new_full (g_str_hash,
                                              g_str_equal,
                                              g_free,
                                              g_object_unref);

  /* Load grilo plugins */
  registry = grl_plugin_registry_get_instance ();

  g_signal_connect (registry, "source-added",
                    G_CALLBACK (source_added_cb), NULL);
  g_signal_connect (registry, "source-removed",
                    G_CALLBACK (source_removed_cb), NULL);

  if (!args || !args[0]) {
    grl_plugin_registry_load_all (registry);
  } else {
    for (i = 0; args[i]; i++) {
      grl_plugin_registry_load (registry, args[i]);
    }
  }
  g_main_loop_run (g_main_loop_new (NULL, FALSE));
}
