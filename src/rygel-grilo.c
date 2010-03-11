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

#include "rygel-grilo-media-container.h"

#define ENTRY_POINT_SERVICE "org.gnome.UPnP.MediaServer1"
#define ENTRY_POINT_PATH    "/org/gnome/UPnP/MediaServer1"

#define DEFAULT_LIMIT 5

static gint limit;
static gchar **args;
static DBusGProxy *gproxy = NULL;

static GOptionEntry entries[] = {
  { "limit", 'l', 0,
    G_OPTION_ARG_INT, &limit,
    "Limit max. results per container",
    NULL },
  { G_OPTION_REMAINING, '\0', 0,
    G_OPTION_ARG_FILENAME_ARRAY, &args,
    "Grilo module to load",
    NULL },
  { NULL }
};


static void
dbus_register_name (const gchar *name)
{
  GError *error = NULL;
  guint request_name_result;
  g_assert(org_freedesktop_DBus_request_name (gproxy,
                                              name,
                                              DBUS_NAME_FLAG_DO_NOT_QUEUE,
                                              &request_name_result,
                                              &error));
}

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

static void
get_root_cb (GrlMediaSource *source,
             GrlMedia *media,
             gpointer user_data,
             const GError *error)
{
  gchar *dbus_path = (gchar *) user_data;

  g_assert (media);

  /* WORKAROUND: THIS MUST BE FIXED IN GRILO */
  g_object_ref (media);

  rygel_grilo_media_container_new_root (dbus_path, media, limit);

  g_free (dbus_path);
}

static void
source_added_cb (GrlPluginRegistry *registry, gpointer user_data)
{
  GList *keys;
  GrlSupportedOps supported_ops;
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

    /* Get root to put in dbus */
    keys = grl_metadata_key_list_new (GRL_METADATA_KEY_TITLE, NULL);

    grl_media_source_metadata (GRL_MEDIA_SOURCE (user_data),
                               NULL,
                               keys,
                               GRL_RESOLVE_FULL,
                               get_root_cb,
                               dbus_path);

    g_list_free (keys);
  } else {
    g_debug ("%s source does not support either browse or metadata",
             grl_metadata_source_get_id (GRL_METADATA_SOURCE (user_data)));
  }
}

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

  /* Check limit */
  if (limit == 0) {
    limit = DEFAULT_LIMIT;
  } else if (limit < 0) {
    limit = G_MAXINT;
  }

  /* Get DBus */
  connection = dbus_g_bus_get (DBUS_BUS_SESSION, &error);
  g_assert (connection);

  gproxy = dbus_g_proxy_new_for_name (connection,
                                      DBUS_SERVICE_DBUS,
                                      DBUS_PATH_DBUS,
                                      DBUS_INTERFACE_DBUS);

  /* Load grilo plugins */
  registry = grl_plugin_registry_get_instance ();

  g_signal_connect (registry, "source-added",
                    G_CALLBACK (source_added_cb), NULL);

  if (!args || !args[0]) {
    grl_plugin_registry_load_all (registry);
  } else {
    for (i = 0; args[i]; i++) {
      grl_plugin_registry_load (registry, args[i]);
    }
  }

  g_main_loop_run (g_main_loop_new (NULL, FALSE));
}
