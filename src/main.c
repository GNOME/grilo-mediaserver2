#include <stdio.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-bindings.h>

#include "dgrilo-media-object.h"
#include "dgrilo-media-container.h"

#define DGRILO_NAME "org.gnome.UPnP.MediaServer1.DGrilo"
#define DGRILO_PATH "/org/gnome/UPnP/MediaServer1/DGrilo"

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
  DGriloMediaContainer *c;
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

  c = dgrilo_media_container_new_with_dbus_path (DGRILO_PATH, NULL);
  printf ("Created object at %p\n", c);

  /* c = dgrilo_media_container_new_with_parent (DGRILO_MEDIA_OBJECT (c), */
  /*                                             NULL); */
  /* printf ("Created antother object at %p\n", c); */

  g_main_loop_run (g_main_loop_new (NULL, FALSE));
}
