#include <media-server2-client.h>
#include <glib.h>
#include <string.h>

int main (int argc, char **argv)
{
  gchar **providers;
  MS2Client *client;

  g_type_init ();

  providers = ms2_client_get_providers ();
  if (!providers) {
    g_print ("There is no MediaServer2 provider\n");
    return 0;
  }

  client = ms2_client_new (providers[0]);
}
