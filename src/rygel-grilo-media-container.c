/* (C) 2010 Igalia S.L.
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
#include "rygel-grilo-media-container.h"
#include "rygel-grilo-media-container-glue.h"
#include "rygel-grilo-media-item.h"

#define DEFAULT_LIMIT 50

#define DBUS_TYPE_G_ARRAY_OF_OBJECT_PATH                                \
  (dbus_g_type_get_collection ("GPtrArray", DBUS_TYPE_G_OBJECT_PATH))

#define RYGEL_GRILO_MEDIA_CONTAINER_GET_PRIVATE(o)                      \
  G_TYPE_INSTANCE_GET_PRIVATE((o), RYGEL_GRILO_MEDIA_CONTAINER_TYPE, RygelGriloMediaContainerPrivate)

typedef void (*RetryCb) (RygelGriloMediaContainer *obj,
                         gchar *property,
                         DBusGMethodInvocation *context);

enum {
  PROP_0,
  PROP_ITEMS,
  PROP_ITEM_COUNT,
  PROP_CONTAINERS,
  PROP_CONTAINER_COUNT,
  LAST_PROP
};

typedef struct {
  RygelGriloMediaContainer *container;
  DBusGMethodInvocation *context;
  RetryCb retry;
  gchar *item;
} BrowseData;

/*
 * Private RygelGriloMediaCointainer structure
 *   item_count: number of children items of this container
 *   container_count: number of children containers of this container
 *   items: list of RygelGriloMediaItem children
 *   containers: list of RygelGriloMediaContainer children
 *   browsed: @TRUE if previous fields have right values
 */
typedef struct {
  guint item_count;
  guint container_count;
  GList *items;
  GList *containers;
  gboolean browsed;
} RygelGriloMediaContainerPrivate;

G_DEFINE_TYPE (RygelGriloMediaContainer, rygel_grilo_media_container, RYGEL_GRILO_MEDIA_OBJECT_TYPE);

/* Frees a RygelGrilo object registered in dbus_path */
static void
free_child (gchar *dbus_path)
{
  GError *error = NULL;
  static DBusGConnection *connection = NULL;

  if (!connection) {
    connection = dbus_g_bus_get (DBUS_BUS_SESSION, &error);
    if (error) {
      g_warning ("Unable to get dbus connection, %s", error->message);
      g_error_free (error);
    }
  }

  if (connection) {
    g_object_unref (dbus_g_connection_lookup_g_object (connection,
                                                       dbus_path));
  }

  g_free (dbus_path);
}

/* Reinvokes rygel_grilo_media_container_get() function. Used when a browse has
   finished */
static void
retry_get (RygelGriloMediaContainer *obj,
           gchar *property,
           DBusGMethodInvocation *context)
{
  rygel_grilo_media_container_get (obj, NULL, property, context, NULL);
}

/* Reinvokes rygel_grilo_media_container_get_all() function. Used when a browse
   has finished */
static void
retry_get_all (RygelGriloMediaContainer *obj,
               gchar *interface,
               DBusGMethodInvocation *context)
{
  rygel_grilo_media_container_get_all (obj, interface, context, NULL);
}

/* Callback with the results of a grilo browse() operation */
static void
browse_result_cb (GrlMediaSource *source,
                  guint browse_id,
                  GrlMedia *media,
                  guint remaining,
                  gpointer user_data,
                  const GError *error)
{
  BrowseData *bd = (BrowseData *) user_data;
  RygelGriloMediaContainerPrivate *priv;
  RygelGriloMediaContainer *container;
  RygelGriloMediaItem *item;

  g_assert (!error);

  priv = RYGEL_GRILO_MEDIA_CONTAINER_GET_PRIVATE (bd->container);
  if (media) {
    if (GRL_IS_MEDIA_BOX (media)) {
      container =
        rygel_grilo_media_container_new_with_parent (RYGEL_GRILO_MEDIA_OBJECT (bd->container),
                                                     media);
      priv->containers =
        g_list_prepend (priv->containers,
                        g_strdup (rygel_grilo_media_object_get_dbus_path (RYGEL_GRILO_MEDIA_OBJECT (container))));
      priv->container_count++;
    } else {
      item =
        rygel_grilo_media_item_new (RYGEL_GRILO_MEDIA_OBJECT (bd->container),
                                    media);
      priv->items =
        g_list_prepend (priv->items,
                        g_strdup (rygel_grilo_media_object_get_dbus_path (RYGEL_GRILO_MEDIA_OBJECT (item))));
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

/* Performs a browse in from the grilo media that is wrapped */
static void
browse_grilo_media (BrowseData *bd)
{
  static GList *keys = NULL;
  GrlMedia *media;
  GrlMediaSource *source;
  GrlPluginRegistry *registry;
  RygelGriloMediaContainerClass *klass =
    RYGEL_GRILO_MEDIA_CONTAINER_GET_CLASS (bd->container);

  /* Get the source */
  g_object_get (bd->container,
                "grl-media", &media,
                NULL);

  registry = grl_plugin_registry_get_instance ();
  source =
    (GrlMediaSource *) grl_plugin_registry_lookup_source (registry,
                                                          grl_media_get_source (media));

  if (!keys) {
    keys = grl_metadata_key_list_new (GRL_METADATA_KEY_ALBUM,
                                      GRL_METADATA_KEY_ARTIST,
                                      GRL_METADATA_KEY_BITRATE,
                                      GRL_METADATA_KEY_DURATION,
                                      GRL_METADATA_KEY_GENRE,
                                      GRL_METADATA_KEY_HEIGHT,
                                      GRL_METADATA_KEY_MIME,
                                      GRL_METADATA_KEY_TITLE,
                                      GRL_METADATA_KEY_URL,
                                      GRL_METADATA_KEY_WIDTH,
                                      NULL);
  }

  grl_media_source_browse (source,
                           media,
                           keys,
                           0,
                           klass->limit,
                           GRL_RESOLVE_FULL | GRL_RESOLVE_IDLE_RELAY,
                           browse_result_cb,
                           bd);
}

/* Convert a GList in a GPtrArray */
static GPtrArray *
rygel_grilo_media_container_get_elements (GList *elements)
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

/* Returns an array with the RygelGriloMediaItem children */
static GPtrArray *
rygel_grilo_media_container_get_items (RygelGriloMediaContainer *obj)
{
  return rygel_grilo_media_container_get_elements (RYGEL_GRILO_MEDIA_CONTAINER_GET_PRIVATE (obj)->items);
}

/* Returns an array with the RygelGriloMediaContainer children */
static GPtrArray *
rygel_grilo_media_container_get_containers (RygelGriloMediaContainer *obj)
{
  return rygel_grilo_media_container_get_elements (RYGEL_GRILO_MEDIA_CONTAINER_GET_PRIVATE (obj)->containers);
}

/* Gets a property value */
static void
rygel_grilo_media_container_get_property (GObject *object,
                                          guint prop_id,
                                          GValue *value,
                                          GParamSpec *pspec)
{
  RygelGriloMediaContainer *self =
    RYGEL_GRILO_MEDIA_CONTAINER (object);
  RygelGriloMediaContainerPrivate *priv =
    RYGEL_GRILO_MEDIA_CONTAINER_GET_PRIVATE (self);

  switch (prop_id) {
  case PROP_ITEM_COUNT:
    g_value_set_uint (value, priv->item_count);
    break;
  case PROP_CONTAINER_COUNT:
    g_value_set_uint (value, priv->container_count);
    break;
  case PROP_ITEMS:
    g_value_take_boxed (value,
                        rygel_grilo_media_container_get_items (self));
    break;
  case PROP_CONTAINERS:
    g_value_take_boxed (value,
                        rygel_grilo_media_container_get_containers (self));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    break;
  }
}

/* Sets a property */
static void
rygel_grilo_media_container_set_property (GObject *object,
                                          guint prop_id,
                                          const GValue *value,
                                          GParamSpec *pspec)
{
  RygelGriloMediaContainer *self =
    RYGEL_GRILO_MEDIA_CONTAINER (object);
  RygelGriloMediaContainerPrivate *priv =
    RYGEL_GRILO_MEDIA_CONTAINER_GET_PRIVATE (self);

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

/* Dispose a RygelGriloMediaContainer, freeing also the children */
static void
rygel_grilo_media_container_dispose (GObject *object)
{
  RygelGriloMediaContainer *self =
    RYGEL_GRILO_MEDIA_CONTAINER (object);
  RygelGriloMediaContainerPrivate *priv =
    RYGEL_GRILO_MEDIA_CONTAINER_GET_PRIVATE (self);

  /* Free children */
  g_list_foreach (priv->containers, (GFunc) free_child, NULL);
  g_list_free (priv->containers);

  g_list_foreach (priv->items, (GFunc) free_child, NULL);
  g_list_free (priv->items);

  G_OBJECT_CLASS (rygel_grilo_media_container_parent_class)->dispose (object);
}

/* Class init function */
static void
rygel_grilo_media_container_class_init (RygelGriloMediaContainerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (RygelGriloMediaContainerPrivate));

  object_class->get_property = rygel_grilo_media_container_get_property;
  object_class->set_property = rygel_grilo_media_container_set_property;
  object_class->dispose = rygel_grilo_media_container_dispose;

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
                                                      G_PARAM_READABLE |
                                                      G_PARAM_WRITABLE));

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
                                                      G_PARAM_READABLE |
                                                      G_PARAM_WRITABLE));

  klass->limit = DEFAULT_LIMIT;

  /* Register introspection */
  dbus_g_object_type_install_info (RYGEL_GRILO_MEDIA_CONTAINER_TYPE,
                                   &dbus_glib_rygel_grilo_media_container_object_info);
}

/* Object init function */
static void
rygel_grilo_media_container_init (RygelGriloMediaContainer *server)
{
}

/**
 * rygel_grilo_media_container_get:
 * @obj: a RygelGriloMediaContainer
 * @interface: dbus interface used to query the object
 * @property: property queried
 * @context: dbus method context to send the reply
 * @error: error if something is wrong
 *
 * Send the value of a property through dbus. If property refers to some
 * children, the current container is browsed previously to get children, an
 * then the function is invoked again to send the values.
 *
 * Returns: @TRUE if the property can be sent
 **/
gboolean
rygel_grilo_media_container_get (RygelGriloMediaContainer *obj,
                                 const gchar *interface,
                                 const gchar *property,
                                 DBusGMethodInvocation *context,
                                 GError **error)
{
  BrowseData *bd;
  RygelGriloMediaContainerPrivate *priv =
    RYGEL_GRILO_MEDIA_CONTAINER_GET_PRIVATE (obj);
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
                          rygel_grilo_media_container_get_items (obj));
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
                          rygel_grilo_media_container_get_containers (obj));
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

/**
 * rygel_grilo_media_container_get_all:
 * @obj: a RygelGriloMediacontainer
 * @interface: dbus interface used to query the object
 * @context: dbus method context to send the reply
 * @error: error if something is wrong
 *
 * Send the value of all properties in the interface. If interface refers to
 * "org.gnome.UPnP.MediaObject then a browse is performed to get the children,
 * and then values are returned.
 *
 * Returns: @TRUE if property can be sent
 **/
gboolean
rygel_grilo_media_container_get_all (RygelGriloMediaContainer *obj,
                                     const gchar *interface,
                                     DBusGMethodInvocation *context,
                                     GError **error)
{
  BrowseData *bd;
  RygelGriloMediaContainerPrivate *priv =
    RYGEL_GRILO_MEDIA_CONTAINER_GET_PRIVATE (obj);
  GHashTable *map;
  GPtrArray *containers;
  GPtrArray *items;
  GValue containerc_value = { 0 };
  GValue containers_value = { 0 };
  GValue display_value = { 0 };
  GValue itemc_value = { 0 };
  GValue items_value = { 0 };
  GValue parent_value = { 0 };
  gchar *display_name;
  gchar *parent_path;
  guint container_count;
  guint item_count;

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
    if (priv->browsed) {
      g_object_get (obj,
                    "item-count", &item_count,
                    "items", &items,
                    "container-count", &container_count,
                    "containers", &containers,
                    NULL);
      g_value_init (&itemc_value, G_TYPE_UINT);
      g_value_init (&items_value, DBUS_TYPE_G_ARRAY_OF_OBJECT_PATH);
      g_value_init (&containerc_value, G_TYPE_UINT);
      g_value_init (&containers_value, DBUS_TYPE_G_ARRAY_OF_OBJECT_PATH);

      g_value_set_uint (&itemc_value, item_count);
      g_value_take_boxed (&items_value, items);
      g_value_set_uint (&containerc_value, container_count);
      g_value_take_boxed (&containers_value, containers);

      g_hash_table_insert (map, "ItemCount", &itemc_value);
      g_hash_table_insert (map, "Items", &items_value);
      g_hash_table_insert (map, "ContainerCount", &containerc_value);
      g_hash_table_insert (map, "Containers", &containers_value);
    } else {
      bd = g_new0 (BrowseData, 1);
      bd->container = obj;
      bd->context = context;
      bd->retry = retry_get_all;
      bd->item = g_strdup ("org.gnome.UPnP.MediaContainer1");
      browse_grilo_media (bd);
      return TRUE;
    }
  } else {
    g_hash_table_unref (map);
    return FALSE;
  }

  dbus_g_method_return (context, map);
  g_hash_table_unref (map);

  return TRUE;
}

/**
 * rygel_grilo_media_container_new_root:
 * @dbus_path: dbus path where object is registered
 * @media: grilo media object being wrapped
 * @limit: up to how many children each container will have
 *
 * Creates a new container that will be the root of all containers. Usually it
 * wraps the root of a grilo source.
 *
 * Returns: a new RygelGriloMediaContainer
 **/
RygelGriloMediaContainer *
rygel_grilo_media_container_new_root (const gchar *dbus_path,
                                      GrlMedia *media,
                                      gint limit)
{
  RygelGriloMediaContainer *obj;
  RygelGriloMediaContainerClass *klass;

  obj = g_object_new (RYGEL_GRILO_MEDIA_CONTAINER_TYPE,
                      "parent", dbus_path,
                      "dbus-path", dbus_path,
                      "grl-media", media,
                      NULL);
  klass = RYGEL_GRILO_MEDIA_CONTAINER_GET_CLASS (obj);
  klass->limit = limit;
  rygel_grilo_media_object_dbus_register (RYGEL_GRILO_MEDIA_OBJECT (obj));

  return obj;
}


/**
 * rygel_grilo_media_container_new_with_parent:
 * @parent: parent object
 * @media: grilo media object to be wrapped
 *
 * Creates a new container that is child of another container.
 *
 * Returns: a new RygelGriloMediaContainer
 **/
RygelGriloMediaContainer *
rygel_grilo_media_container_new_with_parent (RygelGriloMediaObject *parent,
                                             GrlMedia *media)
{
  RygelGriloMediaContainer *obj;
  RygelGriloMediaObjectClass *klass;
  gchar *dbus_path;

  klass = RYGEL_GRILO_MEDIA_OBJECT_GET_CLASS (parent);
  dbus_path = g_strdup_printf ("%s/%u",
                               rygel_grilo_media_object_get_dbus_path (parent),
                               klass->index);

  obj =
    g_object_new (RYGEL_GRILO_MEDIA_CONTAINER_TYPE,
                  "parent", rygel_grilo_media_object_get_dbus_path (parent),
                  "dbus-path", dbus_path,
                  "grl-media", media,
                  "parent-media", parent,
                  NULL);

  g_free (dbus_path);
  klass->index++;
  rygel_grilo_media_object_dbus_register (RYGEL_GRILO_MEDIA_OBJECT (obj));

  return obj;
}
