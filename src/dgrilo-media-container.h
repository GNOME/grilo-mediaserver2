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

#ifndef _DGRILO_MEDIA_CONTAINER_H_
#define _DGRILO_MEDIA_CONTAINER_H_

#include <glib.h>
#include <glib-object.h>

#include "dgrilo-media-object.h"

#define DGRILO_MEDIA_CONTAINER_TYPE             \
  (dgrilo_media_container_get_type ())

#define DGRILO_MEDIA_CONTAINER(obj)                             \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj),                           \
                               DGRILO_MEDIA_CONTAINER_TYPE,     \
                               DGriloMediaContainer))

#define DGRILO_IS_MEDIA_CONTAINER(obj)                             \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj),                              \
                               DGRILO_MEDIA_CONTAINER_TYPE))

#define DGRILO_MEDIA_CONTAINER_CLASS(klass)                 \
  (G_TYPE_CHECK_CLASS_CAST((klass),                         \
                           DGRILO_MEDIA_CONTAINER_TYPE,     \
                           DGriloMediaContainerClass))

#define DGRILO_IS_MEDIA_CONTAINER_CLASS(klass)             \
  (G_TYPE_CHECK_CLASS_TYPE((klass),                        \
                           DGRILO_MEDIA_CONTAINER_TYPE))

#define DGRILO_MEDIA_CONTAINER_GET_CLASS(obj)                   \
  (G_TYPE_INSTANCE_GET_CLASS ((obj),                            \
                              DGRILO_MEDIA_CONTAINER_TYPE,      \
                              DGriloMediaContainerClass))

typedef struct _DGriloMediaContainer DGriloMediaContainer;

struct _DGriloMediaContainer {

  DGriloMediaObject parent;

};

typedef struct _DGriloMediaContainerClass DGriloMediaContainerClass;

struct _DGriloMediaContainerClass {

  DGriloMediaObjectClass parent_class;

  /*< private >*/
  gint limit;
};

GType dgrilo_media_container_get_type (void);

DGriloMediaContainer *dgrilo_media_container_new (const gchar *parent, const gchar *display_name);

gboolean dgrilo_media_container_get (DGriloMediaContainer *obj,
                                     const gchar *interface,
                                     const gchar *property,
                                     DBusGMethodInvocation *context,
                                     GError **error);

gboolean dgrilo_media_container_get_all (DGriloMediaContainer *obj,
                                         const gchar *interface,
                                         DBusGMethodInvocation *context,
                                         GError **error);

DGriloMediaContainer *dgrilo_media_container_new_root (const gchar *dbus_path,
                                                       GrlMedia *media,
                                                       gint limit);

DGriloMediaContainer *dgrilo_media_container_new_with_parent (DGriloMediaObject *parent,
                                                              GrlMedia *media);


#endif /* _DGRILO_MEDIA_CONTAINER_H_ */
