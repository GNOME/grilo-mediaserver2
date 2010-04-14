#include <media-server2-client.h>
#include <glib.h>
#include <string.h>

static  const gchar *properties[] = { MS2_PROP_DISPLAY_NAME,
                                      MS2_PROP_PARENT,
                                      MS2_PROP_CHILD_COUNT,
                                      NULL };

static void
test_properties_sync ()
{
  GError *error = NULL;
  GHashTable *result;
  GValue *v;
  MS2Client *client;
  const gchar **p;
  gchar **provider;
  gchar **providers;

  providers = ms2_client_get_providers ();

  if (!providers) {
    g_print ("There is no MediaServer2 provider\n");
    return;
  }

  for (provider = providers; *provider; provider++) {
    client = ms2_client_new (*provider);

    if (!client) {
      g_printerr ("Unable to create a client\n");
      return;
    }

    result = ms2_client_get_properties (client, MS2_ROOT, properties, &error);

    if (!result) {
      g_print ("Did not get any property, %s\n", error->message);
      return;
    }

    g_print ("\n* Provider '%s'\n", *provider);
    for (p = properties; *p; p++) {
      v = g_hash_table_lookup (result, *p);
      if (v && G_VALUE_HOLDS_INT (v)) {
        g_print ("\t* '%s' value: '%d'\n", *p, g_value_get_int (v));
      } else if (v && G_VALUE_HOLDS_STRING (v)) {
        g_print ("\t* '%s' value: '%s'\n", *p, g_value_get_string (v));
      } else {
        g_print ("\t* '%s' value: ---\n", *p);
      }
    }
    g_hash_table_unref (result);
    g_object_unref (client);
  }

  g_strfreev (providers);
}

static void
test_children_sync ()
{
  GError *error = NULL;
  GList *children;
  GList *child;
  MS2Client *client;
  gchar **provider;
  gchar **providers;

  providers = ms2_client_get_providers ();

  if (!providers) {
    g_print ("There is no MediaServer2 provider\n");
    return;
  }

  for (provider = providers; *provider; provider ++) {
    client = ms2_client_new (*provider);

    if (!client) {
      g_printerr ("Unable to create a client\n");
      return;
    }

    children  = ms2_client_get_children (client, MS2_ROOT, 0, -1, properties, &error);

    if (!children) {
      g_print ("Did not get any child, %s\n", error->message);
      return;
    }

    g_print ("\n* Provider '%s'\n", *provider);
    for (child = children; child; child = g_list_next (child)) {
      g_print ("\t* '%s', '%s'\n",
               g_value_get_string(g_hash_table_lookup (child->data, MS2_PROP_ID)),
               g_value_get_string(g_hash_table_lookup (child->data, MS2_PROP_DISPLAY_NAME)));
    }

    g_list_foreach (children, (GFunc) g_hash_table_unref, NULL);
    g_list_free (children);
    g_object_unref (client);
  }

  g_strfreev (providers);
}


int main (int argc, char **argv)
{
  g_type_init ();

  if (0) test_properties_sync ();
  if (1) test_children_sync ();
}
