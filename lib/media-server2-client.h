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

#ifndef _MEDIA_SERVER2_CLIENT_H_
#define _MEDIA_SERVER2_CLIENT_H_

#include <glib.h>
#include <glib-object.h>

#include "media-server2-common.h"

#define MS2_TYPE_CLIENT                         \
  (ms2_client_get_type ())

#define MS2_CLIENT(obj)                         \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj),           \
                               MS2_TYPE_CLIENT, \
                               MS2Client))

#define MS2_IS_CLIENT(obj)                              \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj),                   \
                               MS2_TYPE_CLIENT))

#define MS2_CLIENT_CLASS(klass)                 \
  (G_TYPE_CHECK_CLASS_CAST((klass),             \
                           MS2_TYPE_CLIENT,     \
                           MS2ClientClass))

#define MS2_IS_CLIENT_CLASS(klass)              \
  (G_TYPE_CHECK_CLASS_TYPE((klass),             \
                           MS2_TYPE_CLIENT))

#define MS2_CLIENT_GET_CLASS(obj)               \
  (G_TYPE_INSTANCE_GET_CLASS ((obj),            \
                              MS2_TYPE_CLIENT,  \
                              MS2ClientClass))

typedef struct _MS2Client        MS2Client;
typedef struct _MS2ClientPrivate MS2ClientPrivate;

struct _MS2Client {

  GObject parent;

  /*< private >*/
  MS2ClientPrivate *priv;
};

typedef struct _MS2ClientClass MS2ClientClass;

struct _MS2ClientClass {

  GObjectClass parent_class;
};

GType ms2_client_get_type (void);

gchar **ms2_client_get_providers (void);

MS2Client *ms2_client_new (const gchar *provider);

GHashTable *ms2_client_get_properties (MS2Client *client,
                                       const gchar *id,
                                       const gchar **properties,
                                       GError **error);

GList *ms2_client_get_children (MS2Client *client,
                                const gchar *id,
                                guint offset,
                                gint max_count,
                                const gchar **properties,
                                GError **error);

#endif /* _MEDIA_SERVER2_CLIENT_H_ */
