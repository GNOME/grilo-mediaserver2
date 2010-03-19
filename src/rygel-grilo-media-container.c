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
#include <string.h>
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

typedef enum {
  NOT_DONE = 0,
  IN_PROGRESS,
  DONE
} BrowseStatus;

/*
 * Private RygelGriloMediaCointainer structure
 *   item_count: number of children items of this container
 *   container_count: number of children containers of this container
 *   items: list of RygelGriloMediaItem children
 *   containers: list of RygelGriloMediaContainer children
 *   browse_status: status of browse operation
 */
struct _RygelGriloMediaContainerPrivate {
  guint item_count;
  guint container_count;
  GList *items;
  GList *containers;
  BrowseStatus browse_status;
};

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

/* Callback with the results of a grilo browse() operation */
static void
browse_result_cb (GrlMediaSource *source,
                  guint browse_id,
                  GrlMedia *media,
                  guint remaining,
                  gpointer user_data,
                  const GError *error)
{
  RygelGriloMediaContainer *container =
    (RygelGriloMediaContainer *) user_data;
  RygelGriloMediaContainer *child_container;
  RygelGriloMediaItem *child_item;

  if (media) {
    if (GRL_IS_MEDIA_BOX (media)) {
      child_container =
        rygel_grilo_media_container_new_with_parent (RYGEL_GRILO_MEDIA_OBJECT (container),
                                                     media);
      if (child_container) {
        container->priv->containers =
          g_list_prepend (container->priv->containers,
                          g_strdup (rygel_grilo_media_object_get_dbus_path (RYGEL_GRILO_MEDIA_OBJECT (child_container))));
        container->priv->container_count++;
      }
    } else {
      child_item =
        rygel_grilo_media_item_new (RYGEL_GRILO_MEDIA_OBJECT (container),
                                    media);
      if (child_item) {
        container->priv->items =
          g_list_prepend (container->priv->items,
                          g_strdup (rygel_grilo_media_object_get_dbus_path (RYGEL_GRILO_MEDIA_OBJECT (child_item))));
        container->priv->item_count++;
      }
    }
  }

  /* Check if browse has finished */
  if (!remaining) {
    container->priv->containers = g_list_reverse (container->priv->containers);
    container->priv->items = g_list_reverse (container->priv->items);
    container->priv->browse_status = DONE;
  }
}

/*  Synchronous function that wrappes a grilo browse; when browse finish, it
    sets the right properties and returns */
static void
browse_grilo_media (RygelGriloMediaContainer *container)
{
  GMainContext *mainloop_context;
  GMainLoop *mainloop;
  GrlMedia *media;
  GrlMediaSource *source;
  GrlPluginRegistry *registry;
  static GList *keys = NULL;

  /* Check if browse is already done */
  if (container->priv->browse_status == DONE) {
    return;
  }

  /* Check if we need to perform a browse */
  if (container->priv->browse_status == NOT_DONE) {
    /* Get the source */
    g_object_get (container,
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

    container->priv->browse_status = IN_PROGRESS;

    grl_media_source_browse (source,
                             media,
                             keys,
                             0,
                             RYGEL_GRILO_MEDIA_CONTAINER_GET_CLASS (container)->limit,
                             GRL_RESOLVE_FULL | GRL_RESOLVE_IDLE_RELAY,
                             browse_result_cb,
                             container);
  }

  /* Check if we need to wait until operation finishes */
  if (container->priv->browse_status == IN_PROGRESS) {
    mainloop = g_main_loop_new (NULL, TRUE);
    mainloop_context = g_main_loop_get_context (mainloop);
    while (container->priv->browse_status != DONE) {
      g_main_context_iteration (mainloop_context, TRUE);
    }
  }
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

  browse_grilo_media (self);
  switch (prop_id) {
  case PROP_ITEM_COUNT:
    g_value_set_uint (value, self->priv->item_count);
    break;
  case PROP_CONTAINER_COUNT:
    g_value_set_uint (value, self->priv->container_count);
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

  switch (prop_id) {
  case PROP_ITEM_COUNT:
    self->priv->item_count = g_value_get_uint (value);
    break;
  case PROP_CONTAINER_COUNT:
    self->priv->container_count = g_value_get_uint (value);
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

  /* Free children */
  g_list_foreach (self->priv->containers, (GFunc) free_child, NULL);
  g_list_free (self->priv->containers);

  g_list_foreach (self->priv->items, (GFunc) free_child, NULL);
  g_list_free (self->priv->items);

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
  server->priv = RYGEL_GRILO_MEDIA_CONTAINER_GET_PRIVATE (server);
  memset (server->priv, 0, sizeof (RygelGriloMediaContainerPrivate));
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

  if (rygel_grilo_media_object_dbus_register (RYGEL_GRILO_MEDIA_OBJECT (obj))) {
    return obj;
  } else {
    g_object_unref (obj);
    return NULL;
  }
}


/**
 * rygel_grilo_media_container_new_with_parent:
 * @parent: parent object
 * @media: grilo media object to be wrapped
 *
 * Creates a new container that is child of another container.
 *
 * Returns: a new RygelGriloMediaContainer registered on dbus, or @NULL
 * otherwise
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
  if (rygel_grilo_media_object_dbus_register (RYGEL_GRILO_MEDIA_OBJECT (obj))) {
    klass->index++;
    return obj;
  } else {
    g_object_unref (obj);
    return NULL;
  }
}
