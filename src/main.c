#include <stdio.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-bindings.h>

#include "dgrilo-media-object.h"

#define DGRILO_NAME "org.gnome.UPnP.MediaObject1.DGrilo"

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
  DGriloMediaObject *obj;
  GError *error = NULL;

  g_type_init ();

  /* Get DBus name */
  connection = dbus_g_bus_get (DBUS_BUS_SESSION, &error);
  g_assert (connection);

  gproxy = dbus_g_proxy_new_for_name (connection,
                                      DBUS_SERVICE_DBUS,
                                      DBUS_PATH_DBUS,
                                      DBUS_INTERFACE_DBUS);

  dbus_register_name (gproxy, DGRILO_NAME);

  obj = dgrilo_media_object_new ("my parent", "pretty title");
  printf ("Created object at %p\n", obj);

  obj = dgrilo_media_object_new (NULL, "this should be the parent");
  printf ("Created another object at %p\n", obj);

  g_main_loop_run (g_main_loop_new (NULL, FALSE));
}
