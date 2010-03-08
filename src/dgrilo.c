#include <stdio.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-bindings.h>

#include "dgrilo-media-object.h"
#include "dgrilo-media-container.h"

#define ENTRY_POINT_SERVICE "org.gnome.UPnP.MediaServer1"
#define ENTRY_POINT_PATH    "/org/gnome/UPnP/MediaServer1"

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

static void
get_root_cb (GrlMediaSource *source,
             GrlMedia *media,
             gpointer user_data,
             const GError *error)
{
  gchar *dbus_path = (gchar *) user_data;
  DGriloMediaContainer *c;

  g_assert (media);

  /* WORKAROUND: THIS MUST BE FIXED IN GRILO */
  g_object_ref (media);

  c = dgrilo_media_container_new_root (dbus_path, media);
  printf ("Created object at %p\n", c);

  g_free (dbus_path);
}

gint
main (gint argc, gchar **argv)
{
  DBusGConnection *connection;
  DBusGProxy *gproxy;
  GError *error = NULL;
  GList *keys;
  GrlMediaSource *jamendo;
  GrlPluginRegistry *registry;
  gchar *dbus_path;
  gchar *dbus_service;
  gchar *source_id;

  g_type_init ();

  /* Load jamendo grilo plugin */
  registry = grl_plugin_registry_get_instance ();
  grl_plugin_registry_load (registry,
                            "/home/jasuarez/Projects/grilo/grilo-plugins/src/jamendo/.libs/libgrljamendo.so");

  jamendo =
    (GrlMediaSource *) grl_plugin_registry_lookup_source (registry,
                                                          "grl-jamendo");

  g_assert (jamendo);


  /* Get DBus name */
  connection = dbus_g_bus_get (DBUS_BUS_SESSION, &error);
  g_assert (connection);

  gproxy = dbus_g_proxy_new_for_name (connection,
                                      DBUS_SERVICE_DBUS,
                                      DBUS_PATH_DBUS,
                                      DBUS_INTERFACE_DBUS);

  source_id = g_strdup (grl_metadata_source_get_id (GRL_METADATA_SOURCE (jamendo)));
  sanitize (source_id);

  dbus_service = g_strconcat (ENTRY_POINT_SERVICE ".",
                              source_id,
                              NULL);
  dbus_path = g_strconcat (ENTRY_POINT_PATH "/",
                           source_id,
                           NULL);
  g_free (source_id);

  dbus_register_name (gproxy, dbus_service);
  g_free (dbus_service);

  /* Get root */
  keys = grl_metadata_key_list_new (GRL_METADATA_KEY_TITLE,
                                    NULL);

  grl_media_source_metadata (jamendo, NULL, keys, GRL_RESOLVE_FAST_ONLY, get_root_cb, dbus_path);

  g_main_loop_run (g_main_loop_new (NULL, FALSE));
}
