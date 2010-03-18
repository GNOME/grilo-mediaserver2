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

#ifndef _RYGEL_GRILO_MEDIA_OBJECT_H_
#define _RYGEL_GRILO_MEDIA_OBJECT_H_

#include <glib.h>
#include <glib-object.h>
#include <grilo.h>

#define RYGEL_GRILO_MEDIA_OBJECT_TYPE           \
  (rygel_grilo_media_object_get_type ())

#define RYGEL_GRILO_MEDIA_OBJECT(obj)                           \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj),                           \
                               RYGEL_GRILO_MEDIA_OBJECT_TYPE,   \
                               RygelGriloMediaObject))

#define RYGEL_GRILO_IS_MEDIA_OBJECT(obj)                        \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj),                           \
                               RYGEL_GRILO_MEDIA_OBJECT_TYPE))

#define RYGEL_GRILO_MEDIA_OBJECT_CLASS(klass)                   \
  (G_TYPE_CHECK_CLASS_CAST((klass),                             \
                           RYGEL_GRILO_MEDIA_OBJECT_TYPE,       \
                           RygelGriloMediaObjectClass))

#define RYGEL_GRILO_IS_MEDIA_OBJECT_CLASS(klass)                \
  (G_TYPE_CHECK_CLASS_TYPE((klass),                             \
                           RYGEL_GRILO_MEDIA_OBJECT_TYPE))

#define RYGEL_GRILO_MEDIA_OBJECT_GET_CLASS(obj)                 \
  (G_TYPE_INSTANCE_GET_CLASS ((obj),                            \
                              RYGEL_GRILO_MEDIA_OBJECT_TYPE,    \
                              RygelGriloMediaObjectClass))

typedef struct _RygelGriloMediaObject RygelGriloMediaObject;

struct _RygelGriloMediaObject {

  GObject parent;

};

typedef struct _RygelGriloMediaObjectClass RygelGriloMediaObjectClass;

struct _RygelGriloMediaObjectClass {

  GObjectClass parent_class;

  /*< private >*/
  /* A unique number used to form the dbus path name where object is
     registered */
  guint index;
};

GType rygel_grilo_media_object_get_type (void);

gboolean rygel_grilo_media_object_dbus_register (RygelGriloMediaObject *obj);

const gchar *rygel_grilo_media_object_get_dbus_path (RygelGriloMediaObject *obj);

#endif /* _RYGEL_GRILO_MEDIA_OBJECT_H_ */
