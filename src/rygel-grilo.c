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

static GOptionEntry entries[] = {
  { G_OPTION_REMAINING, '\0', 0,
    G_OPTION_ARG_FILENAME_ARRAY, &args,
    "Grilo module to load",
    NULL },
  { NULL }
};


static void
dbus_register_name (DBusGProxy *gproxy,
                    const gchar *name)
{
  GError *error = NULL;
  guint request_name_result;
  g_assert(org_freedesktop_DBus_request_name (gproxy,
                                              name,
                                              DBUS_NAME_FLAG_DO_NOT_QUEUE,
                                              &request_name_result,
                                              &error));
  g_assert(request_name_result == DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER);
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
      string[i] = '_';
      break;
    }
    i++;
  }
}

gint
main (gint argc, gchar **argv)
{
  DBusGConnection *connection;
  DBusGProxy *gproxy;
  GError *error = NULL;
  GOptionContext *context = NULL;
  GrlMediaPlugin **sources;
  GrlMediaSource *grilo_source;
  GrlPluginRegistry *registry;
  gchar *dbus_path;
  gchar *dbus_service;
  gchar *source_id;

  g_type_init ();

  context = g_option_context_new ("- run Grilo plugin as UPnP service");
  g_option_context_add_main_entries (context, entries, NULL);
  g_option_context_parse (context, &argc, &argv, &error);

  if (error) {
    g_printerr ("Invalid arguments, %s\n", error->message);
    g_clear_error (&error);
    return -1;
  }

  if (!args || !args[0]) {
    g_print ("%s\n", g_option_context_get_help (context, FALSE, NULL));
    return -1;
  }

  g_option_context_free (context);

  /* Load grilo plugin */
  registry = grl_plugin_registry_get_instance ();
  grl_plugin_registry_load (registry, args[0]);

  /* Get source */
  sources = grl_plugin_registry_get_sources_by_capabilities (registry,
                                                             GRL_OP_METADATA |
                                                             GRL_OP_BROWSE,
                                                             FALSE);
  if (!sources[0]) {
    g_printerr ("Did not found any browsable source");
    return -1;
  }

  grilo_source = GRL_MEDIA_SOURCE (sources[0]);

  /* Get DBus name */
  connection = dbus_g_bus_get (DBUS_BUS_SESSION, &error);
  g_assert (connection);

  gproxy = dbus_g_proxy_new_for_name (connection,
                                      DBUS_SERVICE_DBUS,
                                      DBUS_PATH_DBUS,
                                      DBUS_INTERFACE_DBUS);

  source_id =
    g_strdup (grl_metadata_source_get_id (GRL_METADATA_SOURCE (grilo_source)));
  sanitize (source_id);

  dbus_service = g_strconcat (ENTRY_POINT_SERVICE ".",
                              source_id,
                              NULL);

  dbus_path = g_strconcat (ENTRY_POINT_PATH "/",
                           source_id,
                           NULL);
  g_free (source_id);

  dbus_register_name (gproxy, dbus_service);

  rygel_grilo_media_server_new (dbus_path, grilo_source);

  g_free (dbus_service);
  g_free (dbus_path);

  g_main_loop_run (g_main_loop_new (NULL, FALSE));
}
