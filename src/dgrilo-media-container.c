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

#include <dbus/dbus-glib.h>
#include "dgrilo-media-container.h"
#include "dgrilo-media-container-glue.h"
#include "dgrilo-media-item.h"

#define MAX_RESULTS 50

#define DGRILO_PATH "/org/gnome/UPnP/MediaServer1/DGrilo"

#define DBUS_TYPE_G_ARRAY_OF_OBJECT_PATH                                \
  (dbus_g_type_get_collection ("GPtrArray", DBUS_TYPE_G_OBJECT_PATH))

#define DGRILO_MEDIA_CONTAINER_GET_PRIVATE(o)                           \
  G_TYPE_INSTANCE_GET_PRIVATE((o), DGRILO_MEDIA_CONTAINER_TYPE, DGriloMediaContainerPrivate)

typedef void (*RetryCb) (DGriloMediaContainer *obj, gchar *property, DBusGMethodInvocation *context);

enum {
  PROP_0,
  PROP_ITEMS,
  PROP_ITEM_COUNT,
  PROP_CONTAINERS,
  PROP_CONTAINER_COUNT,
  LAST_PROP
};

typedef struct {
  DGriloMediaContainer *container;
  DBusGMethodInvocation *context;
  RetryCb retry;
  gchar *item;
} BrowseData;

typedef struct {
  guint item_count;
  guint container_count;
  GList *items;
  GList *containers;
  gboolean browsed;
} DGriloMediaContainerPrivate;

G_DEFINE_TYPE (DGriloMediaContainer, dgrilo_media_container, DGRILO_MEDIA_OBJECT_TYPE);

static void
retry_get (DGriloMediaContainer *obj,
           gchar *property,
           DBusGMethodInvocation *context)
{
  dgrilo_media_container_get (obj, NULL, property, context, NULL);
}

#if 0
static void
retry_get_all (DGriloMediaContainer *obj,
               gchar *interface,
               DBusGMethodInvocation *context)
{
  dgrilo_media_container_get_all (obj, interface, context, NULL);
}
#endif

static void
browse_result_cb (GrlMediaSource *source,
                  guint browse_id,
                  GrlMedia *media,
                  guint remaining,
                  gpointer user_data,
                  const GError *error)
{
  BrowseData *bd = (BrowseData *) user_data;
  DGriloMediaContainerPrivate *priv;
  DGriloMediaContainer *container;
  DGriloMediaItem *item;

  g_assert (!error);

  priv = DGRILO_MEDIA_CONTAINER_GET_PRIVATE (bd->container);
  if (media) {
    if (GRL_IS_MEDIA_BOX (media)) {
      container =
        dgrilo_media_container_new_with_parent (DGRILO_MEDIA_OBJECT (bd->container),
                                                media);
      priv->containers =
        g_list_prepend (priv->containers,
                        g_strdup (dgrilo_media_object_get_dbus_path (DGRILO_MEDIA_OBJECT (container))));
      priv->container_count++;
    } else {
      item =
        dgrilo_media_item_new_with_parent (DGRILO_MEDIA_OBJECT (bd->container),
                                           media);
      priv->items =
        g_list_prepend (priv->items,
                        g_strdup (dgrilo_media_object_get_dbus_path (DGRILO_MEDIA_OBJECT (item))));
      priv->item_count++;
    }
  }

  if (!remaining) {
    /* Send result */
    priv->browsed = TRUE;
    priv->containers = g_list_reverse (priv->containers);
    priv->items = g_list_reverse (priv->items);
    bd->retry (bd->container, bd->item, bd->context);
    g_free (bd->item);
    g_free (bd);
  }
}

static void
browse_grilo_media (BrowseData *bd)
{
  GList *keys;
  GrlMedia *media;
  GrlMediaSource *source;
  GrlPluginRegistry *registry;

  /* Get the source */
  g_object_get (bd->container,
                "grl-media", &media,
                NULL);

  registry = grl_plugin_registry_get_instance ();
  source =
    (GrlMediaSource *) grl_plugin_registry_lookup_source (registry,
                                                          grl_media_get_source (media));

  keys = grl_metadata_key_list_new (GRL_METADATA_KEY_TITLE,
                                    NULL);

  grl_media_source_browse (source,
                           media,
                           keys,
                           0,
                           MAX_RESULTS,
                           GRL_RESOLVE_FAST_ONLY,
                           browse_result_cb,
                           bd);
}

static GPtrArray *
dgrilo_media_container_get_elements (GList *elements)
{
  GList *p;
  GPtrArray *pelements = NULL;
  gint size;

  size = g_list_length (elements);
  pelements = g_ptr_array_sized_new (size);
  for (p = elements; p; p = g_list_next (p)) {
    g_ptr_array_add (pelements, g_strdup (p->data));
  }

  return pelements;
}

static GPtrArray *
dgrilo_media_container_get_items (DGriloMediaContainer *obj)
{
  return dgrilo_media_container_get_elements (DGRILO_MEDIA_CONTAINER_GET_PRIVATE (obj)->items);
}

static GPtrArray *
dgrilo_media_container_get_containers (DGriloMediaContainer *obj)
{
  return dgrilo_media_container_get_elements (DGRILO_MEDIA_CONTAINER_GET_PRIVATE (obj)->containers);
}

static void
dgrilo_media_container_get_property (GObject *object,
                                     guint prop_id,
                                     GValue *value,
                                     GParamSpec *pspec)
{
  DGriloMediaContainer *self = DGRILO_MEDIA_CONTAINER (object);
  DGriloMediaContainerPrivate *priv = DGRILO_MEDIA_CONTAINER_GET_PRIVATE (self);

  switch (prop_id) {
  case PROP_ITEM_COUNT:
    g_value_set_uint (value, priv->item_count);
    break;
  case PROP_CONTAINER_COUNT:
    g_value_set_uint (value, priv->container_count);
    break;
  case PROP_ITEMS:
    g_value_take_boxed (value,
                        dgrilo_media_container_get_items (self));
    break;
  case PROP_CONTAINERS:
    g_value_take_boxed (value,
                        dgrilo_media_container_get_containers (self));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    break;
  }
}

static void
dgrilo_media_container_set_property (GObject *object,
                                     guint prop_id,
                                     const GValue *value,
                                     GParamSpec *pspec)
{
  DGriloMediaContainer *self = DGRILO_MEDIA_CONTAINER (object);
  DGriloMediaContainerPrivate *priv = DGRILO_MEDIA_CONTAINER_GET_PRIVATE (self);

  switch (prop_id) {
  case PROP_ITEM_COUNT:
    priv->item_count = g_value_get_uint (value);
    break;
  case PROP_CONTAINER_COUNT:
    priv->container_count = g_value_get_uint (value);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    break;
  }
}

static void
dgrilo_media_container_dispose (GObject *object)
{
  G_OBJECT_CLASS (dgrilo_media_container_parent_class)->dispose (object);
}

gboolean
dgrilo_media_container_get (DGriloMediaContainer *obj,
                            const gchar *interface,
                            const gchar *property,
                            DBusGMethodInvocation *context,
                            GError **error)
{
  BrowseData *bd;
  DGriloMediaContainerPrivate *priv = DGRILO_MEDIA_CONTAINER_GET_PRIVATE (obj);
  GValue val = { 0 };
  gchar *display_name = NULL;
  gchar *parent_path = NULL;
  guint container_count;
  guint item_count;

  if (g_strcmp0 (property, "DisplayName") == 0) {
    g_object_get (obj, "display-name", &display_name, NULL);
    g_value_init (&val, G_TYPE_STRING);
    g_value_take_string (&val, display_name);
  } else if (g_strcmp0 (property, "Parent") == 0) {
    g_object_get (obj, "parent", &parent_path, NULL);
    g_value_init (&val, G_TYPE_STRING);
    g_value_take_string (&val, parent_path);
  } else if (g_strcmp0 (property, "ContainerCount") == 0) {
    if (priv->browsed) {
      g_object_get (obj, "container-count", &container_count, NULL);
      g_value_init (&val, G_TYPE_UINT);
      g_value_set_uint (&val, container_count);
    } else {
      bd = g_new0 (BrowseData, 1);
      bd->container = obj;
      bd->context = context;
      bd->retry = retry_get;
      bd->item = g_strdup ("ContainerCount");
      browse_grilo_media (bd);
      return TRUE;
    }
  } else if (g_strcmp0 (property, "ItemCount") == 0) {
    if (priv->browsed) {
      g_object_get (obj, "item-count", &item_count, NULL);
      g_value_init (&val, G_TYPE_UINT);
      g_value_set_uint (&val, item_count);
    } else {
      bd = g_new0 (BrowseData, 1);
      bd->container = obj;
      bd->context = context;
      bd->retry = retry_get;
      bd->item = g_strdup ("ItemCount");
      browse_grilo_media (bd);
      return TRUE;
    }
  } else if (g_strcmp0 (property, "Items") == 0) {
    if (priv->browsed) {
      g_value_init (&val, DBUS_TYPE_G_ARRAY_OF_OBJECT_PATH);
      g_value_take_boxed (&val,
                          dgrilo_media_container_get_items (obj));
    } else {
      bd = g_new0 (BrowseData, 1);
      bd->container = obj;
      bd->context = context;
      bd->retry = retry_get;
      bd->item = g_strdup ("Items");
      browse_grilo_media (bd);
      return TRUE;
    }
  } else if (g_strcmp0 (property, "Containers") == 0) {
    if (priv->browsed) {
      g_value_init (&val, DBUS_TYPE_G_ARRAY_OF_OBJECT_PATH);
      g_value_take_boxed (&val,
                          dgrilo_media_container_get_containers (obj));
    } else {
      bd = g_new0 (BrowseData, 1);
      bd->container = obj;
      bd->context = context;
      bd->retry = retry_get;
      bd->item = g_strdup ("Containers");
      browse_grilo_media (bd);
      return TRUE;
    }
  } else {
    return FALSE;
  }

  dbus_g_method_return (context, &val);
  g_value_unset (&val);

  return TRUE;
}

gboolean
dgrilo_media_container_get_all (DGriloMediaContainer *obj,
                                const gchar *interface,
                                DBusGMethodInvocation *context,
                                GError **error)
{
  gchar *parent_path;
  gchar *display_name;
  guint item_count;
  guint container_count;
  GValue parent_value = { 0 };
  GValue display_value = { 0 };
  GValue itemc_value = { 0 };
  GValue containerc_value = { 0 };
  GHashTable *map;

  map = g_hash_table_new_full (g_str_hash,
                               g_str_equal,
                               NULL,
                               (GDestroyNotify) g_value_unset);

  if (g_strcmp0 (interface, "org.gnome.UPnP.MediaObject1") == 0) {
    g_object_get (obj,
                  "parent", &parent_path,
                  "display-name", &display_name,
                  NULL);
    g_value_init (&parent_value, G_TYPE_STRING);
    g_value_init (&display_value, G_TYPE_STRING);
    g_value_take_string (&parent_value, parent_path);
    g_value_take_string (&display_value, display_name);

    g_hash_table_insert (map, "Parent", &parent_value);
    g_hash_table_insert (map, "DisplayName", &display_value);
  } else if (g_strcmp0 (interface, "org.gnome.UPnP.MediaContainer1") == 0) {
    g_object_get (obj,
                  "item-count", &item_count,
                  "container-count", &container_count,
                  NULL);
    g_value_init (&itemc_value, G_TYPE_UINT);
    g_value_init (&containerc_value, G_TYPE_UINT);
    g_value_set_uint (&itemc_value, item_count);
    g_value_set_uint (&containerc_value, container_count);

    g_hash_table_insert (map, "ItemCount", &itemc_value);
    g_hash_table_insert (map, "ContainerCount", &containerc_value);
  } else {
    g_hash_table_unref (map);
    return FALSE;
  }

  dbus_g_method_return (context, map);
  g_hash_table_unref (map);

  return TRUE;
}


static void
dgrilo_media_container_class_init (DGriloMediaContainerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (DGriloMediaContainerPrivate));

  object_class->get_property = dgrilo_media_container_get_property;
  object_class->set_property = dgrilo_media_container_set_property;
  object_class->dispose = dgrilo_media_container_dispose;

  g_object_class_install_property (object_class,
                                   PROP_ITEMS,
                                   g_param_spec_boxed ("items",
                                                       "Items",
                                                       "List of Items",
                                                       DBUS_TYPE_G_ARRAY_OF_OBJECT_PATH,
                                                       G_PARAM_READABLE));

  g_object_class_install_property (object_class,
                                   PROP_ITEM_COUNT,
                                   g_param_spec_uint ("item-count",
                                                      "ItemCount",
                                                      "Number of Items",
                                                      0, G_MAXUINT, 0,
                                                      G_PARAM_READABLE | G_PARAM_WRITABLE));

  g_object_class_install_property (object_class,
                                   PROP_CONTAINERS,
                                   g_param_spec_boxed ("containers",
                                                       "Containers",
                                                       "List of Containers",
                                                       DBUS_TYPE_G_ARRAY_OF_OBJECT_PATH,
                                                       G_PARAM_READABLE));

  g_object_class_install_property (object_class,
                                   PROP_CONTAINER_COUNT,
                                   g_param_spec_uint ("container-count",
                                                      "ContainerCount",
                                                      "Number of Containers",
                                                      0, G_MAXUINT, 0,
                                                      G_PARAM_READABLE | G_PARAM_WRITABLE));

  /* Register introspection */
  dbus_g_object_type_install_info (DGRILO_MEDIA_CONTAINER_TYPE,
                                   &dbus_glib_dgrilo_media_container_object_info);
}

static void
dgrilo_media_container_init (DGriloMediaContainer *server)
{
}

DGriloMediaContainer *
dgrilo_media_container_new_with_dbus_path (const gchar *dbus_path,
                                           GrlMedia *media)
{
  DGriloMediaContainer *obj;

  obj = g_object_new (DGRILO_MEDIA_CONTAINER_TYPE,
                      "parent", dbus_path,
                      "display-name", media? grl_media_get_title (media): "Unknown",
                      "dbus-path", dbus_path,
                      "grl-media", media,
                      NULL);

  dgrilo_media_object_dbus_register (DGRILO_MEDIA_OBJECT (obj));

  return obj;
}

DGriloMediaContainer *
dgrilo_media_container_new_with_parent (DGriloMediaObject *parent,
                                        GrlMedia *media)
{
  DGriloMediaContainer *obj;
  DGriloMediaObjectClass *klass;
  gchar *dbus_path;

  klass = DGRILO_MEDIA_OBJECT_GET_CLASS (parent);
  dbus_path = g_strdup_printf ("%s/%u",
                               dgrilo_media_object_get_dbus_path (parent),
                               klass->index);

  obj = g_object_new (DGRILO_MEDIA_CONTAINER_TYPE,
                      "parent", dgrilo_media_object_get_dbus_path (parent),
                      "display-name", media? grl_media_get_title (media): "Unknown",
                      "dbus-path", dbus_path,
                      "grl-media", media,
                      "parent-media", parent,
                      NULL);

  g_free (dbus_path);
  klass->index++;
  dgrilo_media_object_dbus_register (DGRILO_MEDIA_OBJECT (obj));

  return obj;
}
