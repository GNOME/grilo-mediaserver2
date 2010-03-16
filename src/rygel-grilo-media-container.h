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

#ifndef _RYGEL_GRILO_MEDIA_CONTAINER_H_
#define _RYGEL_GRILO_MEDIA_CONTAINER_H_

#include <glib.h>
#include <glib-object.h>

#include "rygel-grilo-media-object.h"

#define RYGEL_GRILO_MEDIA_CONTAINER_TYPE        \
  (rygel_grilo_media_container_get_type ())

#define RYGEL_GRILO_MEDIA_CONTAINER(obj)                                \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj),                                   \
                               RYGEL_GRILO_MEDIA_CONTAINER_TYPE,        \
                               RygelGriloMediaContainer))

#define RYGEL_GRILO_IS_MEDIA_CONTAINER(obj)                             \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj),                                   \
                               RYGEL_GRILO_MEDIA_CONTAINER_TYPE))

#define RYGEL_GRILO_MEDIA_CONTAINER_CLASS(klass)                \
  (G_TYPE_CHECK_CLASS_CAST((klass),                             \
                           RYGEL_GRILO_MEDIA_CONTAINER_TYPE,    \
                           RygelGriloMediaContainerClass))

#define RYGEL_GRILO_IS_MEDIA_CONTAINER_CLASS(klass)             \
  (G_TYPE_CHECK_CLASS_TYPE((klass),                             \
                           RYGEL_GRILO_MEDIA_CONTAINER_TYPE))

#define RYGEL_GRILO_MEDIA_CONTAINER_GET_CLASS(obj)              \
  (G_TYPE_INSTANCE_GET_CLASS ((obj),                            \
                              RYGEL_GRILO_MEDIA_CONTAINER_TYPE, \
                              RygelGriloMediaContainerClass))

typedef struct _RygelGriloMediaContainer RygelGriloMediaContainer;

struct _RygelGriloMediaContainer {

  RygelGriloMediaObject parent;

};

typedef struct _RygelGriloMediaContainerClass RygelGriloMediaContainerClass;

struct _RygelGriloMediaContainerClass {

  RygelGriloMediaObjectClass parent_class;

  /*< private >*/
  /* How many elements will be requested per category */
  gint limit;
};

GType rygel_grilo_media_container_get_type (void);

RygelGriloMediaContainer *rygel_grilo_media_container_new (const gchar *parent,
                                                           const gchar *display_name);

gboolean rygel_grilo_media_container_get (RygelGriloMediaContainer *obj,
                                          const gchar *interface,
                                          const gchar *property,
                                          DBusGMethodInvocation *context,
                                          GError **error);

gboolean rygel_grilo_media_container_get_all (RygelGriloMediaContainer *obj,
                                              const gchar *interface,
                                              DBusGMethodInvocation *context,
                                              GError **error);

RygelGriloMediaContainer *rygel_grilo_media_container_new_root (const gchar *dbus_path,
                                                                GrlMedia *media,
                                                                gint limit);

RygelGriloMediaContainer *rygel_grilo_media_container_new_with_parent (RygelGriloMediaObject *parent,
                                                                       GrlMedia *media);


#endif /* _RYGEL_GRILO_MEDIA_CONTAINER_H_ */
