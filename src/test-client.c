#include <media-server2-client.h>
#include <glib.h>

int main (int argc, char **argv)
{
  gchar **providers;

  g_type_init ();

  providers = ms2_client_get_providers ();
}
