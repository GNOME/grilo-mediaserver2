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

#ifndef _RYGEL_GRILO_MEDIA_ITEM_H_
#define _RYGEL_GRILO_MEDIA_ITEM_H_

#include <glib.h>
#include <glib-object.h>
#include <grilo.h>

#include "rygel-grilo-media-object.h"

#define RYGEL_GRILO_MEDIA_ITEM_TYPE             \
  (rygel_grilo_media_item_get_type ())

#define RYGEL_GRILO_MEDIA_ITEM(obj)                             \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj),                           \
                               RYGEL_GRILO_MEDIA_ITEM_TYPE,     \
                               RygelGriloMediaItem))

#define RYGEL_GRILO_IS_MEDIA_ITEM(obj)                          \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj),                           \
                               RYGEL_GRILO_MEDIA_ITEM_TYPE))

#define RYGEL_GRILO_MEDIA_ITEM_CLASS(klass)             \
  (G_TYPE_CHECK_CLASS_CAST((klass),                     \
                           RYGEL_GRILO_MEDIA_ITEM_TYPE, \
                           RygelGriloMediaItemClass))

#define RYGEL_GRILO_IS_MEDIA_ITEM_CLASS(klass)                  \
  (G_TYPE_CHECK_CLASS_TYPE((klass),                             \
                           RYGEL_GRILO_MEDIA_ITEM_TYPE))

#define RYGEL_GRILO_MEDIA_ITEM_GET_CLASS(obj)                   \
  (G_TYPE_INSTANCE_GET_CLASS ((obj),                            \
                              RYGEL_GRILO_MEDIA_ITEM_TYPE,      \
                              RygelGriloMediaItemClass))

typedef struct _RygelGriloMediaItem RygelGriloMediaItem;

struct _RygelGriloMediaItem {

  RygelGriloMediaObject parent;

};

typedef struct _RygelGriloMediaItemClass RygelGriloMediaItemClass;

struct _RygelGriloMediaItemClass {

  RygelGriloMediaObjectClass parent_class;
};

GType rygel_grilo_media_item_get_type (void);

RygelGriloMediaItem *rygel_grilo_media_item_new (RygelGriloMediaObject *parent,
                                                 GrlMedia *media);

#endif /* _RYGEL_GRILO_MEDIA_ITEM_H_ */
