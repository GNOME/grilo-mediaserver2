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

#ifndef _DGRILO_MEDIA_ITEM_H_
#define _DGRILO_MEDIA_ITEM_H_

#include <glib.h>
#include <glib-object.h>
#include <grilo.h>

#include "dgrilo-media-object.h"

#define DGRILO_MEDIA_ITEM_TYPE                  \
  (dgrilo_media_item_get_type ())

#define DGRILO_MEDIA_ITEM(obj)                          \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj),                   \
                               DGRILO_MEDIA_ITEM_TYPE,  \
                               DGriloMediaItem))

#define DGRILO_IS_MEDIA_ITEM(obj)                       \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj),                   \
                               DGRILO_MEDIA_ITEM_TYPE))

#define DGRILO_MEDIA_ITEM_CLASS(klass)                  \
  (G_TYPE_CHECK_CLASS_CAST((klass),                     \
                           DGRILO_MEDIA_ITEM_TYPE,      \
                           DGriloMediaItemClass))

#define DGRILO_IS_MEDIA_ITEM_CLASS(klass)               \
  (G_TYPE_CHECK_CLASS_TYPE((klass),                     \
                           DGRILO_MEDIA_ITEM_TYPE))

#define DGRILO_MEDIA_ITEM_GET_CLASS(obj)                \
  (G_TYPE_INSTANCE_GET_CLASS ((obj),                    \
                              DGRILO_MEDIA_ITEM_TYPE,   \
                              DGriloMediaItemClass))

typedef struct _DGriloMediaItem DGriloMediaItem;

struct _DGriloMediaItem {

  DGriloMediaObject parent;

};

typedef struct _DGriloMediaItemClass DGriloMediaItemClass;

struct _DGriloMediaItemClass {

  DGriloMediaObjectClass parent_class;
};

GType dgrilo_media_item_get_type (void);

DGriloMediaItem *dgrilo_media_item_new (DGriloMediaObject *parent,
                                        GrlMedia *media);

#endif /* _DGRILO_MEDIA_ITEM_H_ */
