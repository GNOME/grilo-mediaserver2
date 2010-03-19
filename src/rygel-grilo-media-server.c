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

#include <dbus/dbus-glib.h>
#include <string.h>
#include "rygel-grilo-media-server.h"
#include "rygel-grilo-media-server-glue.h"

#define ID_PREFIX_AUDIO     "gra://"
#define ID_PREFIX_CONTAINER "grc://"
#define ID_PREFIX_IMAGE     "gri://"
#define ID_PREFIX_VIDEO     "grv://"
#define ID_ROOT             "0"
#define ID_SEPARATOR        "/"

#define MS_INT_VALUE_UNKNOWN -1
#define MS_STR_VALUE_UNKNOWN ""

#define MS_TYPE_AUDIO     "audio"
#define MS_TYPE_CONTAINER "container"
#define MS_TYPE_IMAGE     "image"
#define MS_TYPE_VIDEO     "video"

#define MS_PROP_ALBUM        "album"
#define MS_PROP_ARTIST       "artist"
#define MS_PROP_BITRATE      "bitrate"
#define MS_PROP_CHILD_COUNT  "child-count"
#define MS_PROP_DISPLAY_NAME "display-name"
#define MS_PROP_DURATION     "duration"
#define MS_PROP_GENRE        "genre"
#define MS_PROP_HEIGHT       "height"
#define MS_PROP_MIME_TYPE    "mime-type"
#define MS_PROP_PARENT       "parent"
#define MS_PROP_TYPE         "type"
#define MS_PROP_URLS         "URLs"
#define MS_PROP_WIDTH        "width"

#define DBUS_TYPE_G_ARRAY_OF_STRING                             \
  (dbus_g_type_get_collection ("GPtrArray", G_TYPE_STRING))

#define RYGEL_GRILO_MEDIA_SERVER_GET_PRIVATE(o)                         \
  G_TYPE_INSTANCE_GET_PRIVATE((o), RYGEL_GRILO_MEDIA_SERVER_TYPE, RygelGriloMediaServerPrivate)

typedef struct {
  gchar *ms_id;
  gchar **filter;
  DBusGMethodInvocation *context;
  GHashTable *result;
} GetData;

/*
 * Private RygelGriloMediaServer structure
 *   grl_source: grilo source being wrapped
 */
struct _RygelGriloMediaServerPrivate {
  GrlMediaSource *grl_source;
};

G_DEFINE_TYPE (RygelGriloMediaServer, rygel_grilo_media_server, G_TYPE_OBJECT);

/* Frees an array of gvalues */
static void
free_property_array (GPtrArray *p)
{
  g_ptr_array_foreach (p, (GFunc) g_value_unset, NULL);
  g_ptr_array_free (p, TRUE);
}

/* Returns a new GetData used when requesting a property */
static GetData *
get_data_new_properties (const gchar *ms_id,
                         const gchar **filter,
                         DBusGMethodInvocation *context)
{
  GetData *data = g_new0 (GetData, 1);

  data->ms_id = g_strdup (ms_id);
  data->filter = g_strdupv ((gchar **) filter);
  data->context = context;

  return data;
}

/* Returns a new GetDAta used when requesting for children */
static GetData *
get_data_new_children (const gchar *ms_id,
                       const gchar **filter,
                       DBusGMethodInvocation *context)
{
  GetData *data;

  data = get_data_new_properties (ms_id, filter, context);
  data->result = g_hash_table_new_full (g_str_hash,
                                        g_str_equal,
                                        g_free,
                                        (GDestroyNotify) free_property_array);

  return data;
}

/* Frees GetData and used resources */
static void
get_data_free (GetData *data) {
  g_free (data->ms_id);
  g_strfreev (data->filter);
  if (data->result) {
    g_hash_table_unref (data->result);
  }
  g_free (data);
}

/* Registers the RygelGriloMediaServer object in dbus */
static gboolean
rygel_grilo_media_server_dbus_register (RygelGriloMediaServer *obj,
                                        const gchar *dbus_path)
{
  DBusGConnection *connection;
  GError *error = NULL;

  connection = dbus_g_bus_get (DBUS_BUS_SESSION, &error);
  if (!connection) {
    g_printerr ("Could not connect to session bus, %s\n", error->message);
    g_clear_error (&error);
    return FALSE;
  }

  dbus_g_connection_register_g_object (connection,
                                       dbus_path,
                                       G_OBJECT (obj));

  return TRUE;
}

/* Dispose an object */
static void
rygel_grilo_media_server_dispose (GObject *object)
{
  RygelGriloMediaServer *self;

  self = RYGEL_GRILO_MEDIA_SERVER (object);
  g_object_unref (self->priv->grl_source);

  G_OBJECT_CLASS (rygel_grilo_media_server_parent_class)->dispose (object);
}

/* Class init function */
static void
rygel_grilo_media_server_class_init (RygelGriloMediaServerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (RygelGriloMediaServerPrivate));

  object_class->dispose = rygel_grilo_media_server_dispose;

  /* Register introspection */
  dbus_g_object_type_install_info (RYGEL_GRILO_MEDIA_SERVER_TYPE,
                                   &dbus_glib_rygel_grilo_media_server_object_info);
}

/* Object init function */
static void
rygel_grilo_media_server_init (RygelGriloMediaServer *server)
{
  server->priv = RYGEL_GRILO_MEDIA_SERVER_GET_PRIVATE (server);
  memset (server->priv, 0, sizeof (RygelGriloMediaServerPrivate));
}

/* Given a rygel-grilo id, returns the id of grilo media that it is being
   wrapped; as rygel-grilo ids are formed by concatenating hierarchy of all
   grilo media ids, the searched id is the last token */
static gchar *
extract_grl_id (const gchar *ms_id)
{
  gchar **offspring;
  gchar *grl_id;

  if (g_strcmp0 (ms_id, ID_ROOT) == 0) {
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

/* Given a Rygel GriloMediaServer, builds a new grilo media object represented
   by the rygel-grilo id */
static GrlMedia *
rygel_grilo_media_server_build_media (RygelGriloMediaServer *server,
                                      const gchar *id)
{
  GrlMedia *media = NULL;
  gchar *grl_id;

  if (g_strcmp0 (id, ID_ROOT) == 0 ||
      g_str_has_prefix (id, ID_PREFIX_CONTAINER)) {
    media = grl_media_box_new ();
  } else if (g_str_has_prefix (id, ID_PREFIX_AUDIO)) {
    media = grl_media_audio_new ();
  } else if (g_str_has_prefix (id, ID_PREFIX_VIDEO)) {
    media = grl_media_video_new ();
  } else if (g_str_has_prefix (id, ID_PREFIX_IMAGE)) {
    media = grl_media_image_new ();
  }

  grl_media_set_source (media,
                        grl_metadata_source_get_id (GRL_METADATA_SOURCE (server->priv->grl_source)));


  grl_id = extract_grl_id (id);
  if (grl_id) {
    grl_media_set_id (media, grl_id);
    g_free (grl_id);
  }

  return media;
}

/* Given a null-terminated array of MediaServerSpec2 properties, returns a list
   with the corresponding Grilo metadata keys */
static GList *
rygel_grilo_media_server_get_keys (const gchar **ms_keys)
{
  GList *grl_keys = NULL;
  gint i;

  for (i = 0; ms_keys[i]; i++) {
    if (g_strcmp0 (ms_keys[i], MS_PROP_DISPLAY_NAME) == 0) {
      grl_keys = g_list_append (grl_keys,
                                GRLKEYID_TO_POINTER (GRL_METADATA_KEY_TITLE));
    } else if (g_strcmp0 (ms_keys[i], MS_PROP_ALBUM) == 0) {
      grl_keys = g_list_append (grl_keys,
                                GRLKEYID_TO_POINTER (GRL_METADATA_KEY_ALBUM));
    } else if (g_strcmp0 (ms_keys[i], MS_PROP_ARTIST) == 0) {
      grl_keys = g_list_append (grl_keys,
                                GRLKEYID_TO_POINTER (GRL_METADATA_KEY_ARTIST));
    } else if (g_strcmp0 (ms_keys[i], MS_PROP_GENRE) == 0) {
      grl_keys = g_list_append (grl_keys,
                                GRLKEYID_TO_POINTER (GRL_METADATA_KEY_GENRE));
    } else if (g_strcmp0 (ms_keys[i], MS_PROP_MIME_TYPE) == 0) {
      grl_keys = g_list_append (grl_keys,
                                GRLKEYID_TO_POINTER (GRL_METADATA_KEY_MIME));
    } else if (g_strcmp0 (ms_keys[i], MS_PROP_CHILD_COUNT) == 0) {
      grl_keys = g_list_append (grl_keys,
                                GRLKEYID_TO_POINTER (GRL_METADATA_KEY_CHILDCOUNT));
    } else if (g_strcmp0 (ms_keys[i], MS_PROP_URLS) == 0) {
      grl_keys = g_list_append (grl_keys,
                                GRLKEYID_TO_POINTER (GRL_METADATA_KEY_URL));
    } else if (g_strcmp0 (ms_keys[i], MS_PROP_BITRATE) == 0) {
      grl_keys = g_list_append (grl_keys,
                                GRLKEYID_TO_POINTER (GRL_METADATA_KEY_BITRATE));
    } else if (g_strcmp0 (ms_keys[i], MS_PROP_DURATION) == 0) {
      grl_keys = g_list_append (grl_keys,
                                GRLKEYID_TO_POINTER (GRL_METADATA_KEY_DURATION));
    } else if (g_strcmp0 (ms_keys[i], MS_PROP_HEIGHT) == 0) {
      grl_keys = g_list_append (grl_keys,
                                GRLKEYID_TO_POINTER (GRL_METADATA_KEY_HEIGHT));
    } else if (g_strcmp0 (ms_keys[i], MS_PROP_WIDTH) == 0) {
      grl_keys = g_list_append (grl_keys,
                                GRLKEYID_TO_POINTER (GRL_METADATA_KEY_WIDTH));
    }
  }

  return grl_keys;
}

/* Return the type of a grilo media (audio, video, imae, container or unknown */
static GValue *
get_type (GrlMedia *media)
{
  GValue *val;
  gchar *type;

  if (GRL_IS_MEDIA_BOX (media)) {
    type = MS_TYPE_CONTAINER;
  } else if (GRL_IS_MEDIA_AUDIO (media)) {
    type = MS_TYPE_AUDIO;
  } else if (GRL_IS_MEDIA_VIDEO (media)) {
    type = MS_TYPE_VIDEO;
  } else if (GRL_IS_MEDIA_IMAGE (media)) {
    type = MS_TYPE_IMAGE;
  } else {
    type = MS_STR_VALUE_UNKNOWN;
  }

  val = g_new0 (GValue, 1);
  g_value_init (val, G_TYPE_STRING);
  g_value_set_string (val, type);

  return val;
}

/* Given the rygel-grilo parent id, and a media, builds the rygel-grilo id for
   the media */
static gchar *
build_ms_id (const gchar *parent_id,
             GrlMedia *media)
{
  gchar *escaped_id;
  gchar *ms_id;

  escaped_id = g_uri_escape_string (grl_media_get_id (media), NULL, TRUE);

  if (g_strcmp0 (parent_id, ID_ROOT) == 0) {
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

/* Returns the list of URLs for the grilo media */
static GValue *
get_urls (GrlMedia *media)
{
  GPtrArray *urls;
  GValue *val;
  const gchar *url;

  url = grl_media_get_url (media);
  urls = g_ptr_array_sized_new (1);
  g_ptr_array_add (urls,
                   url? g_strdup (url): g_strdup (MS_STR_VALUE_UNKNOWN));

  val = g_new0 (GValue, 1);
  g_value_init (val, DBUS_TYPE_G_ARRAY_OF_STRING);
  g_value_take_boxed (val, urls);

  return val;
}

/* Get childcount value for grilo-media */
static GValue *
get_child_count (GrlMedia *media)
{
  GValue *val;
  gint childcount = MS_INT_VALUE_UNKNOWN;

  if (GRL_IS_MEDIA_BOX (media)) {
    childcount = grl_media_box_get_childcount (GRL_MEDIA_BOX (media));
    if (childcount == GRL_METADATA_KEY_CHILDCOUNT_UNKNOWN) {
      childcount = MS_INT_VALUE_UNKNOWN;
    }
  }

  val = g_new0 (GValue, 1);
  val = g_value_init (val, G_TYPE_INT);
  g_value_set_int (val, childcount);

  return val;
}

/* Returns a gvalue containing the string, or default case if it is null */
static GValue *
get_value_string (const gchar *s)
{
  GValue *val;

  val = g_new0 (GValue, 1);
  g_value_init (val, G_TYPE_STRING);
  g_value_set_string (val,
                      s? s: MS_STR_VALUE_UNKNOWN);

  return val;
}

/* Returns a gvalue containing the integer, or default value it is 0 */
static GValue *
get_value_int (gint i)
{
  GValue *val;

  val = g_new0 (GValue, 1);
  g_value_init (val, G_TYPE_INT);
  g_value_set_int (val,
                   i? i: MS_INT_VALUE_UNKNOWN);

  return val;
}

/* Returns the title of a media */
static GValue *
get_display_name (GrlMedia *media)
{
  return get_value_string (grl_media_get_title (media));
}

/* Returns the album of a media */
static GValue *
get_album (GrlMedia *media)
{
  return get_value_string (grl_data_get_string (GRL_DATA (media),
                                                GRL_METADATA_KEY_ALBUM));
}

/* Returns the artist of a media */
static GValue *
get_artist (GrlMedia *media)
{
  return get_value_string (grl_data_get_string (GRL_DATA (media),
                                                GRL_METADATA_KEY_ARTIST));
}

/* Returns the genre of a media */
static GValue *
get_genre (GrlMedia *media)
{
  return get_value_string (grl_data_get_string (GRL_DATA (media),
                                                GRL_METADATA_KEY_GENRE));
}

/* Returns the mime type of a media */
static GValue *
get_mime_type (GrlMedia *media)
{
  return get_value_string (grl_data_get_string (GRL_DATA (media),
                                                GRL_METADATA_KEY_MIME));
}

/* Returns the bitrate of a media */
static GValue *
get_bitrate (GrlMedia *media)
{
  return get_value_int (grl_data_get_int (GRL_DATA (media),
                                          GRL_METADATA_KEY_BITRATE));
}

/* Returns the duration of a media */
static GValue *
get_duration (GrlMedia *media)
{
  return get_value_int (grl_data_get_int (GRL_DATA (media),
                                          GRL_METADATA_KEY_DURATION));
}

/* Returns the height of a media */
static GValue *
get_height (GrlMedia *media)
{
  return get_value_int (grl_data_get_int (GRL_DATA (media),
                                          GRL_METADATA_KEY_HEIGHT));
}

/* Returns the width of a media */
static GValue *
get_width (GrlMedia *media)
{
  return get_value_int (grl_data_get_int (GRL_DATA (media),
                                          GRL_METADATA_KEY_WIDTH));
}

/* Returns the rygel-grilo parent id of the child */
static GValue *
get_parent_id (const gchar *child_id)
{
  GValue *val;
  gchar *parent_end;
  gchar *parent_id;
  gsize bytes_to_copy;

  if (g_strcmp0 (child_id, ID_ROOT) == 0) {
    return get_value_string (NULL);
  }

  parent_end = g_strrstr (child_id, ID_SEPARATOR);
  bytes_to_copy = parent_end - child_id;

  /* Check if parent is a root */
  if (bytes_to_copy < 6) {
    return get_value_string (ID_ROOT);
  }

  /* Save parent id */
  parent_id = g_strndup (child_id, bytes_to_copy);

  /* Parent should be always a container */
  parent_id[2] = 'c';

  val = get_value_string (parent_id);
  g_free (parent_id);

  return val;
}

/* Returns an array with gvalues of properties in filter for the media */
static GPtrArray *
get_property_values (const gchar *ms_media_id,
                     GrlMedia *media,
                     gchar **filter)
{
  GPtrArray *prop_values;
  gint i;

  prop_values = g_ptr_array_sized_new (g_strv_length (filter));
  for (i = 0; filter[i]; i++) {
    if (g_strcmp0 (filter[i], MS_PROP_DISPLAY_NAME) == 0) {
      g_ptr_array_add (prop_values,
                       get_display_name (media));
    } else if (g_strcmp0 (filter[i], MS_PROP_ALBUM) == 0) {
      g_ptr_array_add (prop_values,
                       get_album (media));
    } else if (g_strcmp0 (filter[i], MS_PROP_ARTIST) == 0) {
      g_ptr_array_add (prop_values,
                       get_artist (media));
    } else if (g_strcmp0 (filter[i], MS_PROP_GENRE) == 0) {
      g_ptr_array_add (prop_values,
                       get_genre (media));
    } else if (g_strcmp0 (filter[i], MS_PROP_MIME_TYPE) == 0) {
      g_ptr_array_add (prop_values,
                       get_mime_type (media));
    } else if (g_strcmp0 (filter[i], MS_PROP_TYPE) == 0) {
      g_ptr_array_add (prop_values,
                       get_type (media));
    } else if (g_strcmp0 (filter[i], MS_PROP_CHILD_COUNT) == 0) {
      g_ptr_array_add (prop_values,
                       get_child_count (media));
    } else if (g_strcmp0 (filter[i], MS_PROP_URLS) == 0) {
      g_ptr_array_add (prop_values,
                       get_urls (media));
    } else if (g_strcmp0 (filter[i], MS_PROP_BITRATE) == 0) {
      g_ptr_array_add (prop_values,
                       get_bitrate (media));
    } else if (g_strcmp0 (filter[i], MS_PROP_DURATION) == 0) {
      g_ptr_array_add (prop_values,
                       get_duration (media));
    } else if (g_strcmp0 (filter[i], MS_PROP_HEIGHT) == 0) {
      g_ptr_array_add (prop_values,
                       get_height (media));
    } else if (g_strcmp0 (filter[i], MS_PROP_WIDTH) == 0) {
      g_ptr_array_add (prop_values,
                       get_width (media));
    } else if (g_strcmp0 (filter[i], MS_PROP_PARENT) == 0) {
      g_ptr_array_add (prop_values,
                       get_parent_id (ms_media_id));
    }
  }

  return prop_values;
}

/* Callback with the metadata of a Grilo media */
static void
get_properties_cb (GrlMediaSource *source,
                   GrlMedia *media,
                   gpointer user_data,
                   const GError *error)
{
  GPtrArray *prop_values;
  GError *rg_error = NULL;
  GetData *data = (GetData *) user_data;

  if (!media) {
    if (error && error->message) {
      rg_error = g_error_new_literal (RYGEL_GRILO_ERROR,
                                      RYGEL_GRILO_ERROR_GENERAL,
                                      error->message);
    } else {
      rg_error = g_error_new (RYGEL_GRILO_ERROR,
                              RYGEL_GRILO_ERROR_GENERAL,
                              "Unable to resolve %s",
                              data->ms_id);
    }
    dbus_g_method_return_error (data->context, rg_error);
    g_error_free (rg_error);
  } else {
    prop_values = get_property_values (data->ms_id, media, data->filter);
    dbus_g_method_return (data->context, prop_values);
    free_property_array (prop_values);
    get_data_free (data);
    g_object_unref (media);
  }
}

/* Callback with the results of a browse */
static void
get_children_cb (GrlMediaSource *source,
                 guint browse_id,
                 GrlMedia *media,
                 guint remaining,
                 gpointer user_data,
                 const GError *error)
{
  GetData *data = (GetData *) user_data;
  gchar *child_id;

  if (media) {
    child_id = build_ms_id (data->ms_id, media);
    g_hash_table_insert (data->result,
                         child_id,
                         get_property_values (child_id, media, data->filter));
  }

  if (!remaining) {
    dbus_g_method_return (data->context, data->result);
    get_data_free (data);
  }
}

/**
 * rygel_grilo_media_server_get_properties:
 * @server: a RygelGriloMediaServer wrapping a source
 * @id: the id used to identify a Grilo media
 * @filter: list of MediaServerSpec2 properties
 * @context: dbus method context to send the reply
 * @error: error if something is wrong
 *
 * Sends through dbus an array with gvalues of properties in filter for element identified with id.
 *
 * Returns: @TRUE if values can be sent
 **/
gboolean
rygel_grilo_media_server_get_properties (RygelGriloMediaServer *server,
                                         const gchar *id,
                                         const gchar **filter,
                                         DBusGMethodInvocation *context,
                                         GError **error)
{
  GList *keys;
  GetData *data;
  GrlMedia *media;

  media = rygel_grilo_media_server_build_media (server, id);
  keys = rygel_grilo_media_server_get_keys (filter);
  data = get_data_new_properties (id, filter, context);

  grl_media_source_metadata (server->priv->grl_source,
                             media,
                             keys,
                             GRL_RESOLVE_FULL | GRL_RESOLVE_IDLE_RELAY,
                             get_properties_cb,
                             data);
  g_list_free (keys);

  return TRUE;
}

/**
 * rygel_grilo_media_server_get_children:
 * @server: a RygelGriloMediaServer wrapping a source
 * @id: the id used to identify a Grilo media
 * @offset: how many elements should be skipped
 * @max_count: how many elements should be sent
 * @filter: list of MediaServerSpec2 properties
 * @context: dbus method context to send the reply
 * @error: error if something is wrong
 *
 * Send through dbus a dictionary with children id and their properties values, specified in filter.
 *
 * Returns: @TRUE if values can be sent
 **/
gboolean
rygel_grilo_media_server_get_children (RygelGriloMediaServer *server,
                                       const gchar *id,
                                       guint offset,
                                       gint max_count,
                                       const gchar **filter,
                                       DBusGMethodInvocation *context,
                                       GError **error)

{
  GList *keys;
  GetData *data;
  GrlMedia *media;

  media = rygel_grilo_media_server_build_media (server, id);
  keys = rygel_grilo_media_server_get_keys (filter);
  data = get_data_new_children (id, filter, context);

  grl_media_source_browse (server->priv->grl_source,
                           media,
                           keys,
                           offset,
                           max_count < 0? G_MAXINT: max_count,
                           GRL_RESOLVE_FULL | GRL_RESOLVE_IDLE_RELAY,
                           get_children_cb,
                           data);
  g_object_unref (media);
  g_list_free (keys);

  return TRUE;
}

/**
 * rygel_grilo_media_server_new:
 * @dbus_path: dbus path where object will be registered
 * @source: a grilo source being wrapped.
 *
 * Creates a new RygelGriloMediaServer wrapping a grilo source and registered in dbus
 *
 * Returns: a new RygelGriloMediaServer
 **/
RygelGriloMediaServer *
rygel_grilo_media_server_new (const gchar *dbus_path,
                              GrlMediaSource *source)
{
  RygelGriloMediaServer *obj;

  obj = g_object_new (RYGEL_GRILO_MEDIA_SERVER_TYPE, NULL);

  obj->priv->grl_source = source;
  g_object_ref (obj->priv->grl_source);

  if (!rygel_grilo_media_server_dbus_register (obj, dbus_path)) {
    g_object_unref (obj);
    return NULL;
  } else {
    return obj;
  }
}
