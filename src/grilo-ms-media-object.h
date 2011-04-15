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

#ifndef _GRILO_MS_MEDIA_OBJECT_H_
#define _GRILO_MS_MEDIA_OBJECT_H_

#include <glib.h>
#include <glib-object.h>
#include <grilo.h>

#define GRILO_MS_MEDIA_OBJECT_TYPE              \
  (grilo_ms_media_object_get_type ())

#define GRILO_MS_MEDIA_OBJECT(obj)                              \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj),                           \
                               GRILO_MS_MEDIA_OBJECT_TYPE,      \
                               GriloMsMediaObject))

#define GRILO_MS_IS_MEDIA_OBJECT(obj)                           \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj),                           \
                               GRILO_MS_MEDIA_OBJECT_TYPE))

#define GRILO_MS_MEDIA_OBJECT_CLASS(klass)              \
  (G_TYPE_CHECK_CLASS_CAST((klass),                     \
                           GRILO_MS_MEDIA_OBJECT_TYPE,  \
                           GriloMsMediaObjectClass))

#define GRILO_MS_IS_MEDIA_OBJECT_CLASS(klass)           \
  (G_TYPE_CHECK_CLASS_TYPE((klass),                     \
                           GRILO_MS_MEDIA_OBJECT_TYPE))

#define GRILO_MS_MEDIA_OBJECT_GET_CLASS(obj)                    \
  (G_TYPE_INSTANCE_GET_CLASS ((obj),                            \
                              GRILO_MS_MEDIA_OBJECT_TYPE,       \
                              GriloMsMediaObjectClass))

typedef struct _GriloMsMediaObject        GriloMsMediaObject;
typedef struct _GriloMsMediaObjectPrivate GriloMsMediaObjectPrivate;
struct _GriloMsMediaObject {

  GObject parent;

  /*< private >*/
  GriloMsMediaObjectPrivate *priv;
};

typedef struct _GriloMsMediaObjectClass GriloMsMediaObjectClass;

struct _GriloMsMediaObjectClass {

  GObjectClass parent_class;

  /*< private >*/
  /* A unique number used to form the dbus path name where object is
     registered */
  guint index;
};

GType grilo_ms_media_object_get_type (void);

gboolean grilo_ms_media_object_dbus_register (GriloMsMediaObject *obj);

const gchar *grilo_ms_media_object_get_dbus_path (GriloMsMediaObject *obj);

#endif /* _GRILO_MS_MEDIA_OBJECT_H_ */
