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

#ifndef _MEDIA_SERVER2_OBSERVER_H_
#define _MEDIA_SERVER2_OBSERVER_H_

/* #include <gio/gio.h> */
#include <glib-object.h>
/* #include <glib.h> */

#include "media-server2-common.h"

#define MS2_TYPE_OBSERVER                       \
  (ms2_observer_get_type ())

#define MS2_OBSERVER(obj)                               \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj),                   \
                               MS2_TYPE_OBSERVER,       \
                               MS2Observer))

#define MS2_IS_OBSERVER(obj)                            \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj),                   \
                               MS2_TYPE_OBSERVER))

#define MS2_OBSERVER_CLASS(klass)               \
  (G_TYPE_CHECK_CLASS_CAST((klass),             \
                           MS2_TYPE_OBSERVER,   \
                           MS2ObserverClass))

#define MS2_IS_OBSERVER_CLASS(klass)            \
  (G_TYPE_CHECK_CLASS_TYPE((klass),             \
                           MS2_TYPE_OBSERVER))

#define MS2_OBSERVER_GET_CLASS(obj)                     \
  (G_TYPE_INSTANCE_GET_CLASS ((obj),                    \
                              MS2_TYPE_OBSERVER,        \
                              MS2ObserverClass))

typedef struct _MS2Observer        MS2Observer;
typedef struct _MS2ObserverPrivate MS2ObserverPrivate;

struct _MS2Observer {

  GObject parent;

  /*< private >*/
  MS2ObserverPrivate *priv;
};

typedef struct _MS2ObserverClass MS2ObserverClass;

struct _MS2ObserverClass {

  GObjectClass parent_class;

  void (*new) (MS2Observer *observer,
               const gchar *provider);
};

GType ms2_observer_get_type (void);

MS2Observer *ms2_observer_get_instance (void);

#endif /* _MEDIA_SERVER2_OBSERVER_H_ */
