#include <stdio.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-bindings.h>

#include "dgrilo-media-object.h"
#include "dgrilo-media-object-glue.h"

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

gint
main (gint argc, gchar **argv)
{
  DBusGConnection *connection;
  DBusGProxy *gproxy;
  GError *error = NULL;

  g_type_init ();

  DGriloMediaObject *obj =
    dgrilo_media_object_new ("my parent", "pretty title");

  printf ("Created object at %p\n", obj);

  connection = dbus_g_bus_get (DBUS_BUS_SESSION, &error);
  g_assert (connection);

  gproxy = dbus_g_proxy_new_for_name (connection,
                                      DBUS_SERVICE_DBUS,
                                      DBUS_PATH_DBUS,
                                      DBUS_INTERFACE_DBUS);

  dbus_register_name (gproxy, "org.gnome.UPnP.MediaObject1.DGrilo");
  dbus_g_object_type_install_info (DGRILO_MEDIA_OBJECT_TYPE,
                                   &dbus_glib_dgrilo_media_object_object_info);

  dbus_g_connection_register_g_object (connection,
                                       "/org/gnome/UPnP/MediaObject1/DGrilo",
                                       G_OBJECT (obj));

  g_main_loop_run (g_main_loop_new (NULL, FALSE));
}
