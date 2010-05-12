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

#include <media-server1-server.h>
#include <media-server1-client.h>

#define RYGEL_GRILO_CONFIG_FILE "rygel-grilo.conf"

#define ID_PREFIX_AUDIO     "gra://"
#define ID_PREFIX_CONTAINER "grc://"
#define ID_PREFIX_IMAGE     "gri://"
#define ID_PREFIX_VIDEO     "grv://"
#define ID_SEPARATOR        "/"

static GHashTable *servers = NULL;
static GList *providers_names = NULL;
static GrlPluginRegistry *registry = NULL;

static gboolean dups;
static gchar **args = NULL;
static gchar *conffile = NULL;
static gint limit = 0;

static GOptionEntry entries[] = {
  { "config-file", 'c', 0,
    G_OPTION_ARG_STRING, &conffile,
    "Use this config file",
    NULL },
  { "allow-duplicates", 'D', 0,
    G_OPTION_ARG_NONE, &dups,
    "Allow more than one provider with same name",
    NULL },
  { "limit", 'l', 0,
    G_OPTION_ARG_INT, &limit,
    "Limit max. number of children for Items/Containers ( 0 = unlimited)",
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
  MS1Server *server;
  gboolean updated;
  GList *other_keys;
  gchar *parent_id;
} RygelGriloData;

static GHashTable *
get_properties_cb (MS1Server *server,
                   const gchar *id,
                   const gchar **properties,
                   gpointer data,
                   GError **error);

static GList *
get_children_cb (MS1Server *server,
                 const gchar *id,
                 guint offset,
                 guint max_count,
                 const gchar **properties,
                 gpointer data,
                 GError **error);

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

  if (g_strcmp0 (child_id, MS1_ROOT) == 0) {
    return NULL;
  }

  parent_end = g_strrstr (child_id, ID_SEPARATOR);
  bytes_to_copy = parent_end - child_id;

  /* Check if parent is a root */
  if (bytes_to_copy < 6) {
    return g_strdup (MS1_ROOT);
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

  if (g_strcmp0 (ms_id, MS1_ROOT) == 0) {
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
  const gchar *media_id;
  gchar *escaped_id;
  gchar *ms_id;

  media_id = grl_media_get_id (media);

  /* Check if it is root media */
  if (!media_id) {
    return g_strdup (MS1_ROOT);
  }

  escaped_id = g_uri_escape_string (media_id, NULL, TRUE);

  if (g_strcmp0 (parent_id, MS1_ROOT) == 0) {
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

  if (g_strcmp0 (id, MS1_ROOT) == 0 ||
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

static void
get_items_and_containers (MS1Server *server,
                          GrlMediaSource *source,
                          const gchar *container_id,
                          GList **items,
                          guint *item_count,
                          GList **containers,
                          guint *container_count)
{
  const gchar *properties[] = { MS1_PROP_PATH, MS1_PROP_TYPE, NULL };
  GList *children;
  GList *child;
  MS1ItemType type;

  /* Initialize values */
  if (items) {
    *items = NULL;
  }

  if (item_count) {
    *item_count = 0;
  }

  if (containers) {
    *containers = NULL;
  }

  if (container_count) {
    *container_count = 0;
  }

  children =
    get_children_cb (server, container_id, 0, (guint) limit, properties, source, NULL);

  /* Separate containers from items */
  for (child = children; child; child = g_list_next (child)) {
    type = ms1_client_get_item_type (child->data);
    if (type == MS1_ITEM_TYPE_CONTAINER) {
      if (container_count) {
        (*container_count)++;
      }
      if (containers) {
        *containers = g_list_prepend (*containers, child->data);
      } else {
        g_hash_table_unref (child->data);
      }
    } else {
      if (item_count) {
        (*item_count)++;
      }
      if (items) {
        *items = g_list_prepend (*items, child->data);
      }
    }
  }
  if (containers) {
    *containers = g_list_reverse (*containers);
  }
  if (items) {
    *items = g_list_reverse (*items);
  }
  g_list_free (children);
}

/* Given a null-terminated array of MediaServerSpec1 properties, returns a list
   with the corresponding Grilo metadata keys */
static GList *
get_grilo_keys (const gchar **ms_keys, GList **other_keys)
{
  GList *grl_keys = NULL;
  gint i;

  for (i = 0; ms_keys[i]; i++) {
    if (g_strcmp0 (ms_keys[i], MS1_PROP_PATH) == 0) {
      grl_keys = g_list_prepend (grl_keys,
                                 GRLKEYID_TO_POINTER (GRL_METADATA_KEY_ID));
    } else if (g_strcmp0 (ms_keys[i], MS1_PROP_DISPLAY_NAME) == 0) {
      grl_keys = g_list_prepend (grl_keys,
                                 GRLKEYID_TO_POINTER (GRL_METADATA_KEY_TITLE));
    } else if (g_strcmp0 (ms_keys[i], MS1_PROP_DATE) == 0) {
      grl_keys = g_list_prepend (grl_keys,
                                 GRLKEYID_TO_POINTER (GRL_METADATA_KEY_DATE));
    } else if (g_strcmp0 (ms_keys[i], MS1_PROP_ALBUM) == 0) {
      grl_keys = g_list_prepend (grl_keys,
                                 GRLKEYID_TO_POINTER (GRL_METADATA_KEY_ALBUM));
    } else if (g_strcmp0 (ms_keys[i], MS1_PROP_ARTIST) == 0) {
      grl_keys = g_list_prepend (grl_keys,
                                 GRLKEYID_TO_POINTER (GRL_METADATA_KEY_ARTIST));
    } else if (g_strcmp0 (ms_keys[i], MS1_PROP_GENRE) == 0) {
      grl_keys = g_list_prepend (grl_keys,
                                 GRLKEYID_TO_POINTER (GRL_METADATA_KEY_GENRE));
    } else if (g_strcmp0 (ms_keys[i], MS1_PROP_MIME_TYPE) == 0) {
      grl_keys = g_list_prepend (grl_keys,
                                 GRLKEYID_TO_POINTER (GRL_METADATA_KEY_MIME));
    } else if (g_strcmp0 (ms_keys[i], MS1_PROP_URLS) == 0) {
      grl_keys = g_list_prepend (grl_keys,
                                 GRLKEYID_TO_POINTER (GRL_METADATA_KEY_URL));
    } else if (g_strcmp0 (ms_keys[i], MS1_PROP_BITRATE) == 0) {
      grl_keys = g_list_prepend (grl_keys,
                                 GRLKEYID_TO_POINTER (GRL_METADATA_KEY_BITRATE));
    } else if (g_strcmp0 (ms_keys[i], MS1_PROP_DURATION) == 0) {
      grl_keys = g_list_prepend (grl_keys,
                                 GRLKEYID_TO_POINTER (GRL_METADATA_KEY_DURATION));
    } else if (g_strcmp0 (ms_keys[i], MS1_PROP_HEIGHT) == 0) {
      grl_keys = g_list_prepend (grl_keys,
                                 GRLKEYID_TO_POINTER (GRL_METADATA_KEY_HEIGHT));
    } else if (g_strcmp0 (ms_keys[i], MS1_PROP_WIDTH) == 0) {
      grl_keys = g_list_prepend (grl_keys,
                                 GRLKEYID_TO_POINTER (GRL_METADATA_KEY_WIDTH));
    } else if (g_strcmp0 (ms_keys[i], MS1_PROP_PARENT) == 0 && other_keys) {
      *other_keys = g_list_prepend (*other_keys, (gchar *) ms_keys[i]);
    } else if (g_strcmp0 (ms_keys[i], MS1_PROP_TYPE) == 0 && other_keys) {
      *other_keys = g_list_prepend (*other_keys, (gchar *) ms_keys[i]);
    } else if (g_strcmp0 (ms_keys[i], MS1_PROP_ITEMS) == 0 && other_keys) {
      *other_keys = g_list_prepend (*other_keys, (gchar *) ms_keys[i]);
    } else if (g_strcmp0 (ms_keys[i], MS1_PROP_ITEM_COUNT) == 0 && other_keys) {
      *other_keys = g_list_prepend (*other_keys, (gchar *) ms_keys[i]);
    } else if (g_strcmp0 (ms_keys[i], MS1_PROP_CONTAINERS) == 0 && other_keys) {
      *other_keys = g_list_prepend (*other_keys, (gchar *) ms_keys[i]);
    } else if (g_strcmp0 (ms_keys[i], MS1_PROP_CONTAINER_COUNT) == 0 && other_keys) {
      *other_keys = g_list_prepend (*other_keys, (gchar *) ms_keys[i]);
    } else if (g_strcmp0 (ms_keys[i], MS1_PROP_SEARCHABLE) == 0 && other_keys) {
      *other_keys = g_list_prepend (*other_keys, (gchar *) ms_keys[i]);
    }
  }

  return grl_keys;
}

static void
fill_properties_table (MS1Server *server,
                       GHashTable *properties_table,
                       GList *keys,
                       GrlMedia *media,
                       const gchar *parent_id)
{
  GList *prop;
  GrlKeyID key;
  gchar *id;
  gchar *urls[2] = { 0 };

  for (prop = keys; prop; prop = g_list_next (prop)) {
    key = POINTER_TO_GRLKEYID (prop->data);
    if (key == GRL_METADATA_KEY_ID ||
        grl_data_key_is_known (GRL_DATA (media), key)) {
      switch (key) {
      case GRL_METADATA_KEY_ID:
        id = serialize_media (parent_id, media);
        if (id) {
          ms1_server_set_path (server,
                               properties_table,
                               id,
                               GRL_IS_MEDIA_BOX (media));
          g_free (id);
        }
        break;
      case GRL_METADATA_KEY_TITLE:
        ms1_server_set_display_name (server,
                                     properties_table,
                                     grl_media_get_title (media));
        break;
      case GRL_METADATA_KEY_ALBUM:
        ms1_server_set_album (server,
                              properties_table,
                              grl_data_get_string (GRL_DATA (media),
                                                   GRL_METADATA_KEY_ALBUM));
        break;
      case GRL_METADATA_KEY_ARTIST:
        ms1_server_set_artist (server,
                               properties_table,
                               grl_data_get_string (GRL_DATA (media),
                                                    GRL_METADATA_KEY_ARTIST));
        break;
      case GRL_METADATA_KEY_GENRE:
        ms1_server_set_genre (server,
                              properties_table,
                              grl_data_get_string (GRL_DATA (media),
                                                   GRL_METADATA_KEY_GENRE));
        break;
      case GRL_METADATA_KEY_MIME:
        ms1_server_set_mime_type (server,
                                  properties_table,
                                  grl_media_get_mime (media));
        break;
      case GRL_METADATA_KEY_URL:
        urls[0] = (gchar *) grl_media_get_url (media);
        ms1_server_set_urls (server, properties_table, urls);
        break;
      case GRL_METADATA_KEY_BITRATE:
        ms1_server_set_bitrate (server,
                                properties_table,
                                grl_data_get_int (GRL_DATA (media),
                                                  GRL_METADATA_KEY_BITRATE));
        break;
      case GRL_METADATA_KEY_DURATION:
        ms1_server_set_duration (server,
                                 properties_table,
                                 grl_media_get_duration (media));
        break;
      case GRL_METADATA_KEY_HEIGHT:
        ms1_server_set_height (server,
                               properties_table,
                               grl_data_get_int (GRL_DATA (media),
                                                 GRL_METADATA_KEY_HEIGHT));
        break;
      case GRL_METADATA_KEY_WIDTH:
        ms1_server_set_width (server,
                              properties_table,
                              grl_data_get_int (GRL_DATA (media),
                                                GRL_METADATA_KEY_WIDTH));
        break;
      }
    }
  }
}

static void
fill_other_properties_table (MS1Server *server,
                             GrlMediaSource *source,
                             GHashTable *properties_table,
                             GList *keys,
                             GrlMedia *media,
                             const gchar *parent_id)
{
  GList **containers = NULL;
  GList **items = NULL;
  GList *_containers;
  GList *_items;
  GList *key;
  gchar *id;
  guint *container_count = NULL;
  guint *item_count = NULL;
  guint _container_count;
  guint _item_count;

  for (key = keys; key; key = g_list_next (key)) {
    if (g_strcmp0 (key->data, MS1_PROP_PARENT) == 0 && parent_id) {
      ms1_server_set_parent (server,
                             properties_table,
                             parent_id);
    } else if (g_strcmp0 (key->data, MS1_PROP_TYPE) == 0) {
      if (GRL_IS_MEDIA_BOX (media)) {
        ms1_server_set_item_type (server,
                                  properties_table,
                                  MS1_ITEM_TYPE_CONTAINER);
      } else if (GRL_IS_MEDIA_IMAGE (media)) {
        ms1_server_set_item_type (server,
                                  properties_table,
                                  MS1_ITEM_TYPE_IMAGE);
      } else if (GRL_IS_MEDIA_AUDIO (media)) {
        ms1_server_set_item_type (server,
                                  properties_table,
                                  MS1_ITEM_TYPE_AUDIO);
      } else if (GRL_IS_MEDIA_VIDEO (media)) {
        ms1_server_set_item_type (server,
                                  properties_table,
                                  MS1_ITEM_TYPE_VIDEO);
      } else {
        ms1_server_set_item_type (server,
                                  properties_table,
                                  MS1_ITEM_TYPE_UNKNOWN);
      }
    } else if (g_strcmp0 (key->data, MS1_PROP_ITEMS) == 0) {
      items = &_items;
    } else if (g_strcmp0 (key->data, MS1_PROP_ITEM_COUNT) == 0) {
      item_count = &_item_count;
    } else if (g_strcmp0 (key->data, MS1_PROP_CONTAINERS) == 0) {
      containers = &_containers;
    } else if (g_strcmp0 (key->data, MS1_PROP_CONTAINER_COUNT) == 0) {
      container_count = &_container_count;
    } else if (g_strcmp0 (key->data, MS1_PROP_SEARCHABLE) == 0) {
      ms1_server_set_searchable (server,
                                 properties_table,
                                 FALSE);
    }
  }

  if (items || item_count || containers || container_count) {
    id = serialize_media (parent_id, media);
    if (id) {
      get_items_and_containers (server,
                                source,
                                id,
                                items,
                                item_count,
                                containers,
                                container_count);
      g_free (id);
    }
    if (items) {
      ms1_server_set_items (server, properties_table, *items);
      g_list_foreach (*items, (GFunc) g_hash_table_unref, NULL);
      g_list_free (*items);
    }
    if (item_count) {
      ms1_server_set_item_count (server, properties_table, *item_count);
    }
    if (containers) {
      ms1_server_set_containers (server, properties_table, *containers);
      g_list_foreach (*containers, (GFunc) g_hash_table_unref, NULL);
      g_list_free (*containers);
    }
    if (container_count) {
      ms1_server_set_container_count (server, properties_table, *container_count);
    }
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

  rgdata->properties = ms1_server_new_properties_hashtable ();
  fill_properties_table (rgdata->server,
                         rgdata->properties,
                         rgdata->keys,
                         media,
                         rgdata->parent_id);

  fill_other_properties_table (rgdata->server,
                               source,
                               rgdata->properties,
                               rgdata->other_keys,
                               media,
                               rgdata->parent_id);

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

  if (error) {
    rgdata->error = g_error_copy (error);
    rgdata->updated = TRUE;
    return;
  }

  if (media) {
    prop_table = ms1_server_new_properties_hashtable ();
    fill_properties_table (rgdata->server,
                           prop_table,
                           rgdata->keys,
                           media,
                           rgdata->parent_id);
    fill_other_properties_table (rgdata->server,
                                 source,
                                 prop_table,
                                 rgdata->other_keys,
                                 media,
                                 rgdata->parent_id);
    rgdata->children = g_list_prepend (rgdata->children, prop_table);
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
get_properties_cb (MS1Server *server,
                   const gchar *id,
                   const gchar **properties,
                   gpointer data,
                   GError **error)
{
  GHashTable *properties_table = NULL;
  GrlMedia *media;
  RygelGriloData *rgdata;

  rgdata = g_slice_new0 (RygelGriloData);
  rgdata->server = g_object_ref (server);
  rgdata->source = (GrlMediaSource *) data;
  rgdata->keys = get_grilo_keys (properties, &rgdata->other_keys);
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
  } else {
    properties_table = rgdata->properties;
  }

  g_object_unref (media);
  g_list_free (rgdata->keys);
  g_list_free (rgdata->other_keys);
  g_free (rgdata->parent_id);
  g_object_unref (rgdata->server);
  g_slice_free (RygelGriloData, rgdata);

  return properties_table;
}

static GList *
get_children_cb (MS1Server *server,
                 const gchar *id,
                 guint offset,
                 guint max_count,
                 const gchar **properties,
                 gpointer data,
                 GError **error)
{
  GList *children;
  GrlMedia *media;
  RygelGriloData *rgdata;

  rgdata = g_slice_new0 (RygelGriloData);
  rgdata->server = g_object_ref (server);
  rgdata->source = (GrlMediaSource *) data;
  rgdata->keys = get_grilo_keys (properties, &rgdata->other_keys);
  rgdata->parent_id = g_strdup (id);
  media = unserialize_media (GRL_METADATA_SOURCE (rgdata->source), id);

  grl_media_source_browse (rgdata->source,
                           media,
                           rgdata->keys,
                           offset,
                           max_count == 0? G_MAXINT: max_count,
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
  g_list_free (rgdata->other_keys);
  g_free (rgdata->parent_id);
  g_object_unref (rgdata->server);
  g_slice_free (RygelGriloData, rgdata);

  return children;
}

/* Callback invoked whenever a new source comes up */
static void
source_added_cb (GrlPluginRegistry *registry, gpointer user_data)
{
  GrlSupportedOps supported_ops;
  MS1Server *server;
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

    server = ms1_server_new (source_id, GRL_MEDIA_SOURCE (user_data));

    if (!server) {
      g_warning ("Cannot register %s", source_id);
      g_free (source_id);
    } else {
      ms1_server_set_get_properties_func (server, get_properties_cb);
      ms1_server_set_get_children_func (server, get_children_cb);
      /* Save reference */
      if (!dups) {
        providers_names = g_list_prepend (providers_names,
                                          g_strdup(source_name));
      }
      g_hash_table_insert (servers, source_id, server);
    }
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
  gchar *source_id;

  source_name =
    grl_metadata_source_get_name (GRL_METADATA_SOURCE (user_data));
  source_id =
    g_strdup (grl_metadata_source_get_id (GRL_METADATA_SOURCE (user_data)));

  if (!dups) {
    entry = g_list_find_custom (providers_names,
                                source_name,
                                (GCompareFunc) g_strcmp0);
    if (entry) {
      g_free (entry->data);
      providers_names = g_list_delete_link (providers_names, entry);
    }
  }

  sanitize (source_id);
  g_hash_table_remove (servers, source_id);
  g_free (source_id);
}

/* Load plugins configuration */
static void
load_config ()
{
  GError *error = NULL;
  GKeyFile *keyfile;
  GrlConfig *config;
  gboolean load_success;
  gchar **key;
  gchar **keys;
  gchar **plugin;
  gchar **plugins;
  gchar **search_paths;
  gchar *value;

  keyfile = g_key_file_new ();

  /* Try first user defined config file */
  if (conffile){
    load_success = g_key_file_load_from_file (keyfile,
                                              conffile,
                                              G_KEY_FILE_NONE,
                                              &error);
  } else {
    search_paths = g_new0 (gchar *, 3);
    search_paths[0] = g_build_filename (g_get_user_config_dir (),
                                        "rygel-grilo",
                                        NULL);
    search_paths[1] = g_strdup (SYSCONFDIR);
    load_success = g_key_file_load_from_dirs (keyfile,
                                              RYGEL_GRILO_CONFIG_FILE,
                                              (const gchar **) search_paths,
                                              NULL,
                                              G_KEY_FILE_NONE,
                                              &error);
    g_strfreev (search_paths);
  }

  if (!load_success) {
    g_warning ("Unable to load configuration. %s", error->message);
    g_error_free (error);
    g_key_file_free (keyfile);
    return;
  }

  /* Look up for defined plugins */
  plugins = g_key_file_get_groups (keyfile, NULL);
  for (plugin = plugins; *plugin; plugin++) {
    config = grl_config_new (*plugin, NULL);

    /* Look up for keys in this plugin */
    keys = g_key_file_get_keys (keyfile, *plugin, NULL, NULL);
    for (key = keys; *key; key++) {
      value = g_key_file_get_string (keyfile, *plugin, *key, NULL);
      if (value) {
        grl_config_set_string (config, *key, value);
        g_free (value);
      }
    }
    grl_plugin_registry_add_config (registry, config);
    g_strfreev (keys);
  }

  g_strfreev (plugins);
  g_key_file_free (keyfile);
}

/* Main program */
gint
main (gint argc, gchar **argv)
{
  GError *error = NULL;
  GOptionContext *context = NULL;
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

  /* Adjust limit */
  limit = CLAMP (limit, 0, G_MAXINT);

  /* Load grilo plugins */
  registry = grl_plugin_registry_get_instance ();
  if (!registry) {
    g_printerr ("Unable to load Grilo registry\n");
    return -1;
  }

  /* Load configuration */
  load_config ();

  /* Initialize <grilo-plugin, ms1-server> pairs */
  servers = g_hash_table_new_full (g_str_hash,
                                   g_str_equal,
                                   g_free,
                                   g_object_unref);

  g_signal_connect (registry, "source-added",
                    G_CALLBACK (source_added_cb), NULL);

  g_signal_connect (registry, "source-removed",
                    G_CALLBACK (source_removed_cb), NULL);

  if (!args || !args[0]) {
    grl_plugin_registry_load_all (registry);
  } else {
    for (i = 0; args[i]; i++) {
      grl_plugin_registry_load (registry, args[i]);
    }
  }

  g_main_loop_run (g_main_loop_new (NULL, FALSE));
}
