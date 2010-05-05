#include <media-server2-client.h>
#include <media-server2-observer.h>
#include <glib.h>
#include <string.h>

static const gchar *properties[] = { MS2_PROP_PATH,
				     MS2_PROP_DISPLAY_NAME,
                                     MS2_PROP_PARENT,
                                     MS2_PROP_CHILD_COUNT,
                                     NULL };

static void
children_reply (GObject *source,
                GAsyncResult *res,
                gpointer user_data)
{
  GError *error = NULL;
  GList *children;
  GList *child;
  gchar *provider = (gchar *) user_data;

  children = ms2_client_get_children_finish (MS2_CLIENT (source), res, &error);

  g_print ("\n* Provider '%s'\n", provider);
  g_free (provider);

  if (!children) {
    g_print ("\tDid not get any child, %s\n", error? error->message: "no error");
    return;
  }

  for (child = children; child; child = g_list_next (child)) {
    g_print ("\t* '%s', '%s'\n",
             ms2_client_get_path (child->data),
             ms2_client_get_display_name(child->data));
  }

  g_list_foreach (children, (GFunc) g_hash_table_unref, NULL);
  g_list_free (children);
  g_object_unref (source);
}

static void
test_children_async ()
{
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

    ms2_client_get_children_async (client,
                                   MS2_ROOT,
                                   0,
                                   -1,
                                   properties,
                                   children_reply,
                                   g_strdup (*provider));
  }

  g_strfreev (providers);
}

static void
properties_reply (GObject *source,
                  GAsyncResult *res,
                  gpointer user_data)
{
  GError *error = NULL;
  GHashTable *result;
  GValue *v;
  const gchar **p;
  gchar *provider = (gchar *) user_data;

  result = ms2_client_get_properties_finish (MS2_CLIENT (source), res, &error);

  g_print ("\n* Provider '%s'\n", provider);
  g_free (provider);

  if (!result) {
    g_print ("\tDid not get any property, %s\n", error? error->message: "no error");
    return;
  }

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
  g_object_unref (source);
}

static void
test_properties_async ()
{
  MS2Client *client;
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

    ms2_client_get_properties_async (client,
                                     MS2_ROOT,
                                     properties,
                                     properties_reply,
                                     g_strdup (*provider));
  }

  g_strfreev (providers);
}

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

    g_print ("\n* Provider '%s'\n", *provider);
    if (!result) {
      g_print ("\tDid not get any property, %s\n", error? error->message: "no error");
      return;
    }

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

    g_print ("\n* Provider '%s'\n", *provider);
    if (!children) {
      g_print ("\tDid not get any child, %s\n", error? error->message: "no error");
      return;
    }

    for (child = children; child; child = g_list_next (child)) {
      g_print ("\t* '%s', '%s'\n",
               ms2_client_get_path (child->data),
               ms2_client_get_display_name(child->data));
    }

    g_list_foreach (children, (GFunc) g_hash_table_unref, NULL);
    g_list_free (children);
    g_object_unref (client);
  }

  g_strfreev (providers);
}

static void
destroy_cb (MS2Client *client, gpointer user_data)
{
  g_print ("End of provider %s\n", ms2_client_get_provider_name(client));
}

static void
new_cb (MS2Observer *observer, const gchar *provider, gpointer user_data)
{
  MS2Client *client;

  client = ms2_client_new (provider);
  if (!client) {
    g_printerr ("Unable to create client for %s\n", provider);
  } else {
    g_print ("New provider %s\n", provider);
    g_signal_connect (client, "destroy", G_CALLBACK (destroy_cb), NULL);
  }
}

static void
test_provider_free ()
{
  MS2Client *client;
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
      continue;
    }

    g_print ("Provider %s\n", *provider);
    g_signal_connect (G_OBJECT (client), "destroy", G_CALLBACK (destroy_cb), NULL);
  }

  g_strfreev (providers);
}

static void
test_dynamic_providers ()
{
  MS2Client *client;
  MS2Observer *observer;
  gchar **provider;
  gchar **providers;

  observer = ms2_observer_get_instance ();
  if (!observer) {
    g_printerr ("Unable to get the observer\n");
    return;
  }

  g_signal_connect (observer, "new", G_CALLBACK (new_cb), NULL);

  providers = ms2_client_get_providers ();
  if (!providers) {
    g_print ("There is no MediaServer2 providers\n");
    return;
  }

  for (provider = providers; *provider; provider++) {
    client = ms2_client_new (*provider);
    if (!client) {
      g_printerr ("Unable to create a client for %s\n", *provider);
      continue;
    }

    g_print ("New provider %s\n", *provider);
    g_signal_connect (client, "destroy", G_CALLBACK (destroy_cb), NULL);
  }

  g_strfreev (providers);
}

int main (int argc, char **argv)
{
  GMainLoop *mainloop;

  g_type_init ();

  if (0) test_properties_sync ();
  if (0) test_children_sync ();
  if (0) test_properties_async ();
  if (0) test_children_async ();
  if (0) test_provider_free ();
  if (1) test_dynamic_providers ();

  mainloop = g_main_loop_new (NULL, FALSE);
  g_main_loop_run (mainloop);
  g_main_loop_unref (mainloop);
}
