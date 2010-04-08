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

#ifndef _MEDIA_SERVER2_H_
#define _MEDIA_SERVER2_H_

#include <glib.h>
#include <glib-object.h>

/* Common properties */
#define MS2_PROP_ID           "id"
#define MS2_PROP_PARENT       "parent"
#define MS2_PROP_DISPLAY_NAME "display-name"
#define MS2_PROP_TYPE         "type"

/* Container properties */
#define MS2_PROP_CHILD_COUNT "child-count"
#define MS2_PROP_ICON        "icon"

/* Item properties */
#define MS2_PROP_URLS         "URLs"
#define MS2_PROP_MIME_TYPE    "mime-type"
#define MS2_PROP_SIZE         "size"
#define MS2_PROP_ARTIST       "artist"
#define MS2_PROP_ALBUM        "album"
#define MS2_PROP_DATE         "date"
#define MS2_PROP_DLNA_PROFILE "dlna-profile"

/* Audio/Video items properties */
#define MS2_PROP_DURATION        "duration"
#define MS2_PROP_BITRATE         "bitrate"
#define MS2_PROP_SAMPLE_RATE     "sample-rate"
#define MS2_PROP_BITS_PER_SAMPLE "bits-per-sample"

/* Video/Image items properties */
#define MS2_PROP_WIDTH        "width"
#define MS2_PROP_HEIGHT       "height"
#define MS2_PROP_COLOR_DEPTH  "depth"
#define MS2_PROP_PIXEL_WIDTH  "pixel-width"
#define MS2_PROP_PIXEL_HEIGHT "pixel-height"
#define MS2_PROP_THUMBNAIL    "thumbnail"

/* Audio items properties */
#define MS2_PROP_GENRE "genre"

/* Type items */
#define MS2_TYPE_CONTAINER "container"
#define MS2_TYPE_VIDEO     "video"
#define MS2_TYPE_MOVIE     "video.movie"
#define MS2_TYPE_AUDIO     "audio"
#define MS2_TYPE_MUSIC     "audio.music"
#define MS2_TYPE_IMAGE     "image"
#define MS2_TYPE_PHOTO     "image.photo"

/* Unknown values */
#define MS2_UNKNOWN_INT -1
#define MS2_UNKNOWN_STR ""

#define MEDIA_SERVER2_ERROR                             \
  g_quark_from_static_string("media_server2_error")

#define MEDIA_SERVER2_TYPE                      \
  (media_server2_get_type ())

#define MEDIA_SERVER2(obj)                                      \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj),                           \
                               MEDIA_SERVER2_TYPE,              \
                               MediaServer2))

#define IS_MEDIA_SERVER2(obj)                                           \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj),                                   \
                               MEDIA_SERVER2_TYPE))

#define MEDIA_SERVER2_CLASS(klass)                                \
  (G_TYPE_CHECK_CLASS_CAST((klass),                               \
                           MEDIA_SERVER2_TYPE,                    \
                           MediaServer2Class))

#define IS_MEDIA_SERVER2_CLASS(klass)                           \
  (G_TYPE_CHECK_CLASS_TYPE((klass),                             \
                           MEDIA_SERVER2_TYPE))

#define MEDIA_SERVER2_GET_CLASS(obj)                            \
  (G_TYPE_INSTANCE_GET_CLASS ((obj),                            \
                              MEDIA_SERVER2_TYPE,               \
                              MediaServer2Class))

typedef GHashTable * (*GetPropertiesFunc) (const gchar *id,
                                           const gchar **properties,
                                           gpointer data,
                                           GError **error);

typedef GList * (*GetChildrenFunc) (const gchar *id,
                                    guint offset,
                                    gint max_count,
                                    const gchar **properties,
                                    gpointer data,
                                    GError **error);

typedef struct _MediaServer2        MediaServer2;
typedef struct _MediaServer2Private MediaServer2Private;

struct _MediaServer2 {

  GObject parent;

  /*< private >*/
  MediaServer2Private *priv;
};

typedef struct _MediaServer2Class MediaServer2Class;

struct _MediaServer2Class {

  GObjectClass parent_class;
};

typedef enum {
  MEDIA_SERVER2_ERROR_GENERAL = 1
} MediaServer2Error;

GType media_server2_get_type (void);

MediaServer2 *media_server2_new (const gchar *name,
                                 gpointer data);

void media_server2_set_get_properties_func (MediaServer2 *server,
                                            GetPropertiesFunc get_properties_func);

void media_server2_set_get_children_func (MediaServer2 *server,
                                          GetChildrenFunc get_children_func);

GHashTable *media_server2_new_properties_hashtable (const gchar *id);

void media_server2_set_parent (GHashTable *properties,
                               const gchar *parent);

void media_server2_set_display_name (GHashTable *properties,
                                     const gchar *display_name);

void media_server2_set_type_container (GHashTable *properties);

void media_server2_set_type_video (GHashTable *properties);

void media_server2_set_type_movie (GHashTable *properties);

void media_server2_set_type_audio (GHashTable *properties);

void media_server2_set_type_music (GHashTable *properties);

void media_server2_set_type_image (GHashTable *properties);

void media_server2_set_type_photo (GHashTable *properties);

void media_server2_set_icon (GHashTable *properties,
                             const gchar *icon);

void media_server2_set_mime_type (GHashTable *properties,
                                  const gchar *mime_type);

void media_server2_set_artist (GHashTable *properties,
                               const gchar *artist);

void media_server2_set_album (GHashTable *properties,
                              const gchar *album);

void media_server2_set_date (GHashTable *properties,
                             const gchar *date);

void media_server2_set_dlna_profile (GHashTable *properties,
                                     const gchar *dlna_profile);

void media_server2_set_thumbnail (GHashTable *properties,
                                  const gchar *thumbnail);

void media_server2_set_genre (GHashTable *properties,
                              const gchar *genre);

void media_server2_set_child_count (GHashTable *properties,
                                    gint child_count);

void media_server2_set_size (GHashTable *properties,
                             gint size);

void media_server2_set_duration (GHashTable *properties,
                                 gint duration);

void media_server2_set_bitrate (GHashTable *properties,
                                gint bitrate);

void media_server2_set_sample_rate (GHashTable *properties,
                                    gint sample_rate);

void media_server2_set_bits_per_sample (GHashTable *properties,
                                        gint bits_per_sample);

void media_server2_set_width (GHashTable *properties,
                              gint width);

void media_server2_set_height (GHashTable *properties,
                               gint height);

void media_server2_set_pixel_width (GHashTable *properties,
                                    gint pixel_width);

void media_server2_set_pixel_height (GHashTable *properties,
                                     gint pixel_height);

void media_server2_set_urls (GHashTable *properties,
                             gchar **urls);

#endif /* _MEDIA_SERVER2_H_ */
