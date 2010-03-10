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
#include "rygel-grilo-media-server.h"
#include "rygel-grilo-media-server-glue.h"

#define ROOT_PARENT_ID "0"

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
#define MS_PROP_TYPE         "type"
#define MS_PROP_URLS         "URLs"
#define MS_PROP_WIDTH        "width"
/* #define MS_PROP_PARENT       "parent" */

#define DBUS_TYPE_G_ARRAY_OF_STRING                             \
  (dbus_g_type_get_collection ("GPtrArray", G_TYPE_STRING))

#define RYGEL_GRILO_MEDIA_SERVER_GET_PRIVATE(o)                         \
  G_TYPE_INSTANCE_GET_PRIVATE((o), RYGEL_GRILO_MEDIA_SERVER_TYPE, RygelGriloMediaServerPrivate)

typedef enum {
  TYPE_BOX,
  TYPE_AUDIO,
  TYPE_VIDEO,
  TYPE_IMAGE
} MediaType;

typedef struct {
  gchar **filter;
  DBusGMethodInvocation *context;
} GetPropertiesData;

typedef struct {
  gchar **filter;
  DBusGMethodInvocation *context;
  GHashTable *result;
} GetChildrenData;

struct _RygelGriloMediaServerPrivate {
  GrlMediaSource *grl_source;
};

G_DEFINE_TYPE (RygelGriloMediaServer, rygel_grilo_media_server, G_TYPE_OBJECT);

static void
free_property_array (GPtrArray *p)
{
  g_ptr_array_foreach (p, (GFunc) g_value_unset, NULL);
  g_ptr_array_free (p, TRUE);
}

static GetPropertiesData *
get_properties_data_new (const gchar **filter,
                         DBusGMethodInvocation *context)
{
  GetPropertiesData *data = g_new (GetPropertiesData, 1);

  data->filter = g_strdupv ((gchar **) filter);
  data->context = context;

  return data;
}

static GetChildrenData *
get_children_data_new (const gchar **filter,
                       DBusGMethodInvocation *context)
{
  GetChildrenData *data = g_new (GetChildrenData, 1);
  data->filter = g_strdupv ((gchar **) filter);
  data->context = context;
  data->result = g_hash_table_new_full (g_str_hash,
                                        g_str_equal,
                                        g_free,
                                        (GDestroyNotify) free_property_array);

  return data;
}

static void
get_properties_data_free (GetPropertiesData *data)
{
  g_strfreev (data->filter);
  g_free (data);
}

static void
get_children_data_free (GetChildrenData *data) {
  g_strfreev (data->filter);
  g_hash_table_unref (data->result);
  g_free (data);
}

static void
rygel_grilo_media_server_dbus_register (RygelGriloMediaServer *obj,
                                        const gchar *dbus_path)
{
  DBusGConnection *connection;

  connection = dbus_g_bus_get (DBUS_BUS_SESSION, NULL);
  g_assert (connection);

  dbus_g_connection_register_g_object (connection,
                                       dbus_path,
                                       G_OBJECT (obj));
}

static void
rygel_grilo_media_server_dispose (GObject *object)
{
  RygelGriloMediaServer *self;

  self = RYGEL_GRILO_MEDIA_SERVER (object);
  g_object_unref (self->priv->grl_source);

  G_OBJECT_CLASS (rygel_grilo_media_server_parent_class)->dispose (object);
}

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

static void
rygel_grilo_media_server_init (RygelGriloMediaServer *server)
{
  server->priv = RYGEL_GRILO_MEDIA_SERVER_GET_PRIVATE (server);
}

static GrlMedia *
rygel_grilo_media_server_build_media (RygelGriloMediaServer *server,
                                      const gchar *id,
                                      MediaType mtype)
{
  GrlMedia *media;

  switch (mtype) {
  case TYPE_BOX:
    media = grl_media_box_new ();
    break;
  case TYPE_AUDIO:
    media = grl_media_audio_new ();
    break;
  case TYPE_VIDEO:
    media = grl_media_video_new ();
    break;
  case TYPE_IMAGE:
    media = grl_media_image_new ();
    break;
  }

  grl_media_set_source (media,
                        grl_metadata_source_get_id (GRL_METADATA_SOURCE (server->priv->grl_source)));
  if (g_strcmp0 (id, ROOT_PARENT_ID) != 0) {
    grl_media_set_id (media, id);
  }

  return media;
}

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


static GValue *
get_display_name (GrlMedia *media)
{
  return get_value_string (grl_media_get_title (media));
}

static GValue *
get_album (GrlMedia *media)
{
  return get_value_string (grl_data_get_string (GRL_DATA (media),
                                                GRL_METADATA_KEY_ALBUM));
}

static GValue *
get_artist (GrlMedia *media)
{
  return get_value_string (grl_data_get_string (GRL_DATA (media),
                                                GRL_METADATA_KEY_ARTIST));
}

static GValue *
get_genre (GrlMedia *media)
{
  return get_value_string (grl_data_get_string (GRL_DATA (media),
                                                GRL_METADATA_KEY_GENRE));
}

static GValue *
get_mime_type (GrlMedia *media)
{
  return get_value_string (grl_data_get_string (GRL_DATA (media),
                                                GRL_METADATA_KEY_MIME));
}

static GValue *
get_bitrate (GrlMedia *media)
{
  return get_value_int (grl_data_get_int (GRL_DATA (media),
                                          GRL_METADATA_KEY_BITRATE));
}

static GValue *
get_duration (GrlMedia *media)
{
  return get_value_int (grl_data_get_int (GRL_DATA (media),
                                          GRL_METADATA_KEY_DURATION));
}

static GValue *
get_height (GrlMedia *media)
{
  return get_value_int (grl_data_get_int (GRL_DATA (media),
                                          GRL_METADATA_KEY_HEIGHT));
}

static GValue *
get_width (GrlMedia *media)
{
  return get_value_int (grl_data_get_int (GRL_DATA (media),
                                          GRL_METADATA_KEY_WIDTH));
}

static GPtrArray *
get_property_values (GrlMedia *media,
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
    }
  }

  return prop_values;
}

static void
get_properties_cb (GrlMediaSource *source,
                   GrlMedia *media,
                   gpointer user_data,
                   const GError *error)
{
  GPtrArray *prop_values;
  GetPropertiesData *data = (GetPropertiesData *) user_data;

  g_assert (media);

  prop_values = get_property_values (media, data->filter);
  dbus_g_method_return (data->context, prop_values);
  free_property_array (prop_values);
  get_properties_data_free (data);
  g_object_unref (media);
}

gboolean
rygel_grilo_media_server_get_properties (RygelGriloMediaServer *server,
                                         const gchar *id,
                                         const gchar **filter,
                                         DBusGMethodInvocation *context,
                                         GError **error)
{
  GList *keys;
  GetPropertiesData *data;
  GrlMedia *media;

  media = rygel_grilo_media_server_build_media (server, id, TYPE_BOX);
  keys = rygel_grilo_media_server_get_keys (filter);
  data = get_properties_data_new (filter, context);

  grl_media_source_metadata (server->priv->grl_source,
                             media,
                             keys,
                             GRL_RESOLVE_FULL,
                             get_properties_cb,
                             data);
  g_list_free (keys);

  return TRUE;
}

static void
get_children_cb (GrlMediaSource *source,
                 guint browse_id,
                 GrlMedia *media,
                 guint remaining,
                 gpointer user_data,
                 const GError *error)
{
  GetChildrenData *data = (GetChildrenData *) user_data;

  g_assert (!error);

  if (media) {
    g_hash_table_insert (data->result,
                         g_strdup (grl_media_get_id (media)),
                         get_property_values (media, data->filter));
  }

  if (!remaining) {
    dbus_g_method_return (data->context, data->result);
    get_children_data_free (data);
  }
}

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
  GetChildrenData *data;
  GrlMedia *media;

  media = rygel_grilo_media_server_build_media (server, id, TYPE_BOX);
  keys = rygel_grilo_media_server_get_keys (filter);
  data = get_children_data_new (filter, context);

  grl_media_source_browse (server->priv->grl_source,
                           media,
                           keys,
                           offset,
                           max_count < 0? G_MAXINT: max_count,
                           GRL_RESOLVE_FULL,
                           get_children_cb,
                           data);
  g_object_unref (media);
  g_list_free (keys);

  return TRUE;
}

RygelGriloMediaServer *
rygel_grilo_media_server_new (const gchar *dbus_path,
                              GrlMediaSource *source)
{
  RygelGriloMediaServer *obj;

  obj = g_object_new (RYGEL_GRILO_MEDIA_SERVER_TYPE, NULL);

  obj->priv->grl_source = source;
  g_object_ref (obj->priv->grl_source);

  rygel_grilo_media_server_dbus_register (obj, dbus_path);

  return obj;
}
