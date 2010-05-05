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

#ifndef _MEDIA_SERVER1_PRIVATE_H_
#define _MEDIA_SERVER1_PRIVATE_H_

#include "media-server1-client.h"
#include "media-server1-server.h"

#define MS1_DBUS_SERVICE_PREFIX "org.gnome.UPnP.MediaServer1."
#define MS1_DBUS_PATH_PREFIX    "/org/gnome/UPnP/MediaServer1/"

#define MS1_DBUS_SERVICE_PREFIX_LENGTH 28

void ms1_client_notify_unref (MS1Client *client);

void ms1_observer_add_client (MS1Client *client, const gchar *provider);

void ms1_observer_remove_client (MS1Client *client, const gchar *provider);

#endif /* _MEDIA_SERVER1_PRIVATE_H_ */
