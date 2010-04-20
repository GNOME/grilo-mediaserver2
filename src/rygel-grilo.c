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

#include <dbus/dbus-glib-bindings.h>
#include <dbus/dbus-glib.h>
#include <grilo.h>
#include <stdio.h>

#include <media-server2-server.h>

#define ID_PREFIX_AUDIO     "gra://"
#define ID_PREFIX_CONTAINER "grc://"
#define ID_PREFIX_IMAGE     "gri://"
#define ID_PREFIX_VIDEO     "grv://"
#define ID_SEPARATOR        "/"

static GList *providers_names = NULL;

static gchar **args;
static gboolean dups;

static GOptionEntry entries[] = {
  { "allow-duplicates", 'D', 0,
    G_OPTION_ARG_NONE, &dups,
    "Allow more than one provider with same name",
    NULL },
  { G_OPTION_REMAINING, '\0', 0,
    G_OPTION_ARG_FILENAME_ARRAY, &args,
    "Grilo module to load",
    NULL },
  { NULL }
};

typedef struct {
  GError *error;
  GHashTable *properties;
  GList *children;
  GList *keys;
  GrlMediaSource *source;
  gboolean updated;
  gchar *parent_id;
} RygelGriloData;

/* Fix invalid characters so string can be used in a dbus name */
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

/* Returns the rygel-grilo parent id of the child */
static gchar *
get_parent_id (const gchar *child_id)
{
  gchar *parent_end;
  gchar *parent_id;
  gsize bytes_to_copy;

  if (g_strcmp0 (child_id, MS2_ROOT) == 0) {
    return NULL;
  }

  parent_end = g_strrstr (child_id, ID_SEPARATOR);
  bytes_to_copy = parent_end - child_id;

  /* Check if parent is a root */
  if (bytes_to_copy < 6) {
    return g_strdup (MS2_ROOT);
  }

  /* Save parent id */
  parent_id = g_strndup (child_id, bytes_to_copy);

  /* Parent should be always a container */
  parent_id[2] = 'c';

  return parent_id;
}

static gchar *
get_grl_id (const gchar *ms_id)
{
  gchar **offspring;
  gchar *grl_id;

  if (g_strcmp0 (ms_id, MS2_ROOT) == 0) {
    return NULL;
  }

  /* Skip gr?:// prefix */
  ms_id += 6;

  offspring = g_strsplit (ms_id, ID_SEPARATOR, -1);

  /* Last token is the searched id; first tokens are the family */
  grl_id = g_uri_unescape_string (offspring[g_strv_length (offspring) - 1],
                                  NULL);

  g_strfreev (offspring);

  return grl_id;
}

static gchar *
serialize_media (const gchar *parent_id,
                 GrlMedia *media)
{
  gchar *escaped_id;
  gchar *ms_id;

  escaped_id = g_uri_escape_string (grl_media_get_id (media), NULL, TRUE);

  if (g_strcmp0 (parent_id, MS2_ROOT) == 0) {
    ms_id = g_strconcat (ID_PREFIX_CONTAINER, escaped_id, NULL);
  } else {
    ms_id = g_strconcat (parent_id, ID_SEPARATOR, escaped_id, NULL);
  }

  g_free (escaped_id);

  /* Parent id should be of grc:// type; adjust the prefix to the new content */
  if (GRL_IS_MEDIA_AUDIO (media)) {
    ms_id[2] = 'a';
  } else if (GRL_IS_MEDIA_VIDEO (media)) {
    ms_id[2] = 'v';
  } else if (GRL_IS_MEDIA_IMAGE (media)) {
    ms_id[2] = 'i';
  }

  return ms_id;
}

static GrlMedia *
unserialize_media (GrlMetadataSource *source, const gchar *id)
{
  GrlMedia *media = NULL;
  gchar *grl_id;

  if (g_strcmp0 (id, MS2_ROOT) == 0 ||
      g_str_has_prefix (id, ID_PREFIX_CONTAINER)) {
    media = grl_media_box_new ();
  } else if (g_str_has_prefix (id, ID_PREFIX_AUDIO)) {
    media = grl_media_audio_new ();
  } else if (g_str_has_prefix (id, ID_PREFIX_VIDEO)) {
    media = grl_media_video_new ();
  } else if (g_str_has_prefix (id, ID_PREFIX_IMAGE)) {
    media = grl_media_image_new ();
  }

  grl_media_set_source (media, grl_metadata_source_get_id (source));
  grl_id = get_grl_id (id);
  if (grl_id) {
    grl_media_set_id (media, grl_id);
    g_free (grl_id);
  }

  return media;
}

/* Given a null-terminated array of MediaServerSpec2 properties, returns a list
   with the corresponding Grilo metadata keys */
static GList *
get_grilo_keys (const gchar **ms_keys)
{
  GList *grl_keys = NULL;
  gint i;

  for (i = 0; ms_keys[i]; i++) {
    if (g_strcmp0 (ms_keys[i], MS2_PROP_DISPLAY_NAME) == 0) {
      grl_keys = g_list_append (grl_keys,
                                GRLKEYID_TO_POINTER (GRL_METADATA_KEY_TITLE));
    } else if (g_strcmp0 (ms_keys[i], MS2_PROP_ALBUM) == 0) {
      grl_keys = g_list_append (grl_keys,
                                GRLKEYID_TO_POINTER (GRL_METADATA_KEY_ALBUM));
    } else if (g_strcmp0 (ms_keys[i], MS2_PROP_ARTIST) == 0) {
      grl_keys = g_list_append (grl_keys,
                                GRLKEYID_TO_POINTER (GRL_METADATA_KEY_ARTIST));
    } else if (g_strcmp0 (ms_keys[i], MS2_PROP_GENRE) == 0) {
      grl_keys = g_list_append (grl_keys,
                                GRLKEYID_TO_POINTER (GRL_METADATA_KEY_GENRE));
    } else if (g_strcmp0 (ms_keys[i], MS2_PROP_MIME_TYPE) == 0) {
      grl_keys = g_list_append (grl_keys,
                                GRLKEYID_TO_POINTER (GRL_METADATA_KEY_MIME));
    } else if (g_strcmp0 (ms_keys[i], MS2_PROP_CHILD_COUNT) == 0) {
      grl_keys = g_list_append (grl_keys,
                                GRLKEYID_TO_POINTER (GRL_METADATA_KEY_CHILDCOUNT));
    } else if (g_strcmp0 (ms_keys[i], MS2_PROP_URLS) == 0) {
      grl_keys = g_list_append (grl_keys,
                                GRLKEYID_TO_POINTER (GRL_METADATA_KEY_URL));
    } else if (g_strcmp0 (ms_keys[i], MS2_PROP_BITRATE) == 0) {
      grl_keys = g_list_append (grl_keys,
                                GRLKEYID_TO_POINTER (GRL_METADATA_KEY_BITRATE));
    } else if (g_strcmp0 (ms_keys[i], MS2_PROP_DURATION) == 0) {
      grl_keys = g_list_append (grl_keys,
                                GRLKEYID_TO_POINTER (GRL_METADATA_KEY_DURATION));
    } else if (g_strcmp0 (ms_keys[i], MS2_PROP_HEIGHT) == 0) {
      grl_keys = g_list_append (grl_keys,
                                GRLKEYID_TO_POINTER (GRL_METADATA_KEY_HEIGHT));
    } else if (g_strcmp0 (ms_keys[i], MS2_PROP_WIDTH) == 0) {
      grl_keys = g_list_append (grl_keys,
                                GRLKEYID_TO_POINTER (GRL_METADATA_KEY_WIDTH));
    }
  }

  return grl_keys;
}

static void
fill_properties_table (GHashTable *properties_table,
                       GList *keys,
                       GrlMedia *media,
                       const gchar *parent_id)
{
  GList *prop;
  GrlKeyID key;
  gchar *urls[2] = { 0 };

  for (prop = keys; prop; prop = g_list_next (prop)) {
    key = POINTER_TO_GRLKEYID (prop->data);
    if (grl_data_key_is_known (GRL_DATA (media), key)) {
      switch (key) {
      case GRL_METADATA_KEY_TITLE:
        ms2_server_set_display_name (properties_table,
                                     grl_media_get_title (media));
        break;
      case GRL_METADATA_KEY_ALBUM:
        ms2_server_set_album (properties_table,
                              grl_data_get_string (GRL_DATA (media),
                                                   GRL_METADATA_KEY_ALBUM));
        break;
      case GRL_METADATA_KEY_ARTIST:
        ms2_server_set_artist (properties_table,
                               grl_data_get_string (GRL_DATA (media),
                                                    GRL_METADATA_KEY_ARTIST));
        break;
      case GRL_METADATA_KEY_GENRE:
        ms2_server_set_genre (properties_table,
                              grl_data_get_string (GRL_DATA (media),
                                                   GRL_METADATA_KEY_GENRE));
        break;
      case GRL_METADATA_KEY_MIME:
        ms2_server_set_mime_type (properties_table,
                                  grl_media_get_mime (media));
        break;
      case GRL_METADATA_KEY_CHILDCOUNT:
        ms2_server_set_child_count (properties_table,
                                    grl_data_get_int (GRL_DATA (media),
                                                      GRL_METADATA_KEY_CHILDCOUNT));
        break;
      case GRL_METADATA_KEY_URL:
        urls[0] = (gchar *) grl_media_get_url (media);
        ms2_server_set_urls (properties_table, urls);
        break;
      case GRL_METADATA_KEY_BITRATE:
        ms2_server_set_bitrate (properties_table,
                                grl_data_get_int (GRL_DATA (media),
                                                  GRL_METADATA_KEY_BITRATE));
        break;
      case GRL_METADATA_KEY_DURATION:
        ms2_server_set_duration (properties_table,
                                 grl_media_get_duration (media));
        break;
      case GRL_METADATA_KEY_HEIGHT:
        ms2_server_set_height (properties_table,
                               grl_data_get_int (GRL_DATA (media),
                                                 GRL_METADATA_KEY_HEIGHT));
        break;
      case GRL_METADATA_KEY_WIDTH:
        ms2_server_set_width (properties_table,
                              grl_data_get_int (GRL_DATA (media),
                                                GRL_METADATA_KEY_WIDTH));
        break;
      }
    }
  }

  if (parent_id) {
    ms2_server_set_parent (properties_table, parent_id);
  }
}

static void
metadata_cb (GrlMediaSource *source,
             GrlMedia *media,
             gpointer user_data,
             const GError *error)
{
  RygelGriloData *rgdata = (RygelGriloData *) user_data;

  if (error) {
    rgdata->error = g_error_copy (error);
    rgdata->updated = TRUE;
    return;
  }

  fill_properties_table (rgdata->properties, rgdata->keys, media, rgdata->parent_id);

  rgdata->updated = TRUE;
}

static void
browse_cb (GrlMediaSource *source,
           guint browse_id,
           GrlMedia *media,
           guint remaining,
           gpointer user_data,
           const GError *error)
{
  GHashTable *prop_table;
  RygelGriloData *rgdata = (RygelGriloData *) user_data;
  gchar *id;

  if (error) {
    rgdata->error = g_error_copy (error);
    rgdata->updated = TRUE;
    return;
  }

  if (media) {
    id = serialize_media (rgdata->parent_id, media);
    if (id) {
      prop_table = ms2_server_new_properties_hashtable (id);
      fill_properties_table (prop_table, rgdata->keys, media, rgdata->parent_id);
      rgdata->children = g_list_prepend (rgdata->children, prop_table);
      g_free (id);
    }
  }

  if (!remaining) {
    rgdata->children = g_list_reverse (rgdata->children);
    rgdata->updated = TRUE;
  }
}

static void
wait_for_result (RygelGriloData *rgdata)
{
  GMainLoop *mainloop;
  GMainContext *mainloop_context;

  mainloop = g_main_loop_new (NULL, TRUE);
  mainloop_context = g_main_loop_get_context (mainloop);
  while (!rgdata->updated) {
    g_main_context_iteration (mainloop_context, TRUE);
  }

  g_main_loop_unref (mainloop);
}

static GHashTable *
get_properties_cb (const gchar *id,
                   const gchar **properties,
                   gpointer data,
                   GError **error)
{
  GHashTable *properties_table = NULL;
  GrlMedia *media;
  RygelGriloData *rgdata;

  rgdata = g_slice_new0 (RygelGriloData);
  rgdata->source = (GrlMediaSource *) data;
  rgdata->properties = ms2_server_new_properties_hashtable (id);
  rgdata->keys = get_grilo_keys (properties);
  rgdata->parent_id = get_parent_id (id);
  media = unserialize_media (GRL_METADATA_SOURCE (rgdata->source), id);

  if (rgdata->keys) {
    grl_media_source_metadata (rgdata->source,
                               media,
                               rgdata->keys,
                               GRL_RESOLVE_FULL | GRL_RESOLVE_IDLE_RELAY,
                               metadata_cb,
                               rgdata);
  } else {
    metadata_cb (rgdata->source, media, rgdata, NULL);
  }

  wait_for_result (rgdata);

  if (rgdata->error) {
    if (error) {
      *error = rgdata->error;
    }
    g_hash_table_unref (rgdata->properties);
  } else {
    properties_table = rgdata->properties;
  }

  g_object_unref (media);
  g_list_free (rgdata->keys);
  g_free (rgdata->parent_id);
  g_slice_free (RygelGriloData, rgdata);

  return properties_table;
}

static GList *
get_children_cb (const gchar *id,
                 guint offset,
                 gint max_count,
                 const gchar **properties,
                 gpointer data,
                 GError **error)
{
  GList *children;
  GrlMedia *media;
  RygelGriloData *rgdata;

  rgdata = g_slice_new0 (RygelGriloData);
  rgdata->source = (GrlMediaSource *) data;
  rgdata->keys = get_grilo_keys (properties);
  rgdata->parent_id = g_strdup (id);
  media = unserialize_media (GRL_METADATA_SOURCE (rgdata->source), id);

  grl_media_source_browse (rgdata->source,
                           media,
                           rgdata->keys,
                           offset,
                           max_count < 0? G_MAXINT: max_count,
                           GRL_RESOLVE_FULL | GRL_RESOLVE_IDLE_RELAY,
                           browse_cb,
                           rgdata);

  wait_for_result (rgdata);

  if (rgdata->error) {
    if (error) {
      *error = rgdata->error;
    }
    g_list_foreach (rgdata->children, (GFunc) g_hash_table_unref, NULL);
    g_list_free (rgdata->children);
    children = NULL;
  } else {
    children = rgdata->children;
  }

  g_object_unref (media);
  g_list_free (rgdata->keys);
  g_free (rgdata->parent_id);
  g_slice_free (RygelGriloData, rgdata);

  return children;
}

/* Callback invoked whenever a new source comes up */
static void
source_added_cb (GrlPluginRegistry *registry, gpointer user_data)
{
  GrlSupportedOps supported_ops;
  MS2Server *server;
  const gchar *source_name;
  gchar *source_id;

  /* Only sources that implement browse and metadata are of interest */
  supported_ops =
    grl_metadata_source_supported_operations (GRL_METADATA_SOURCE (user_data));
  if (supported_ops & GRL_OP_BROWSE &&
      supported_ops & GRL_OP_METADATA) {

    source_id =
      (gchar *) grl_metadata_source_get_id (GRL_METADATA_SOURCE (user_data));

    /* Check if there is already another provider with the same name */
    if (!dups) {
      source_name =
        grl_metadata_source_get_name (GRL_METADATA_SOURCE (user_data));
      if (g_list_find_custom (providers_names,
                              source_name,
                              (GCompareFunc) g_strcmp0)) {
        g_debug ("Skipping %s [%s] source", source_id, source_name);
        return;
      }
    }

    /* Register a new service name */
    source_id = g_strdup (source_id);

    g_debug ("Registering %s [%s] source", source_id, source_name);

    sanitize (source_id);

    server = ms2_server_new (source_id, GRL_MEDIA_SOURCE (user_data));

    if (!server) {
      g_warning ("Cannot register %s", source_id);
    } else {
      ms2_server_set_get_properties_func (server, get_properties_cb);
      ms2_server_set_get_children_func (server, get_children_cb);
      /* Save reference */
      if (!dups) {
        providers_names = g_list_prepend (providers_names,
                                          g_strdup(source_name));
      }
    }
    g_free (source_id);
  } else {
    g_debug ("%s source does not support either browse or metadata",
             grl_metadata_source_get_id (GRL_METADATA_SOURCE (user_data)));
  }
}

/* Callback invoked whenever a source goes away */
static void
source_removed_cb (GrlPluginRegistry *registry, gpointer user_data)
{
  GList *entry;
  const gchar *source_name;

  source_name =
    grl_metadata_source_get_name (GRL_METADATA_SOURCE (user_data));
  entry = g_list_find_custom (providers_names,
                              source_name,
                              (GCompareFunc) g_strcmp0);
  if (entry) {
    g_free (entry->data);
    providers_names = g_list_delete_link (providers_names, entry);
  }
}

/* Main program */
gint
main (gint argc, gchar **argv)
{
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

  /* Load grilo plugins */
  registry = grl_plugin_registry_get_instance ();

  g_signal_connect (registry, "source-added",
                    G_CALLBACK (source_added_cb), NULL);

  if (!dups) {
    g_signal_connect (registry, "source-removed",
                      G_CALLBACK (source_removed_cb), NULL);
  }

  if (!args || !args[0]) {
    grl_plugin_registry_load_all (registry);
  } else {
    for (i = 0; args[i]; i++) {
      grl_plugin_registry_load (registry, args[i]);
    }
  }

  g_main_loop_run (g_main_loop_new (NULL, FALSE));
}
