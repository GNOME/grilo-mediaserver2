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
#include <media-server2-client.h>

#define RYGEL_GRILO_CONFIG_FILE "rygel-grilo.conf"

#define grl_media_set_rygel_grilo_parent(media, parent) \
  grl_data_set_string(GRL_DATA(media),                  \
    GRL_METADATA_KEY_RYGEL_GRILO_PARENT,\
                      (parent))

#define grl_media_get_rygel_grilo_parent(media)                 \
  grl_data_get_string(GRL_DATA(media),                          \
                      GRL_METADATA_KEY_RYGEL_GRILO_PARENT)

static GHashTable *servers = NULL;
static GList *providers_names = NULL;
static GrlPluginRegistry *registry = NULL;

static GrlKeyID GRL_METADATA_KEY_RYGEL_GRILO_PARENT = NULL;

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
    "Limit max. number of children (0 = unlimited)",
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
  MS2Server *server;
  gboolean updated;
  GList *other_keys;
  gchar *parent_id;
  guint offset;
  guint count;
  guint operation_id;
  ListType list_type;
} RygelGriloData;

static GHashTable *
get_properties_cb (MS2Server *server,
                   const gchar *id,
                   const gchar **properties,
                   gpointer data,
                   GError **error);

static GList *
list_children_cb (MS2Server *server,
                  const gchar *id,
                  ListType list_type,
                  guint offset,
                  guint max_count,
                  const gchar **properties,
                  gpointer data,
                  GError **error);

static GList *
search_objects_cb (MS2Server *server,
                   const gchar *id,
                   const gchar *query,
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

static gchar *
serialize_media (GrlMedia *media)
{
  static GList *serialize_keys = NULL;
  const gchar *media_id;

  if (!serialize_keys) {
    serialize_keys =
      grl_metadata_key_list_new (GRL_METADATA_KEY_RYGEL_GRILO_PARENT,
                                 NULL);
  }

  media_id = grl_media_get_id (media);

  /* Check if it is root media */
  if (!media_id) {
    return g_strdup (MS2_ROOT);
  } else {
    return grl_media_serialize_extended (media,
                                         GRL_MEDIA_SERIALIZE_PARTIAL,
                                         serialize_keys);
  }
}

static GrlMedia *
unserialize_media (GrlMetadataSource *source, const gchar *serial)
{
  GrlMedia *media;
  /* gchar *parent_serial; */

  if (g_strcmp0 (serial, MS2_ROOT) == 0) {
    /* Root container must be built from scratch */
    media = grl_media_box_new ();
    grl_media_set_source (media, grl_metadata_source_get_id (source));

    /* Set parent to itself */
    /* parent_serial = grl_media_serialize (media); */
    /* grl_media_set_rygel_grilo_parent (media, parent_serial); */
    /* g_free (parent_serial); */
    grl_media_set_rygel_grilo_parent (media, MS2_ROOT);
  } else {
    media = grl_media_unserialize (serial);
  }

  return media;
}

/* Given a null-terminated array of MediaServerSpec2 properties, returns a list
   with the corresponding Grilo metadata keys */
static GList *
get_grilo_keys (const gchar **ms_keys, GList **other_keys)
{
  GList *grl_keys = NULL;
  gint i;

  /* Check if all keys are requested */
  if (g_strcmp0 (ms_keys[0], MS2_PROP_ALL) == 0) {
    grl_keys = grl_plugin_registry_get_metadata_keys (registry);
    if (other_keys) {
      *other_keys = g_list_prepend (*other_keys, MS2_PROP_CHILD_COUNT);
      *other_keys = g_list_prepend (*other_keys, MS2_PROP_TYPE);
      *other_keys = g_list_prepend (*other_keys, MS2_PROP_ITEM_COUNT);
      *other_keys = g_list_prepend (*other_keys, MS2_PROP_CONTAINER_COUNT);
      *other_keys = g_list_prepend (*other_keys, MS2_PROP_SEARCHABLE);
    }
  } else {
    for (i = 0; ms_keys[i]; i++) {
      if (g_strcmp0 (ms_keys[i], MS2_PROP_PATH) == 0) {
        grl_keys = g_list_prepend (grl_keys, GRL_METADATA_KEY_ID);
      } else if (g_strcmp0 (ms_keys[i], MS2_PROP_DISPLAY_NAME) == 0) {
        grl_keys = g_list_prepend (grl_keys, GRL_METADATA_KEY_TITLE);
      } else if (g_strcmp0 (ms_keys[i], MS2_PROP_DATE) == 0) {
        grl_keys = g_list_prepend (grl_keys, GRL_METADATA_KEY_DATE);
      } else if (g_strcmp0 (ms_keys[i], MS2_PROP_ALBUM) == 0) {
        grl_keys = g_list_prepend (grl_keys, GRL_METADATA_KEY_ALBUM);
      } else if (g_strcmp0 (ms_keys[i], MS2_PROP_ARTIST) == 0) {
        grl_keys = g_list_prepend (grl_keys, GRL_METADATA_KEY_ARTIST);
      } else if (g_strcmp0 (ms_keys[i], MS2_PROP_GENRE) == 0) {
        grl_keys = g_list_prepend (grl_keys, GRL_METADATA_KEY_GENRE);
      } else if (g_strcmp0 (ms_keys[i], MS2_PROP_MIME_TYPE) == 0) {
        grl_keys = g_list_prepend (grl_keys, GRL_METADATA_KEY_MIME);
      } else if (g_strcmp0 (ms_keys[i], MS2_PROP_URLS) == 0) {
        grl_keys = g_list_prepend (grl_keys, GRL_METADATA_KEY_URL);
      } else if (g_strcmp0 (ms_keys[i], MS2_PROP_BITRATE) == 0) {
        grl_keys = g_list_prepend (grl_keys, GRL_METADATA_KEY_BITRATE);
      } else if (g_strcmp0 (ms_keys[i], MS2_PROP_DURATION) == 0) {
        grl_keys = g_list_prepend (grl_keys, GRL_METADATA_KEY_DURATION);
      } else if (g_strcmp0 (ms_keys[i], MS2_PROP_HEIGHT) == 0) {
        grl_keys = g_list_prepend (grl_keys, GRL_METADATA_KEY_HEIGHT);
      } else if (g_strcmp0 (ms_keys[i], MS2_PROP_WIDTH) == 0) {
        grl_keys = g_list_prepend (grl_keys, GRL_METADATA_KEY_WIDTH);
      } else if (g_strcmp0 (ms_keys[i], MS2_PROP_PARENT) == 0) {
        grl_keys = g_list_prepend (grl_keys, GRL_METADATA_KEY_RYGEL_GRILO_PARENT);
      } else if (g_strcmp0 (ms_keys[i], MS2_PROP_CHILD_COUNT) == 0 && other_keys) {
        *other_keys = g_list_prepend (*other_keys, (gchar *) ms_keys[i]);
      } else if (g_strcmp0 (ms_keys[i], MS2_PROP_ITEM_COUNT) == 0 && other_keys) {
        *other_keys = g_list_prepend (*other_keys, (gchar *) ms_keys[i]);
      } else if (g_strcmp0 (ms_keys[i], MS2_PROP_CONTAINER_COUNT) == 0 && other_keys) {
        *other_keys = g_list_prepend (*other_keys, (gchar *) ms_keys[i]);
      } else if (g_strcmp0 (ms_keys[i], MS2_PROP_TYPE) == 0 && other_keys) {
        *other_keys = g_list_prepend (*other_keys, (gchar *) ms_keys[i]);
      } else if (g_strcmp0 (ms_keys[i], MS2_PROP_SEARCHABLE) == 0 && other_keys) {
        *other_keys = g_list_prepend (*other_keys, (gchar *) ms_keys[i]);
      }
    }
  }

  return grl_keys;
}

static void
fill_properties_table (MS2Server *server,
                       GHashTable *properties_table,
                       GList *keys,
                       GrlMedia *media)
{
  GList *prop;
  gchar *id;
  gchar *urls[2] = { 0 };

  for (prop = keys; prop; prop = g_list_next (prop)) {
    if (prop->data == GRL_METADATA_KEY_ID ||
        grl_data_has_key (GRL_DATA (media), prop->data)) {
      if (prop->data == GRL_METADATA_KEY_ID) {
        id = serialize_media (media);
        if (id) {
          ms2_server_set_path (server,
                               properties_table,
                               id,
                               GRL_IS_MEDIA_BOX (media));
          g_free (id);
        }
      } else if (prop->data == GRL_METADATA_KEY_TITLE) {
        ms2_server_set_display_name (server,
                                     properties_table,
                                     grl_media_get_title (media));
      } else if (prop->data == GRL_METADATA_KEY_ALBUM) {
        ms2_server_set_album (server,
                              properties_table,
                              grl_data_get_string (GRL_DATA (media),
                                                   GRL_METADATA_KEY_ALBUM));
      } else if (prop->data == GRL_METADATA_KEY_ARTIST) {
        ms2_server_set_artist (server,
                               properties_table,
                               grl_data_get_string (GRL_DATA (media),
                                                    GRL_METADATA_KEY_ARTIST));
      } else if (prop->data == GRL_METADATA_KEY_GENRE) {
        ms2_server_set_genre (server,
                              properties_table,
                              grl_data_get_string (GRL_DATA (media),
                                                   GRL_METADATA_KEY_GENRE));
      } else if (prop->data == GRL_METADATA_KEY_MIME) {
        ms2_server_set_mime_type (server,
                                  properties_table,
                                  grl_media_get_mime (media));
      } else if (prop->data == GRL_METADATA_KEY_URL) {
        urls[0] = (gchar *) grl_media_get_url (media);
        ms2_server_set_urls (server, properties_table, urls);
      } else if (prop->data == GRL_METADATA_KEY_BITRATE) {
        ms2_server_set_bitrate (server,
                                properties_table,
                                grl_data_get_int (GRL_DATA (media),
                                                  GRL_METADATA_KEY_BITRATE));
      } else if (prop->data == GRL_METADATA_KEY_DURATION) {
        ms2_server_set_duration (server,
                                 properties_table,
                                 grl_media_get_duration (media));
      } else if (prop->data == GRL_METADATA_KEY_HEIGHT) {
        ms2_server_set_height (server,
                               properties_table,
                               grl_data_get_int (GRL_DATA (media),
                                                 GRL_METADATA_KEY_HEIGHT));
      } else if (prop->data == GRL_METADATA_KEY_WIDTH) {
        ms2_server_set_width (server,
                              properties_table,
                              grl_data_get_int (GRL_DATA (media),
                                                GRL_METADATA_KEY_WIDTH));
      } else if (prop->data == GRL_METADATA_KEY_RYGEL_GRILO_PARENT) {
        if (grl_media_get_id (media) == NULL) {
          ms2_server_set_parent (server,
                                 properties_table,
                                 MS2_ROOT);
        } else {
          ms2_server_set_parent (server,
                                 properties_table,
                                 grl_media_get_rygel_grilo_parent (media));
        }
      }
    }
  }
}

static void
fill_other_properties_table (MS2Server *server,
                             GrlMediaSource *source,
                             GHashTable *properties_table,
                             GList *keys,
                             GrlMedia *media)
{
  GList *key;
  gint childcount;

  /* Compute childcount */
  if (GRL_IS_MEDIA_BOX (media)) {
    childcount = grl_media_box_get_childcount (GRL_MEDIA_BOX (media));
    if (childcount == GRL_METADATA_KEY_CHILDCOUNT_UNKNOWN) {
      childcount = limit;
    }
  } else {
    childcount = 0;
  }

  for (key = keys; key; key = g_list_next (key)) {
    if (g_strcmp0 (key->data, MS2_PROP_TYPE) == 0) {
      if (GRL_IS_MEDIA_BOX (media)) {
        ms2_server_set_item_type (server,
                                  properties_table,
                                  MS2_ITEM_TYPE_CONTAINER);
      } else if (GRL_IS_MEDIA_IMAGE (media)) {
        ms2_server_set_item_type (server,
                                  properties_table,
                                  MS2_ITEM_TYPE_IMAGE);
      } else if (GRL_IS_MEDIA_AUDIO (media)) {
        ms2_server_set_item_type (server,
                                  properties_table,
                                  MS2_ITEM_TYPE_AUDIO);
      } else if (GRL_IS_MEDIA_VIDEO (media)) {
        ms2_server_set_item_type (server,
                                  properties_table,
                                  MS2_ITEM_TYPE_VIDEO);
      } else {
        ms2_server_set_item_type (server,
                                  properties_table,
                                  MS2_ITEM_TYPE_UNKNOWN);
      }
    } else if (g_strcmp0 (key->data, MS2_PROP_CHILD_COUNT) == 0) {
      ms2_server_set_child_count (server, properties_table, childcount);
    } else if (g_strcmp0 (key->data, MS2_PROP_ITEM_COUNT) == 0) {
      ms2_server_set_item_count (server, properties_table, childcount);
    } else if (g_strcmp0 (key->data, MS2_PROP_CONTAINER_COUNT) == 0) {
      ms2_server_set_container_count (server, properties_table, childcount);
    } else if (g_strcmp0 (key->data, MS2_PROP_SEARCHABLE) == 0) {
      /* Only supports search in the root level */
      if (grl_media_get_id (media) == NULL &&
          grl_metadata_source_supported_operations (GRL_METADATA_SOURCE (source)) & GRL_OP_SEARCH) {
        ms2_server_set_searchable (server,
                                   properties_table,
                                   TRUE);
      } else {
        ms2_server_set_searchable (server,
                                   properties_table,
                                   FALSE);
      }
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

  rgdata->properties = ms2_server_new_properties_hashtable ();
  fill_properties_table (rgdata->server,
                         rgdata->properties,
                         rgdata->keys,
                         media);

  fill_other_properties_table (rgdata->server,
                               source,
                               rgdata->properties,
                               rgdata->other_keys,
                               media);

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
  gboolean add_media = FALSE;

  if (error) {
    rgdata->error = g_error_copy (error);
    rgdata->updated = TRUE;
    return;
  }

  if (media) {
    if ((rgdata->list_type == LIST_ITEMS && !GRL_IS_MEDIA_BOX (media)) ||
        (rgdata->list_type == LIST_CONTAINERS && GRL_IS_MEDIA_BOX (media))) {
      if (rgdata->offset == 0) {
        add_media = TRUE;
      } else {
        rgdata->offset--;
      }
    } else if (rgdata->list_type == LIST_ALL) {
      add_media = TRUE;
    }
  }

  if (add_media) {
    if (rgdata->parent_id) {
      grl_media_set_rygel_grilo_parent (media,
                                        rgdata->parent_id);
    }
    prop_table = ms2_server_new_properties_hashtable ();
    fill_properties_table (rgdata->server,
                           prop_table,
                           rgdata->keys,
                           media);
    fill_other_properties_table (rgdata->server,
                                 source,
                                 prop_table,
                                 rgdata->other_keys,
                                 media);
    rgdata->children = g_list_prepend (rgdata->children, prop_table);
    rgdata->count--;
  }

  if (!remaining) {
    rgdata->children = g_list_reverse (rgdata->children);
    rgdata->updated = TRUE;
  } else if (!rgdata->count) {
    grl_media_source_cancel (source, rgdata->operation_id);
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
get_properties_cb (MS2Server *server,
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
list_children_cb (MS2Server *server,
                  const gchar *id,
                  ListType list_type,
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
  rgdata->offset = offset;
  rgdata->list_type = list_type;
  media = unserialize_media (GRL_METADATA_SOURCE (rgdata->source), id);

  /* Adjust limits */
  if (offset >= limit) {
    browse_cb (rgdata->source, 0, NULL, 0, rgdata, NULL);
  } else {
    /* TIP: as Grilo is not able to split containers and items, in this case we
       will ask for all elements and then remove unneeded children in callback */
    switch (list_type) {
    case LIST_ALL:
      rgdata->count = max_count == 0? (limit - offset): CLAMP (max_count,
                                                               1,
                                                               limit - offset),
        rgdata->operation_id = grl_media_source_browse (rgdata->source,
                                                        media,
                                                        rgdata->keys,
                                                        offset,
                                                        rgdata->count,
                                                        GRL_RESOLVE_FULL | GRL_RESOLVE_IDLE_RELAY,
                                                        browse_cb,
                                                        rgdata);
      break;
    case LIST_CONTAINERS:
    case LIST_ITEMS:
      rgdata->count = max_count == 0? limit: max_count;
      rgdata->operation_id = grl_media_source_browse (rgdata->source,
                                                      media,
                                                      rgdata->keys,
                                                      0,
                                                      limit,
                                                      GRL_RESOLVE_FULL | GRL_RESOLVE_IDLE_RELAY,
                                                      browse_cb,
                                                      rgdata);
      break;
    default:
      /* Protection. It should never be reached, unless ListType is extended */
      browse_cb (rgdata->source, 0, NULL, 0, rgdata, NULL);
    }
  }

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

static GList *
search_objects_cb (MS2Server *server,
                   const gchar *id,
                   const gchar *query,
                   guint offset,
                   guint max_count,
                   const gchar **properties,
                   gpointer data,
                   GError **error)
{
  GList *objects;
  RygelGriloData *rgdata;

  /* Browse is only allowed in root container */
  if (g_strcmp0 (id, MS2_ROOT) != 0) {
    if (error) {
      /* FIXME: a better error should be reported */
      *error = g_error_new (0, 0, "search is only allowed in root container");
    }
    return NULL;
  }

  rgdata = g_slice_new0 (RygelGriloData);
  rgdata->server = g_object_ref (server);
  rgdata->source = (GrlMediaSource *) data;
  rgdata->keys = get_grilo_keys (properties, &rgdata->other_keys);
  rgdata->parent_id = g_strdup (id);
  rgdata->list_type = LIST_ALL;

  /* Adjust limits */
  if (offset >= limit) {
    browse_cb (rgdata->source, 0, NULL, 0, rgdata, NULL);
  } else {
    grl_media_source_search (rgdata->source,
                             query,
                             rgdata->keys,
                             offset,
                             max_count == 0? (limit - offset): CLAMP (max_count,
                                                                      1,
                                                                      limit - offset),
                             GRL_RESOLVE_FULL | GRL_RESOLVE_IDLE_RELAY,
                             browse_cb,
                             rgdata);
  }

  wait_for_result (rgdata);

  if (rgdata->error) {
    if (error) {
      *error = rgdata->error;
    }
    g_list_foreach (rgdata->children, (GFunc) g_hash_table_unref, NULL);
    g_list_free (rgdata->children);
    objects = NULL;
  } else {
    objects = rgdata->children;
  }

  g_list_free (rgdata->keys);
  g_list_free (rgdata->other_keys);
  g_free (rgdata->parent_id);
  g_object_unref (rgdata->server);
  g_slice_free (RygelGriloData, rgdata);

  return objects;
}

/* Callback invoked whenever a new source comes up */
static void
source_added_cb (GrlPluginRegistry *registry, gpointer user_data)
{
  GrlSupportedOps supported_ops;
  MS2Server *server;
  const gchar *source_name;
  gchar *sanitized_source_id;
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
    sanitized_source_id = g_strdup (source_id);

    g_debug ("Registering %s [%s] source", sanitized_source_id, source_name);

    sanitize (sanitized_source_id);

    server = ms2_server_new (sanitized_source_id, GRL_MEDIA_SOURCE (user_data));

    if (!server) {
      g_warning ("Cannot register %s", sanitized_source_id);
      g_free (sanitized_source_id);
    } else {
      ms2_server_set_get_properties_func (server, get_properties_cb);
      ms2_server_set_list_children_func (server, list_children_cb);
      /* Add search  */
      if (supported_ops & GRL_OP_SEARCH) {
        ms2_server_set_search_objects_func (server, search_objects_cb);
      }
      /* Save reference */
      if (!dups) {
        providers_names = g_list_prepend (providers_names,
                                          g_strdup(source_name));
      }
      g_hash_table_insert (servers, sanitized_source_id, server);
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
  gboolean load_success;
  gchar *config_file;

  /* Try first user defined config file */
  if (conffile){
    load_success = grl_plugin_registry_add_config_from_file (registry,
                                                             conffile,
                                                             &error);
  } else {
    config_file = g_build_filename (g_get_user_config_dir (),
                                    "rygel-grilo",
                                    RYGEL_GRILO_CONFIG_FILE,
                                    NULL);
    load_success = grl_plugin_registry_add_config_from_file (registry,
                                                             config_file,
                                                             &error);
    g_free (config_file);
  }

  if (!load_success) {
    g_warning ("Unable to load configuration. %s", error->message);
    g_error_free (error);
  }

  return;
}

/* Main program */
gint
main (gint argc, gchar **argv)
{
  GError *error = NULL;
  GOptionContext *context = NULL;
  gint i;

  context = g_option_context_new ("- run Grilo plugin as UPnP service");
  g_option_context_add_main_entries (context, entries, NULL);
  g_option_context_add_group (context, grl_init_get_option_group ());
  g_option_context_parse (context, &argc, &argv, &error);
  g_option_context_free (context);

  if (error) {
    g_printerr ("Invalid arguments, %s\n", error->message);
    g_clear_error (&error);
    return -1;
  }

  /* Adjust limits */
  limit = CLAMP (limit, 0, G_MAXINT);
  if (limit == 0) {
    limit = G_MAXINT;
  }

  /* Initialize grilo */
  grl_init (&argc, &argv);
  registry = grl_plugin_registry_get_default ();
  if (!registry) {
    g_printerr ("Unable to load Grilo registry\n");
    return -1;
  }

  /* Register a key to store parent */
  GRL_METADATA_KEY_RYGEL_GRILO_PARENT =
    grl_plugin_registry_register_metadata_key (registry,
                                               g_param_spec_string ("rygel-grilo-parent",
                                                                    "RygelGriloParent",
                                                                    "Object path to parent container",
                                                                    NULL,
                                                                    G_PARAM_READWRITE),
                                               NULL);

  if (!GRL_METADATA_KEY_RYGEL_GRILO_PARENT) {
    g_error ("Unable to register Parent key");
    return 1;
  }

  /* Load configuration */
  load_config ();

  /* Initialize <grilo-plugin, ms2-server> pairs */
  servers = g_hash_table_new_full (g_str_hash,
                                   g_str_equal,
                                   g_free,
                                   g_object_unref);

  g_signal_connect (registry, "source-added",
                    G_CALLBACK (source_added_cb), NULL);

  g_signal_connect (registry, "source-removed",
                    G_CALLBACK (source_removed_cb), NULL);

  if (!args || !args[0]) {
    grl_plugin_registry_load_all (registry, NULL);
  } else {
    for (i = 0; args[i]; i++) {
      grl_plugin_registry_load (registry, args[i], NULL);
    }
  }

  g_main_loop_run (g_main_loop_new (NULL, FALSE));
}
