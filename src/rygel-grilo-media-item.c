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
#include "rygel-grilo-media-item.h"
#include "rygel-grilo-media-item-glue.h"

#define DBUS_TYPE_G_ARRAY_OF_STRING                             \
  (dbus_g_type_get_collection ("GPtrArray", G_TYPE_STRING))

enum {
  PROP_0,
  PROP_URLS,
  PROP_MIME_TYPE,
  PROP_TYPE,
  PROP_ARTIST,
  PROP_ALBUM,
  PROP_GENRE,
  PROP_DURATION,
  PROP_BITRATE,
  PROP_WIDTH,
  PROP_HEIGHT,
  LAST_PROP
};

G_DEFINE_TYPE (RygelGriloMediaItem, rygel_grilo_media_item, RYGEL_GRILO_MEDIA_OBJECT_TYPE);

/* Gets the a property */
static void
rygel_grilo_media_item_get_property (GObject *object,
                                     guint prop_id,
                                     GValue *value,
                                     GParamSpec *pspec)
{
  GPtrArray *pa;
  GrlMedia *media;
  const gchar *url;

  g_object_get (object,
                "grl-media", &media,
                NULL);

  switch (prop_id) {
  case PROP_URLS:
    url = grl_media_get_url (media);
    if (url) {
      pa = g_ptr_array_sized_new (1);
      g_ptr_array_add (pa, g_strdup (url));
    } else {
      pa = g_ptr_array_sized_new (0);
    }
    g_value_take_boxed (value, pa);
    break;
  case PROP_MIME_TYPE:
    g_value_set_string (value,
                        grl_media_get_mime (media));
    break;
  case PROP_TYPE:
    if (GRL_IS_MEDIA_VIDEO (media)) {
      g_value_set_string (value, "video");
    } else if (GRL_IS_MEDIA_AUDIO (media)) {
      g_value_set_string (value, "audio");
    } else if (GRL_IS_MEDIA_IMAGE (media)) {
      g_value_set_string (value, "image");
    } else {
      g_value_set_string (value, "unknown");
    }
    break;
  case PROP_ARTIST:
    g_value_set_string (value,
                        grl_data_get_string (GRL_DATA (media),
                                             GRL_METADATA_KEY_ARTIST));
    break;
  case PROP_ALBUM:
    g_value_set_string (value,
                        grl_data_get_string (GRL_DATA (media),
                                             GRL_METADATA_KEY_ALBUM));
    break;
  case PROP_GENRE:
    g_value_set_string (value,
                        grl_data_get_string (GRL_DATA (media),
                                             GRL_METADATA_KEY_GENRE));
    break;
  case PROP_DURATION:
    g_value_set_int (value,
                     grl_media_get_duration (media));
    break;
  case PROP_BITRATE:
    g_value_set_int (value,
                     grl_data_get_int (GRL_DATA (media),
                                       GRL_METADATA_KEY_BITRATE));
    break;
  case PROP_WIDTH:
    g_value_set_int (value,
                     grl_data_get_int (GRL_DATA (media),
                                       GRL_METADATA_KEY_WIDTH));
    break;
  case PROP_HEIGHT:
    g_value_set_int (value,
                     grl_data_get_int (GRL_DATA (media),
                                       GRL_METADATA_KEY_HEIGHT));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    break;
  }
}

/* Dispose a RygelGriloMediaItem object */
static void
rygel_grilo_media_item_dispose (GObject *object)
{
  G_OBJECT_CLASS (rygel_grilo_media_item_parent_class)->dispose (object);
}

/* Class init function */
static void
rygel_grilo_media_item_class_init (RygelGriloMediaItemClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->get_property = rygel_grilo_media_item_get_property;
  object_class->dispose = rygel_grilo_media_item_dispose;

  g_object_class_install_property (object_class,
                                   PROP_URLS,
                                   g_param_spec_boxed ("u-rls",
                                                       "URLs",
                                                       "List of urls",
                                                       DBUS_TYPE_G_ARRAY_OF_STRING,
                                                       G_PARAM_READABLE));

  g_object_class_install_property (object_class,
                                   PROP_MIME_TYPE,
                                   g_param_spec_string ("m-im-etype",
                                                        "MIMEType",
                                                        "MIME type",
                                                        NULL,
                                                        G_PARAM_READABLE));

  g_object_class_install_property (object_class,
                                   PROP_TYPE,
                                   g_param_spec_string ("type",
                                                        "Type",
                                                        "Type of item",
                                                        NULL,
                                                        G_PARAM_READABLE));

  g_object_class_install_property (object_class,
                                   PROP_ARTIST,
                                   g_param_spec_string ("artist",
                                                        "Artist",
                                                        "Artist",
                                                        NULL,
                                                        G_PARAM_READABLE));

  g_object_class_install_property (object_class,
                                   PROP_ALBUM,
                                   g_param_spec_string ("album",
                                                        "Album",
                                                        "Album",
                                                        NULL,
                                                        G_PARAM_READABLE));

  g_object_class_install_property (object_class,
                                   PROP_GENRE,
                                   g_param_spec_string ("genre",
                                                        "Genre",
                                                        "Genre",
                                                        NULL,
                                                        G_PARAM_READABLE));

  g_object_class_install_property (object_class,
                                   PROP_DURATION,
                                   g_param_spec_int ("duration",
                                                     "Duration",
                                                     "Duration in seconds",
                                                     0, G_MAXINT, 0,
                                                     G_PARAM_READABLE));

  g_object_class_install_property (object_class,
                                   PROP_BITRATE,
                                   g_param_spec_int ("bitrate",
                                                     "Bitrate",
                                                     "Bitrate",
                                                     0, G_MAXINT, 0,
                                                     G_PARAM_READABLE));

  g_object_class_install_property (object_class,
                                   PROP_WIDTH,
                                   g_param_spec_int ("width",
                                                     "Width",
                                                     "Width",
                                                     0, G_MAXINT, 0,
                                                     G_PARAM_READABLE));

  g_object_class_install_property (object_class,
                                   PROP_HEIGHT,
                                   g_param_spec_int ("height",
                                                     "Height",
                                                     "Height",
                                                     0, G_MAXINT, 0,
                                                     G_PARAM_READABLE));


  /* Register introspection */
  dbus_g_object_type_install_info (RYGEL_GRILO_MEDIA_ITEM_TYPE,
                                   &dbus_glib_rygel_grilo_media_item_object_info);
}

/* Object init function */
static void
rygel_grilo_media_item_init (RygelGriloMediaItem *obj)
{
}

/**
 * rygel_grilo_media_item_new:
 * @parent: the parent
 * @media: the grilo media element that is being wrapped
 *
 * Creates a new RygelGriloMediaItem that wraps the grilo media, and register it
 * in dbus.
 *
 * Returns: a new RygelGriloMediaItem registered on dbus, or @NULL otherwise
 **/
RygelGriloMediaItem *
rygel_grilo_media_item_new (RygelGriloMediaObject *parent,
                            GrlMedia *media)
{
  RygelGriloMediaItem *obj;
  RygelGriloMediaObjectClass *klass;
  gchar *dbus_path;

  g_return_val_if_fail (parent, NULL);
  g_return_val_if_fail (media, NULL);

  klass = RYGEL_GRILO_MEDIA_OBJECT_GET_CLASS (parent);
  dbus_path = g_strdup_printf ("%s/%u",
                               rygel_grilo_media_object_get_dbus_path (parent),
                               klass->index);

  obj =
    g_object_new (RYGEL_GRILO_MEDIA_ITEM_TYPE,
                  "parent", rygel_grilo_media_object_get_dbus_path (parent),
                  "dbus-path", dbus_path,
                  "grl-media", media,
                  "parent-media", parent,
                  NULL);

  g_free (dbus_path);
  if (rygel_grilo_media_object_dbus_register (RYGEL_GRILO_MEDIA_OBJECT (obj))) {
    klass->index++;
    return obj;
  } else {
    g_object_unref (obj);
    return NULL;
  }
}
