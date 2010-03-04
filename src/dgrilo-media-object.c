/*
 * Copyright (C) 2010 Igalia S.L.
 *
 * Contact: Iago Toral Quiroga <itoral@igalia.com>
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

#include "dgrilo-media-object.h"

#define DGRILO_MEDIA_OBJECT_GET_PRIVATE(o)      \
  G_TYPE_INSTANCE_GET_PRIVATE((o), DGRILO_MEDIA_OBJECT_TYPE, DGriloMediaObjectPrivate)

enum {
  PROP_0,
  PROP_PARENT,
  PROP_DISPLAY_NAME,
  LAST_PROP
};

typedef struct {
  gchar *parent;
  gchar *name;
} DGriloMediaObjectPrivate;

G_DEFINE_TYPE (DGriloMediaObject, dgrilo_media_object, G_TYPE_OBJECT);

static void
get_property (GObject *object,
              guint prop_id,
              GValue *value,
              GParamSpec *pspec)
{
  DGriloMediaObject *self = DGRILO_MEDIA_OBJECT (object);
  DGriloMediaObjectPrivate *priv = DGRILO_MEDIA_OBJECT_GET_PRIVATE (self);

  switch (prop_id) {
  case PROP_PARENT:
    g_value_set_string (value, priv->parent);
    break;
  case PROP_DISPLAY_NAME:
    g_value_set_string (value, priv->name);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    break;
  }
}

static void
dispose (GObject *object)
{
  DGriloMediaObject *self = DGRILO_MEDIA_OBJECT (object);
  DGriloMediaObjectPrivate *priv = DGRILO_MEDIA_OBJECT_GET_PRIVATE (self);

  g_free (priv->parent);
  g_free (priv->name);

  G_OBJECT_CLASS (dgrilo_media_object_parent_class)->dispose (object);
}

static void
set_property (GObject *object,
              guint prop_id,
              const GValue *value,
              GParamSpec *pspec)
{
  DGriloMediaObject *self = DGRILO_MEDIA_OBJECT (object);
  DGriloMediaObjectPrivate *priv = DGRILO_MEDIA_OBJECT_GET_PRIVATE (self);

  switch (prop_id) {
  case PROP_PARENT:
    priv->parent = g_value_dup_string (value);
    break;
  case PROP_DISPLAY_NAME:
    priv->name = g_value_dup_string (value);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    break;
  }
}

static void
dgrilo_media_object_class_init (DGriloMediaObjectClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (DGriloMediaObjectPrivate));

  object_class->get_property = get_property;
  object_class->set_property = set_property;
  object_class->dispose = dispose;

  g_object_class_install_property (object_class,
                                   PROP_PARENT,
                                   g_param_spec_string ("parent",
                                                        "Parent",
                                                        "Parent",
                                                        NULL,
                                                        G_PARAM_READABLE | G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class,
                                   PROP_DISPLAY_NAME,
                                   g_param_spec_string ("display-name",
                                                        "DisplayName",
                                                        "Pretty name",
                                                        NULL,
                                                        G_PARAM_READABLE | G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY));
}

static void
dgrilo_media_object_init (DGriloMediaObject *server)
{
}

DGriloMediaObject *
dgrilo_media_object_new (const gchar *parent, const gchar *display_name)
{
  return g_object_new (DGRILO_MEDIA_OBJECT_TYPE,
                       "parent", parent,
                       "display-name", display_name,
                       NULL);
}
