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

#ifndef _MEDIA_SERVER1_OBSERVER_H_
#define _MEDIA_SERVER1_OBSERVER_H_

/* #include <gio/gio.h> */
#include <glib-object.h>
/* #include <glib.h> */

#include "media-server1-common.h"

#define MS1_TYPE_OBSERVER                       \
  (ms1_observer_get_type ())

#define MS1_OBSERVER(obj)                               \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj),                   \
                               MS1_TYPE_OBSERVER,       \
                               MS1Observer))

#define MS1_IS_OBSERVER(obj)                            \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj),                   \
                               MS1_TYPE_OBSERVER))

#define MS1_OBSERVER_CLASS(klass)               \
  (G_TYPE_CHECK_CLASS_CAST((klass),             \
                           MS1_TYPE_OBSERVER,   \
                           MS1ObserverClass))

#define MS1_IS_OBSERVER_CLASS(klass)            \
  (G_TYPE_CHECK_CLASS_TYPE((klass),             \
                           MS1_TYPE_OBSERVER))

#define MS1_OBSERVER_GET_CLASS(obj)                     \
  (G_TYPE_INSTANCE_GET_CLASS ((obj),                    \
                              MS1_TYPE_OBSERVER,        \
                              MS1ObserverClass))

typedef struct _MS1Observer        MS1Observer;
typedef struct _MS1ObserverPrivate MS1ObserverPrivate;

struct _MS1Observer {

  GObject parent;

  /*< private >*/
  MS1ObserverPrivate *priv;
};

typedef struct _MS1ObserverClass MS1ObserverClass;

struct _MS1ObserverClass {

  GObjectClass parent_class;

  void (*new) (MS1Observer *observer,
               const gchar *provider);
};

GType ms1_observer_get_type (void);

MS1Observer *ms1_observer_get_instance (void);

#endif /* _MEDIA_SERVER1_OBSERVER_H_ */
