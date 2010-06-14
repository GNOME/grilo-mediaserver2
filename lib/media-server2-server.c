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

#include <dbus/dbus-glib-lowlevel.h>
#include <stdlib.h>

#include "media-server2-private.h"
#include "media-server2-server.h"
#include "media-server2-introspection.h"

#define INTROSPECTION_FILE                                              \
  "/home/jasuarez/Projects/grilo/rygel-grilo/data/media-server2.xml"

#define DBUS_TYPE_G_ARRAY_OF_STRING                             \
  (dbus_g_type_get_collection ("GPtrArray", G_TYPE_STRING))

#define MS2_SERVER_GET_PRIVATE(o)                                       \
  G_TYPE_INSTANCE_GET_PRIVATE((o), MS2_TYPE_SERVER, MS2ServerPrivate)

/*
 * Private MS2Server structure
 *   name: provider name
 *   data: holds stuff for owner
 *   list_children: function to get children
 *   search_objects: function to search objects
 *   get_properties: function to get properties
 */
struct _MS2ServerPrivate {
  gchar *name;
  gpointer *data;
  ListChildrenFunc list_children;
  SearchObjectsFunc search_objects;
  GetPropertiesFunc get_properties;
};

/* dbus message signatures */
static const gchar introspect_sgn[] = { DBUS_TYPE_INVALID };

static const gchar get_sgn[]  = { DBUS_TYPE_STRING, /* interface */
                                  DBUS_TYPE_STRING, /* property */
                                  DBUS_TYPE_INVALID };

static const gchar getall_sgn[] = { DBUS_TYPE_STRING, /* interface */
                                    DBUS_TYPE_INVALID };

static const gchar listchildren_sgn[] = { DBUS_TYPE_UINT32,                  /* offset */
                                          DBUS_TYPE_UINT32,                  /* max */
                                          DBUS_TYPE_ARRAY, DBUS_TYPE_STRING, /* filter */
                                          DBUS_TYPE_INVALID };

static const gchar searchobjects_sgn[] = { DBUS_TYPE_STRING,                  /* query */
                                           DBUS_TYPE_UINT32,                  /* offset */
                                           DBUS_TYPE_UINT32,                  /* max */
                                           DBUS_TYPE_ARRAY, DBUS_TYPE_STRING, /* filter */
                                           DBUS_TYPE_INVALID };


/* Properties in interfaces */
static const gchar *mediaobject1_properties[] = { MS2_PROP_PARENT,
                                                  MS2_PROP_TYPE,
                                                  MS2_PROP_PATH,
                                                  MS2_PROP_DISPLAY_NAME,
                                                  NULL };

static const gchar *mediaitem1_properties[] = { MS2_PROP_URLS,
                                                MS2_PROP_MIME_TYPE,
                                                MS2_PROP_SIZE,
                                                MS2_PROP_ARTIST,
                                                MS2_PROP_ALBUM,
                                                MS2_PROP_DATE,
                                                MS2_PROP_GENRE,
                                                MS2_PROP_DLNA_PROFILE,
                                                MS2_PROP_DURATION,
                                                MS2_PROP_BITRATE,
                                                MS2_PROP_SAMPLE_RATE,
                                                MS2_PROP_BITS_PER_SAMPLE,
                                                MS2_PROP_WIDTH,
                                                MS2_PROP_HEIGHT,
                                                MS2_PROP_COLOR_DEPTH,
                                                MS2_PROP_PIXEL_WIDTH,
                                                MS2_PROP_PIXEL_HEIGHT,
                                                MS2_PROP_THUMBNAIL,
                                                MS2_PROP_ALBUM_ART,
                                                NULL };

static const gchar *mediacontainer1_properties[] = { MS2_PROP_CHILD_COUNT,
                                                     MS2_PROP_ITEMS,
                                                     MS2_PROP_ITEM_COUNT,
                                                     MS2_PROP_CONTAINERS,
                                                     MS2_PROP_CONTAINER_COUNT,
                                                     MS2_PROP_SEARCHABLE,
                                                     NULL };

G_DEFINE_TYPE (MS2Server, ms2_server, G_TYPE_OBJECT);

/******************** PRIVATE API ********************/

/* Free gvalue */
static void
free_value (GValue *value)
{
  g_value_unset (value);
  g_free (value);
}

/* Puts a string in a gvalue */
static GValue *
str_to_value (const gchar *str)
{
  GValue *val = NULL;

  if (str) {
    val = g_new0 (GValue, 1);
    g_value_init (val, G_TYPE_STRING);
    g_value_set_string (val, str);
  }

  return val;
}

/* Puts an int in a gvalue */
static GValue *
int_to_value (gint number)
{
  GValue *val = NULL;

  val = g_new0 (GValue, 1);
  g_value_init (val, G_TYPE_INT);
  g_value_set_int (val, number);

  return val;
}

/* Puts an uint in a gvalue */
static GValue *
uint_to_value (guint number)
{
  GValue *val = NULL;

  val = g_new0 (GValue, 1);
  g_value_init (val, G_TYPE_UINT);
  g_value_set_uint (val, number);

  return val;
}

/* Puts an bool in a gvalue */
static GValue *
bool_to_value (gboolean b)
{
  GValue *val = NULL;

  val = g_new0 (GValue, 1);
  g_value_init (val, G_TYPE_BOOLEAN);
  g_value_set_boolean (val, b);

  return val;
}

/* Puts a gptrarray in a gvalue */
static GValue *
ptrarray_to_value (GPtrArray *array)
{
  GValue *val = NULL;

  val = g_new0 (GValue, 1);
  g_value_init (val, DBUS_TYPE_G_ARRAY_OF_STRING);
  g_value_take_boxed (val, array);

  return val;
}

/* Search for string needle in NULL-terminated string array strv; search is done
   by comparing pointers */
static gboolean
lookup_in_strv (gchar **strv,
                const gchar *needle)
{
  gint i;
  if (!strv || !needle) {
    return FALSE;
  }

  for (i = 0; strv[i] && strv[i] != needle; i++);
  return (strv[i] != NULL);
}

/* Searches for property in properties and return its value; if is not found,
   then returns a default value for that property */
static GValue *
properties_lookup_with_default (GHashTable *properties,
                                const gchar *property)
{
  GPtrArray *ptrarray;
  GValue *ret_value;
  GValue *propvalue;
  const gchar *intern_property;
  static gchar **int_type_properties = NULL;
  static gchar **uint_type_properties = NULL;
  static gchar **bool_type_properties = NULL;
  static gchar **gptrarray_type_properties = NULL;

  /* Initialize data */
  if (!int_type_properties) {
    int_type_properties = g_new (gchar *, 11);
    int_type_properties[0] = (gchar *) g_intern_static_string (MS2_PROP_SIZE);
    int_type_properties[1] = (gchar *) g_intern_static_string (MS2_PROP_DURATION);
    int_type_properties[2] = (gchar *) g_intern_static_string (MS2_PROP_BITRATE);
    int_type_properties[3] = (gchar *) g_intern_static_string (MS2_PROP_SAMPLE_RATE);
    int_type_properties[4] = (gchar *) g_intern_static_string (MS2_PROP_BITS_PER_SAMPLE);
    int_type_properties[5] = (gchar *) g_intern_static_string (MS2_PROP_WIDTH);
    int_type_properties[6] = (gchar *) g_intern_static_string (MS2_PROP_HEIGHT);
    int_type_properties[7] = (gchar *) g_intern_static_string (MS2_PROP_COLOR_DEPTH);
    int_type_properties[8] = (gchar *) g_intern_static_string (MS2_PROP_PIXEL_WIDTH);
    int_type_properties[9] = (gchar *) g_intern_static_string (MS2_PROP_PIXEL_HEIGHT);
    int_type_properties[10] = NULL;
  }

  if (!uint_type_properties) {
    uint_type_properties = g_new (gchar *, 4);
    uint_type_properties[0] = (gchar *) g_intern_static_string (MS2_PROP_CHILD_COUNT);
    uint_type_properties[1] = (gchar *) g_intern_static_string (MS2_PROP_ITEM_COUNT);
    uint_type_properties[2] = (gchar *) g_intern_static_string (MS2_PROP_CONTAINER_COUNT);
    uint_type_properties[3] = NULL;
  }

  if (!bool_type_properties) {
    bool_type_properties = g_new (gchar *, 2);
    bool_type_properties[0] = (gchar *) g_intern_static_string (MS2_PROP_SEARCHABLE);
    bool_type_properties[1] = NULL;
  }

  if (!gptrarray_type_properties) {
    gptrarray_type_properties = g_new (gchar *, 4);
    gptrarray_type_properties[0] = (gchar *) g_intern_static_string (MS2_PROP_URLS);
    gptrarray_type_properties[1] = (gchar *) g_intern_static_string (MS2_PROP_ITEMS);
    gptrarray_type_properties[2] = (gchar *) g_intern_static_string (MS2_PROP_CONTAINERS);
    gptrarray_type_properties[3] = NULL;
  }

  if (properties) {
    propvalue = g_hash_table_lookup (properties, property);
  } else {
    propvalue = NULL;
  }

  if (propvalue) {
    /* Make a copy and return it */
    ret_value = g_new0 (GValue, 1);
    g_value_init (ret_value, G_VALUE_TYPE (propvalue));
    g_value_copy (propvalue, ret_value);

    return ret_value;
  }

  /* Use a default value */
  intern_property = g_intern_string (property);
  if (lookup_in_strv (int_type_properties, intern_property)) {
    ret_value = int_to_value (MS2_UNKNOWN_INT);
  } else if (lookup_in_strv (uint_type_properties, intern_property)) {
    ret_value = uint_to_value (MS2_UNKNOWN_UINT);
  } else if (lookup_in_strv (bool_type_properties, intern_property)) {
    ret_value = bool_to_value (FALSE);
  } else if (lookup_in_strv (gptrarray_type_properties, intern_property)) {
    ptrarray = g_ptr_array_sized_new (0);
    ret_value = ptrarray_to_value (ptrarray);
  } else {
    ret_value = str_to_value (MS2_UNKNOWN_STR);
  }

  return ret_value;
}

/* Check if property makes sense in the interface */
static gboolean
is_property_valid (const gchar *interface,
                   const gchar *property)
{
  const gchar *prop_intern;
  gboolean found;
  int i;
  static gchar **mo1_properties_intern = NULL;
  static gchar **mi1_properties_intern = NULL;
  static gchar **mc1_properties_intern = NULL;

  /* Initialize MediaObject1 properties interns */
  if (!mo1_properties_intern) {
    mo1_properties_intern = g_new (gchar *,
                                   g_strv_length ((gchar **) mediaobject1_properties) + 1);
    for (i = 0; mediaobject1_properties[i]; i++) {
      mo1_properties_intern[i] =
        (gchar *) g_intern_static_string (mediaobject1_properties[i]);
    }
    mo1_properties_intern[i] = NULL;
  }

  /* Initialize MediaItem1 properties interns */
  if (!mi1_properties_intern) {
    mi1_properties_intern = g_new (gchar *,
                                   g_strv_length ((gchar **) mediaitem1_properties) + 1);
    for (i = 0; mediaitem1_properties[i]; i++) {
      mi1_properties_intern[i] =
        (gchar *) g_intern_static_string (mediaitem1_properties[i]);
    }
    mi1_properties_intern[i] = NULL;
  }

  /* Initialize MediaContainer1 properties interns */
  if (!mc1_properties_intern) {
    mc1_properties_intern = g_new (gchar *,
                                   g_strv_length ((gchar **) mediacontainer1_properties) + 1);
    for (i = 0; mediacontainer1_properties[i]; i++) {
      mc1_properties_intern[i] =
        (gchar *) g_intern_static_string (mediacontainer1_properties[i]);
    }
    mc1_properties_intern[i] = NULL;
  }

  prop_intern = g_intern_string (property);

  /* Check MediaObject1 interface */
  if (!interface || g_strcmp0 (interface, "org.gnome.UPnP.MediaObject1") == 0) {
    found = lookup_in_strv (mo1_properties_intern, prop_intern);

    if (found) {
      return TRUE;
    }

    /* If not found, but interface is NULL, maybe property is in next interface */
    if (!found && interface) {
      return FALSE;
    }
  }

  /* Check MediaItem1 interface */
  if (!interface || g_strcmp0 (interface, "org.gnome.UPnP.MediaItem1") == 0) {
    found = lookup_in_strv (mi1_properties_intern, prop_intern);

    if (found) {
      return TRUE;
    }

    /* If not found, but interface is NULL, maybe property is in next interface */
    if (!found && interface) {
      return FALSE;
    }
  }

  /* Check MediaContainer1 interface */
  if (!interface || g_strcmp0 (interface, "org.gnome.UPnP.MediaContainer1") == 0) {
    return lookup_in_strv (mc1_properties_intern, prop_intern);
  }

  return FALSE;
}

/* Returns the id suitable to be used with server backend */
static gchar *
get_id_from_message (DBusMessage *m)
{
  gchar **path;
  gchar *id;
  gint path_length;

  dbus_message_get_path_decomposed (m, &path);

  /* Path can of type:
     /org/gnome/UPnP/MediaServer2/<name>
     /org/gnome/UPnP/MediaServer2/<name>/items/<id>
     /org/gnome/UPnP/MediaServer2/<name>/containers/<id>
  */
  path_length = g_strv_length (path);

  if (path_length == 5) {
    id = g_strdup (MS2_ROOT);
  } else if (path_length == 7) {
    id =  g_strdup (g_quark_to_string (atoi (path[6])));
  } else {
    id = NULL;
  }

  dbus_free_string_array (path);

  return id;
}

/* Request value of property in the interface */
static GValue *
get_property_value (MS2Server *server,
                    DBusMessage *message,
                    const gchar *interface,
                    const gchar *property)
{
  GHashTable *propresult;
  GValue *v;
  const gchar *prop[2] = { NULL };
  gchar *id;

  /* Check everything is right */
  if (!property ||
      !is_property_valid (interface, property) ||
      !server->priv->get_properties) {
    return NULL;
  }

  /* If asking for Path, we already can use object_path */
  if (g_strcmp0 (property, MS2_PROP_PATH) == 0) {
    v = g_new0 (GValue, 1);
    g_value_init (v, G_TYPE_STRING);
    g_value_set_string (v, dbus_message_get_path (message));
  } else {
    id = get_id_from_message (message);
    prop[0] = property;
    propresult = server->priv->get_properties (server,
                                               id,
                                               prop,
                                               server->priv->data,
                                               NULL);
    g_free (id);
    v = properties_lookup_with_default (propresult, property);

    if (propresult) {
      g_hash_table_unref (propresult);
    }
  }

  return v;
}

/* Adds a GPtrArray as an array of strings to dbus message */
static void
add_gptrarray_as_as (DBusMessage *m,
                     DBusMessageIter *iter,
                     GPtrArray *a)
{
  DBusMessageIter iternew;
  DBusMessageIter sub_array;
  gint i;

  if (!iter) {
    dbus_message_iter_init_append (m, &iternew);
    iter = &iternew;
  }

  dbus_message_iter_open_container (iter, DBUS_TYPE_ARRAY, "s", &sub_array);
  /* Add array */
  for (i = 0; i < a->len; i++) {
    dbus_message_iter_append_basic (&sub_array,
                                    DBUS_TYPE_STRING,
                                    &(g_ptr_array_index (a, i)));
  }
  dbus_message_iter_close_container (iter, &sub_array);
}

/* Adds a GPtrArray as an array of object paths to dbus message */
static void
add_gptrarray_as_os (DBusMessage *m,
                     DBusMessageIter *iter,
                     GPtrArray *a)
{
  DBusMessageIter iternew;
  DBusMessageIter sub_array;
  gint i;

  if (!iter) {
    dbus_message_iter_init_append (m, &iternew);
    iter = &iternew;
  }

  dbus_message_iter_open_container (iter, DBUS_TYPE_ARRAY, "o", &sub_array);

  /* Add array */
  for (i = 0; i < a->len; i++) {
    dbus_message_iter_append_basic (&sub_array,
                                    DBUS_TYPE_OBJECT_PATH,
                                    &(g_ptr_array_index (a, i)));
  }

  dbus_message_iter_close_container (iter, &sub_array);
}

/* Adds a pair <key, v> to dbus message */
static void
add_variant (DBusMessage *m,
             DBusMessageIter *iter,
             const gchar *key,
             const GValue *v)
{
  DBusMessageIter iternew;
  DBusMessageIter sub;
  const gchar *str_value;
  gboolean bool_value;
  gint int_value;
  guint uint_value;

  if (!iter) {
    dbus_message_iter_init_append (m, &iternew);
    iter = &iternew;
  }

  if (G_VALUE_HOLDS_STRING (v)) {
    str_value = g_value_get_string (v);
    dbus_message_iter_open_container (iter, DBUS_TYPE_VARIANT, "s", &sub);
    dbus_message_iter_append_basic (&sub, DBUS_TYPE_STRING, &str_value);
    dbus_message_iter_close_container (iter, &sub);
  } else if (G_VALUE_HOLDS_INT (v)) {
    int_value = g_value_get_int (v);
    dbus_message_iter_open_container (iter, DBUS_TYPE_VARIANT, "i", &sub);
    dbus_message_iter_append_basic (&sub, DBUS_TYPE_INT32, &int_value);
    dbus_message_iter_close_container (iter, &sub);
  } else if (G_VALUE_HOLDS_UINT (v)) {
    uint_value = g_value_get_uint (v);
    dbus_message_iter_open_container (iter, DBUS_TYPE_VARIANT, "u", &sub);
    dbus_message_iter_append_basic (&sub, DBUS_TYPE_UINT32, &uint_value);
    dbus_message_iter_close_container (iter, &sub);
  } else if (G_VALUE_HOLDS_BOOLEAN (v)) {
    bool_value = g_value_get_boolean (v);
    dbus_message_iter_open_container (iter, DBUS_TYPE_VARIANT, "b", &sub);
    dbus_message_iter_append_basic (&sub, DBUS_TYPE_BOOLEAN, &bool_value);
    dbus_message_iter_close_container (iter, &sub);
  } else if (G_VALUE_HOLDS_BOXED (v)) {
    if (g_strcmp0 (key, "URLs") == 0) {
      dbus_message_iter_open_container (iter, DBUS_TYPE_VARIANT, "as", &sub);
      add_gptrarray_as_as (m, &sub, g_value_get_boxed (v));
    } else {
      dbus_message_iter_open_container (iter, DBUS_TYPE_VARIANT, "ao", &sub);
      add_gptrarray_as_os (m, &sub, g_value_get_boxed (v));
    }
    dbus_message_iter_close_container (iter, &sub);
  }
}

/* Adds a GHashTable as a dictionary to dbus message */
static void
add_hashtable_as_dict (DBusMessage *m,
                       DBusMessageIter *iter,
                       GHashTable *t)
{
  DBusMessageIter iternew;
  DBusMessageIter sub_array;
  DBusMessageIter sub_dict;
  GList *key;
  GList *keys;
  GValue *v;

  if (!iter) {
    dbus_message_iter_init_append (m, &iternew);
    iter = &iternew;
  }

  /* Add an array of dict */
  dbus_message_iter_open_container (iter, DBUS_TYPE_ARRAY, "{sv}", &sub_array);
  /* Add hashtable */
  if (t) {
    keys = g_hash_table_get_keys (t);
    for (key = keys; key; key = g_list_next (key)) {
      v = g_hash_table_lookup (t, key->data);
      if (!v) {
        continue;
      }

      /* Add type dict */
      dbus_message_iter_open_container (&sub_array, DBUS_TYPE_DICT_ENTRY, NULL, &sub_dict);
      /* Add key & value */
      dbus_message_iter_append_basic (&sub_dict, DBUS_TYPE_STRING, &key->data);
      add_variant (m, &sub_dict, key->data, v);
      dbus_message_iter_close_container (&sub_array, &sub_dict);
    }
    g_list_free (keys);
  }
  dbus_message_iter_close_container (iter, &sub_array);
}

/* Adds a GList as an array of pairs <string, variant> to dbus message */
static void
add_glist_as_array (DBusMessage *m,
                    DBusMessageIter *iter,
                    GList *l)
{
  DBusMessageIter iternew;
  DBusMessageIter sub_array;

  if (!iter) {
    dbus_message_iter_init_append (m, &iternew);
    iter = &iternew;
  }

  /* Add an array */
  dbus_message_iter_open_container (iter, DBUS_TYPE_ARRAY, "a{sv}", &sub_array);
  while (l) {
    add_hashtable_as_dict (m, &sub_array, l->data);
    l = g_list_next (l);
  }

  dbus_message_iter_close_container (iter, &sub_array);
}

/* Introspect message handler */
static DBusHandlerResult
handle_introspect_message (DBusConnection *c,
                           DBusMessage *m,
                           void *userdata,
                           const gchar *xml)
{
  DBusMessage *r;

  /* Check signature */
  if (dbus_message_has_signature (m, introspect_sgn)) {
    r = dbus_message_new_method_return (m);
    dbus_message_append_args (r, DBUS_TYPE_STRING, &xml, DBUS_TYPE_INVALID);
    dbus_connection_send (c, r, NULL);
    dbus_message_unref (r);
    return DBUS_HANDLER_RESULT_HANDLED;
  } else {
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
  }
}

/* Get message handler */
static DBusHandlerResult
handle_get_message (DBusConnection *c,
                    DBusMessage *m,
                    void *userdata)
{
  GValue *value;
  DBusMessage *r;
  gchar *interface = NULL;
  gchar *property = NULL;
  MS2Server *server = MS2_SERVER (userdata);

  /* Check signature */
  if (dbus_message_has_signature (m, get_sgn)) {
    dbus_message_get_args (m, NULL,
                           DBUS_TYPE_STRING, &interface,
                           DBUS_TYPE_STRING, &property,
                           DBUS_TYPE_INVALID);
    value = get_property_value (server, m, interface, property);
    if (!value) {
      g_printerr ("Invalid property %s in interface %s\n",
                  property,
                  interface);
    } else {
      r = dbus_message_new_method_return (m);
      add_variant (r, NULL, property, value);
      dbus_connection_send (c, r, NULL);
      dbus_message_unref (r);
      free_value (value);
    }
    return DBUS_HANDLER_RESULT_HANDLED;
  } else {
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
  }
}

/* GetAll message handler */
static DBusHandlerResult
handle_get_all_message (DBusConnection *c,
                        DBusMessage *m,
                        void *userdata)
{
  DBusMessage *r;
  GHashTable *propresult;
  MS2Server *server = MS2_SERVER (userdata);
  const gchar **prop;
  gchar *id;
  gchar *interface;

  /* Check signature */
  if (dbus_message_has_signature (m, getall_sgn)) {
    dbus_message_get_args (m, NULL,
                           DBUS_TYPE_STRING, &interface,
                           DBUS_TYPE_INVALID);
    /* Get what properties we should ask */
    if (g_strcmp0 (interface, "org.gnome.UPnP.MediaObject1") == 0) {
      prop = mediaobject1_properties;
    } else if (g_strcmp0 (interface, "org.gnome.UPnP.MediaItem1") == 0) {
      prop = mediaitem1_properties;
    } else if (g_strcmp0 (interface, "org.gnome.UPnP.MediaContainer1") == 0) {
      prop = mediacontainer1_properties;
    } else {
      return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    }

    if (prop && server->priv->get_properties) {
      id = get_id_from_message (m);
      if (!id) {
        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
      }

      propresult = server->priv->get_properties (server,
                                                 id,
                                                 prop,
                                                 server->priv->data,
                                                 NULL);
      g_free (id);
    } else {
      propresult = NULL;
    }
    r = dbus_message_new_method_return (m);
    add_hashtable_as_dict (r, NULL, propresult);
    dbus_connection_send (c, r, NULL);
    dbus_message_unref (r);
    if (propresult) {
      g_hash_table_unref (propresult);
    }
    return DBUS_HANDLER_RESULT_HANDLED;
  } else {
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
  }
}

/* ListChildren message handler */
static DBusHandlerResult
handle_list_children_message (DBusConnection *c,
                              DBusMessage *m,
                              void *userdata)
{
  DBusMessage *r;
  GList *children;
  gchar **filter;
  gchar *id;
  guint max_count;
  guint offset;
  gint nitems;
  MS2Server *server = MS2_SERVER (userdata);

  /* Check signature */
  if (dbus_message_has_signature (m, listchildren_sgn)) {
    dbus_message_get_args (m, NULL,
                           DBUS_TYPE_UINT32, &offset,
                           DBUS_TYPE_UINT32, &max_count,
                           DBUS_TYPE_ARRAY, DBUS_TYPE_STRING, &filter, &nitems,
                           DBUS_TYPE_INVALID);
    if (!server->priv->list_children || nitems == 0) {
      children = NULL;
    } else {
      id = get_id_from_message (m);
      if (!id) {
        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
      }
      children = server->priv->list_children (server,
                                              id,
                                              offset,
                                              max_count? max_count: G_MAXUINT,
                                              (const gchar **) filter,
                                              server->priv->data,
                                              NULL);
      g_free (id);
      dbus_free_string_array (filter);
    }

    r = dbus_message_new_method_return (m);
    add_glist_as_array (r, NULL, children);
    dbus_connection_send (c, r, NULL);
    dbus_message_unref (r);
    if (children) {
      g_list_foreach (children, (GFunc) g_hash_table_unref, NULL);
      g_list_free (children);
    }
    return DBUS_HANDLER_RESULT_HANDLED;
  } else {
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
  }
}

/* SearchObjects message handler */
static DBusHandlerResult
handle_search_objects_message (DBusConnection *c,
                               DBusMessage *m,
                               void *userdata)
{
  DBusMessage *r;
  GList *children;
  gchar **filter;
  gchar *id;
  gchar *query;
  guint max_count;
  guint offset;
  gint nitems;
  MS2Server *server = MS2_SERVER (userdata);

  /* Check signature */
  if (dbus_message_has_signature (m, searchobjects_sgn)) {
    dbus_message_get_args (m, NULL,
                           DBUS_TYPE_STRING, &query,
                           DBUS_TYPE_UINT32, &offset,
                           DBUS_TYPE_UINT32, &max_count,
                           DBUS_TYPE_ARRAY, DBUS_TYPE_STRING, &filter, &nitems,
                           DBUS_TYPE_INVALID);
    if (!server->priv->search_objects || nitems == 0) {
      children = NULL;
    } else {
      id = get_id_from_message (m);
      if (!id) {
        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
      }
      children = server->priv->search_objects (server,
                                               id,
                                               query,
                                               offset,
                                               max_count? max_count: G_MAXUINT,
                                               (const gchar **) filter,
                                               server->priv->data,
                                               NULL);
      g_free (id);
      dbus_free_string_array (filter);
    }

    r = dbus_message_new_method_return (m);
    add_glist_as_array (r, NULL, children);
    dbus_connection_send (c, r, NULL);
    dbus_message_unref (r);
    if (children) {
      g_list_foreach (children, (GFunc) g_hash_table_unref, NULL);
      g_list_free (children);
    }
    return DBUS_HANDLER_RESULT_HANDLED;
  } else {
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
  }
}

/* Items interface handler */
static DBusHandlerResult
items_handler (DBusConnection *c,
               DBusMessage *m,
               void *userdata)
{
  if (dbus_message_is_method_call (m,
                                   "org.freedesktop.DBus.Introspectable",
                                   "Introspect")) {
    return handle_introspect_message (c, m, userdata,
                                      ITEM_INTROSPECTION);
  } else if (dbus_message_is_method_call (m,
                                          "org.freedesktop.DBus.Properties",
                                          "Get")) {
    return handle_get_message (c, m, userdata);
  } else if (dbus_message_is_method_call (m,
                                          "org.freedesktop.DBus.Properties",
                                          "GetAll")) {
    return handle_get_all_message (c, m, userdata);
  } else {
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
  }
}

/* Containers interface handler */
static DBusHandlerResult
containers_handler (DBusConnection *c,
                    DBusMessage *m,
                    void *userdata)
{
  if (dbus_message_is_method_call (m,
                                   "org.freedesktop.DBus.Introspectable",
                                   "Introspect")) {
    return handle_introspect_message (c, m, userdata,
                                      CONTAINER_INTROSPECTION);
  } else if (dbus_message_is_method_call (m,
                                          "org.freedesktop.DBus.Properties",
                                          "Get")) {
    return handle_get_message (c, m, userdata);
  } else if (dbus_message_is_method_call (m,
                                          "org.freedesktop.DBus.Properties",
                                          "GetAll")) {
    return handle_get_all_message (c, m, userdata);
  } else if (dbus_message_is_method_call (m,
                                          "org.gnome.UPnP.MediaContainer1",
                                          "ListChildren")) {
    return handle_list_children_message (c, m, userdata);
  } else if (dbus_message_is_method_call (m,
                                          "org.gnome.UPnP.MediaContainer1",
                                          "SearchObjects")) {
    return handle_search_objects_message (c, m, userdata);
  } else {
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
  }
}

/* Root category handler */
static DBusHandlerResult
root_handler (DBusConnection *c,
              DBusMessage *m,
              void *userdata)
{
  return containers_handler (c, m, userdata);
}

/* Registers the MS2Server object in dbus */
static gboolean
ms2_server_dbus_register (MS2Server *server,
                          const gchar *name)
{
  DBusConnection *connection;
  DBusError error;
  gchar *dbus_name;
  gchar *dbus_path;
  gchar *dbus_path_items;
  gchar *dbus_path_containers;
  static const DBusObjectPathVTable vtable_root = {
    .message_function = root_handler,
  };
  static const DBusObjectPathVTable vtable_items = {
    .message_function = items_handler,
  };
  static const DBusObjectPathVTable vtable_containers = {
    .message_function = containers_handler
  };

  dbus_error_init (&error);

  connection = dbus_bus_get (DBUS_BUS_SESSION, &error);
  if (!connection) {
    g_printerr ("Could not connect to session bus, %s\n", error.message);
    return FALSE;
  }

  /* Request name */
  dbus_name = g_strconcat (MS2_DBUS_SERVICE_PREFIX, name, NULL);
  if (dbus_bus_request_name (connection,
                             dbus_name,
                             DBUS_NAME_FLAG_DO_NOT_QUEUE,
                             &error) != DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER) {
    g_printerr ("Failed to request name %s, %s\n", dbus_name, error.message);
    g_free (dbus_name);
    return FALSE;
  }
  g_free (dbus_name);

  /* Register object paths */
  dbus_path = g_strconcat (MS2_DBUS_PATH_PREFIX, name, NULL);
  dbus_path_items = g_strconcat (dbus_path, "/items", NULL);
  dbus_path_containers = g_strconcat (dbus_path, "/containers", NULL);

  dbus_connection_register_object_path (connection, dbus_path, &vtable_root, server);
  dbus_connection_register_fallback (connection, dbus_path_items, &vtable_items, server);
  dbus_connection_register_fallback (connection, dbus_path_containers, &vtable_containers, server);

  g_free (dbus_path);
  g_free (dbus_path_items);
  g_free (dbus_path_containers);

  dbus_connection_setup_with_g_main(connection, NULL);

  return TRUE;
}

/* Unregister MS2Server from dbus */
static void
ms2_server_dbus_unregister (MS2Server *server,
                            const gchar *name)
{
  DBusConnection *connection;
  DBusError error;
  gchar *dbus_name;
  gchar *dbus_path;
  gchar *dbus_path_containers;
  gchar *dbus_path_items;

  dbus_error_init (&error);

  connection = dbus_bus_get (DBUS_BUS_SESSION, &error);
  if (!connection) {
    g_printerr ("Could not connect to session bus, %s\n", error.message);
    return;
  }

  /* Unregister object paths */
  dbus_path = g_strconcat (MS2_DBUS_PATH_PREFIX, server->priv->name, NULL);
  dbus_path_items = g_strconcat (dbus_path, "/items", NULL);
  dbus_path_containers = g_strconcat (dbus_path, "/containers", NULL);
  dbus_connection_unregister_object_path (connection, dbus_path);
  dbus_connection_unregister_object_path (connection, dbus_path_items);
  dbus_connection_unregister_object_path (connection, dbus_path_containers);
  g_free (dbus_path);
  g_free (dbus_path_items);
  g_free (dbus_path_containers);

  /* Release name */
  dbus_name = g_strconcat (MS2_DBUS_SERVICE_PREFIX, server->priv->name, NULL);
  dbus_bus_release_name (connection,
                         dbus_name,
                         NULL);

  g_free (dbus_name);
}

/* Finalize function */
static void
ms2_server_finalize (GObject *object)
{
  MS2Server *server = MS2_SERVER (object);

  ms2_server_dbus_unregister (server, server->priv->name);
  g_free (server->priv->name);

  G_OBJECT_CLASS (ms2_server_parent_class)->finalize (object);
}

/* Class init function */
static void
ms2_server_class_init (MS2ServerClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MS2ServerPrivate));

  gobject_class->finalize = ms2_server_finalize;
}

/* Object init function */
static void
ms2_server_init (MS2Server *server)
{
  server->priv = MS2_SERVER_GET_PRIVATE (server);
}

/********************* PUBLIC API *********************/

/**
 * ms2_server_new:
 * @name: the name used when registered in DBus
 * @data: user defined data
 *
 * Creates a new #MS2Server that will be registered in DBus under name
 * "org.gnome.UPnP.MediaServer2.<name>".
 *
 * @data will be used as parameter when invoking the functions to retrieve
 * properties or children.
 *
 * Returns: a new #MS2Server registed in DBus, or @NULL if fails
 **/
MS2Server *
ms2_server_new (const gchar *name,
                gpointer data)
{
  MS2Server *server;

  g_return_val_if_fail (name, NULL);

  server = g_object_new (MS2_TYPE_SERVER, NULL);

  server->priv->data = data;
  server->priv->name = g_strdup (name);

  /* Register object in DBus */
  if (!ms2_server_dbus_register (server, name)) {
    g_object_unref (server);
    return NULL;
  } else {
    return server;
  }
}

/**
 * ms2_server_set_get_properties_func:
 * @server: a #MS2Server
 * @get_properties_func: user-defined function to request properties
 *
 * Defines which function must be used when requesting properties.
 **/
void
ms2_server_set_get_properties_func (MS2Server *server,
                                    GetPropertiesFunc get_properties_func)
{
  g_return_if_fail (MS2_IS_SERVER (server));

  server->priv->get_properties = get_properties_func;
}

/**
 * ms2_server_set_list_children_func:
 * @server: a #MS2Server
 * @list_children_func: user-defined function to request children
 *
 * Defines which function must be used when requesting children.
 **/
void
ms2_server_set_list_children_func (MS2Server *server,
                                   ListChildrenFunc list_children_func)
{
  g_return_if_fail (MS2_IS_SERVER (server));

  server->priv->list_children = list_children_func;
}

/**
 * ms2_server_set_search_objects_func:
 * @server: a #MS2Server
 * @search_objects_func: user-defined function to search children
 *
 * Defines which function must be used when searching objects
 **/
void
ms2_server_set_search_objects_func (MS2Server *server,
                                    SearchObjectsFunc search_objects_func)
{
  g_return_if_fail (MS2_IS_SERVER (server));

  server->priv->search_objects = search_objects_func;
}

/**
 * ms2_server_updated:
 * @server: a #MS2Server
 * @id: item identifier that has changed
 *
 * Emit a signal notifying an item has changed.
 *
 * Which shall be triggered when a new child item is created or removed from
 * container, or one of the existing child items is modified, or any of the
 * properties of the container itself are modified. While the signal should be
 * emitted when child containers are created or removed, it shall not be emitted
 * when child containers are modified: instead the signal should be emitted on
 * the child in this case. It up to client to follow these rules when invoking
 * this method
 **/
void
ms2_server_updated (MS2Server *server,
                    const gchar *id)
{
  DBusError error;
  DBusConnection *connection;
  DBusMessage *message;
  gchar *object_path;

  g_return_if_fail (MS2_IS_SERVER (server));

  dbus_error_init (&error);

  connection = dbus_bus_get (DBUS_BUS_SESSION, &error);

  if (!connection) {
    g_printerr ("Could not connect to session bus, %s\n", error.message);
    return;
  }

  /* Get object path */
  if (g_strcmp0 (id, MS2_ROOT) == 0) {
    object_path = g_strconcat (MS2_DBUS_PATH_PREFIX,
                               server->priv->name,
                               NULL);
  } else {
    object_path = g_strdup_printf (MS2_DBUS_PATH_PREFIX "%s/containers/%d",
                                   server->priv->name,
                                   g_quark_from_string (id));
  }

  message = dbus_message_new_signal (object_path,
                                     "org.gnome.UPnP.MediaContainer1",
                                     "Updated");
  dbus_connection_send (connection, message, NULL);
  dbus_message_unref (message);

  g_free (object_path);
}

/**
 * ms2_server_get_name:
 * @server: a #MS2Server
 *
 * Returns name used for this server.
 *
 * Returns: server name. Should not be freed.
 **/
const gchar *
ms2_server_get_name (MS2Server *server)
{
  g_return_val_if_fail (MS2_IS_SERVER (server), NULL);

  return server->priv->name;
}
