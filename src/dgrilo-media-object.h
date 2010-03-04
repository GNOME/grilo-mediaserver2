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

#ifndef _DGRILO_MEDIA_OBJECT_H_
#define _DGRILO_MEDIA_OBJECT_H_

#include <glib.h>
#include <glib-object.h>

#define DGRILO_MEDIA_OBJECT_TYPE                \
  (dgrilo_media_object_get_type ())

#define DGRILO_MEDIA_OBJECT(obj)                         \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj),                    \
                               DGRILO_MEDIA_OBJECT_TYPE, \
                               DGriloMediaObject))

#define DGRILO_IS_MEDIA_OBJECT(obj)                             \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj),                           \
                               DGRILO_MEDIA_OBJECT_TYPE))

#define DGRILO_MEDIA_OBJECT_CLASS(klass)                 \
  (G_TYPE_CHECK_CLASS_CAST((klass),                      \
                           DGRILO_MEDIA_OBJECT_TYPE,     \
                           DGriloMediaObjectClass))

#define DGRILO_IS_MEDIA_OBJECT_CLASS(klass)             \
  (G_TYPE_CHECK_CLASS_TYPE((klass),                     \
                           DGRILO_MEDIA_OBJECT_TYPE))

#define DGRILO_MEDIA_OBJECT_GET_CLASS(obj)               \
  (G_TYPE_INSTANCE_GET_CLASS ((obj),                     \
                              DGRILO_MEDIA_OBJECT_TYPE,  \
                              DGriloMediaObjectClass))

typedef struct _DGriloMediaObject DGriloMediaObject;

struct _DGriloMediaObject {

  GObject parent;

};

typedef struct _DGriloMediaObjectClass DGriloMediaObjectClass;

struct _DGriloMediaObjectClass {

  GObjectClass parent_class;

};

GType dgrilo_media_object_get_type (void);
DGriloMediaObject *dgrilo_media_object_new (const gchar *parent, const gchar *display_name);
gboolean dgrilo_media_object_get (DGriloMediaObject *obj,
                                  const gchar *interface,
                                  const gchar *property,
                                  DBusGMethodInvocation *context,
                                  GError **error);

#endif /* _DGRILO_MEDIA_OBJECT_H_ */
