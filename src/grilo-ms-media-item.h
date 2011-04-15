/*
 * Copyright (C) 2010, 2011 Igalia S.L.
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

#ifndef _GRILO_MS_MEDIA_ITEM_H_
#define _GRILO_MS_MEDIA_ITEM_H_

#include <glib.h>
#include <glib-object.h>
#include <grilo.h>

#include "rygel-grilo-media-object.h"

#define GRILO_MS_MEDIA_ITEM_TYPE                \
  (grilo_ms_media_item_get_type ())

#define GRILO_MS_MEDIA_ITEM(obj)                                \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj),                           \
                               GRILO_MS_MEDIA_ITEM_TYPE,        \
                               GriloMsMediaItem))

#define GRILO_MS_IS_MEDIA_ITEM(obj)                             \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj),                           \
                               GRILO_MS_MEDIA_ITEM_TYPE))

#define GRILO_MS_MEDIA_ITEM_CLASS(klass)                \
  (G_TYPE_CHECK_CLASS_CAST((klass),                     \
                           GRILO_MS_MEDIA_ITEM_TYPE,    \
                           GriloMsMediaItemClass))

#define GRILO_MS_IS_MEDIA_ITEM_CLASS(klass)             \
  (G_TYPE_CHECK_CLASS_TYPE((klass),                     \
                           GRILO_MS_MEDIA_ITEM_TYPE))

#define GRILO_MS_MEDIA_ITEM_GET_CLASS(obj)              \
  (G_TYPE_INSTANCE_GET_CLASS ((obj),                    \
                              GRILO_MS_MEDIA_ITEM_TYPE, \
                              GriloMsMediaItemClass))

typedef struct _GriloMsMediaItem GriloMsMediaItem;

struct _GriloMsMediaItem {

  GriloMsMediaObject parent;

};

typedef struct _GriloMsMediaItemClass GriloMsMediaItemClass;

struct _GriloMsMediaItemClass {

  GriloMsMediaObjectClass parent_class;
};

GType grilo_ms_media_item_get_type (void);

GriloMsMediaItem *grilo_ms_media_item_new (GriloMsMediaObject *parent,
                                           GrlMedia *media);

#endif /* _GRILO_MS_MEDIA_ITEM_H_ */
