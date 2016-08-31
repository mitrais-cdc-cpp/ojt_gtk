#include "config.h"

#include "gskresourcecacheprivate.h"

typedef struct {
  GHashTable *resources;

  GHashFunc key_hash;
  GEqualFunc key_equal;
  GDestroyNotify key_notify;

  GType value_type;

  /* Interned, do not free */
  const char *name;
} GskResourceCachePrivate;

typedef struct {
  GType value_type;
  gpointer value;
  gint64 last_access_time;
  gint64 age;
} ResourceCacheItem;

enum {
  PROP_NAME = 1,

  N_PROPERTIES
};

static GParamSpec *gsk_resource_cache_properties[N_PROPERTIES];

G_DEFINE_ABSTRACT_TYPE_WITH_PRIVATE (GskResourceCache, gsk_resource_cache, G_TYPE_OBJECT)

static void
gsk_resource_cache_dispose (GObject *gobject)
{
  GskResourceCache *self = GSK_RESOURCE_CACHE (gobject);
  GskResourceCachePrivate *priv = gsk_resource_cache_get_instance_private (self);

  g_clear_pointer (&priv->resources, g_hash_table_unref);

  G_OBJECT_CLASS (gsk_resource_cache_parent_class)->dispose (gobject);
}

static void
gsk_resource_cache_set_property (GObject      *gobject,
                                 guint         prop_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
  GskResourceCache *self = GSK_RESOURCE_CACHE (gobject);
  GskResourceCachePrivate *priv = gsk_resource_cache_get_instance_private (self);

  switch (prop_id)
    {
    case PROP_NAME:
      if (g_value_get_string (value) != NULL)
        priv->name = g_intern_static_string (g_value_get_string (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
    }
}

static void
gsk_resource_cache_get_property (GObject    *gobject,
                                 guint       prop_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
  GskResourceCache *self = GSK_RESOURCE_CACHE (gobject);
  GskResourceCachePrivate *priv = gsk_resource_cache_get_instance_private (self);

  switch (prop_id)
    {
    case PROP_NAME:
      g_value_set_string (value, priv->name);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
    }
}

static void
gsk_resource_cache_class_init (GskResourceCacheClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->set_property = gsk_resource_cache_set_property;
  gobject_class->get_property = gsk_resource_cache_get_property;
  gobject_class->dispose = gsk_resource_cache_dispose;

  gsk_resource_cache_properties[PROP_NAME] =
    g_param_spec_string ("name", "Name", "The (short) name of the resource cache",
                         NULL,
                         G_PARAM_CONSTRUCT_ONLY |
                         G_PARAM_READWRITE |
                         G_PARAM_STATIC_STRINGS);
}

static void
gsk_resource_cache_init (GskResourceCache *self)
{
}

/**
 * gsk_resource_cache_set_hash_func:
 * @cache: a #GskResourceCache
 * @key_hash: a hashing function for the key
 *
 * Sets the hashing function for the keys inside the resource @cache.
 *
 * Since: 3.22
 */
void
gsk_resource_cache_set_hash_func (GskResourceCache *cache,
                                  GHashFunc         key_hash)
{
  GskResourceCachePrivate *priv = gsk_resource_cache_get_instance_private (cache);

  g_return_if_fail (GSK_IS_RESOURCE_CACHE (cache));
  g_return_if_fail (priv->resources == NULL);

  priv->key_hash = key_hash;
}

/**
 * gsk_resource_cache_set_equal_func:
 * @cache: a #GskResourceCache
 * @key_equal: a function for checking two keys for equality
 *
 * Sets the comparison function for two keys in the resource @cache.
 *
 * Since: 3.22
 */
void
gsk_resource_cache_set_equal_func (GskResourceCache *cache,
                                   GEqualFunc        key_equal)
{
  GskResourceCachePrivate *priv = gsk_resource_cache_get_instance_private (cache);

  g_return_if_fail (GSK_IS_RESOURCE_CACHE (cache));
  g_return_if_fail (priv->resources == NULL);

  priv->key_equal = key_equal;
}

/**
 * gsk_resource_cache_set_free_func:
 * @cache: a #GskResourceCache
 * @key_notify: a function to be called when removing a key
 *
 * Sets the function to be called when removing a key from the resource @cache.
 *
 * Since: 3.22
 */
void
gsk_resource_cache_set_free_func (GskResourceCache *cache,
                                  GDestroyNotify    key_notify)
{
  GskResourceCachePrivate *priv = gsk_resource_cache_get_instance_private (cache);

  g_return_if_fail (GSK_IS_RESOURCE_CACHE (cache));
  g_return_if_fail (priv->resources == NULL);

  priv->key_notify = key_notify;
}

/**
 * gsk_resource_cache_set_value_type:
 * @cache: a #GskResourceCache
 * @value_type: the type of the data to be stored inside the cache
 *
 * Sets the #GType of the values inside the resource @cache.
 *
 * Only pointer-sized types can be stored inside the cache, i.e. the only
 * admissible types for @value_type are:
 *
 *  - %G_TYPE_POINTER (no memory management)
 *  - %G_TYPE_BOXED (the cache will copy the data)
 *  - %G_TYPE_OBJECT (the cache will acquire a reference to the data)
 *
 * Since: 3.22
 */
void
gsk_resource_cache_set_value_type (GskResourceCache *cache,
                                   GType             value_type)
{
  GskResourceCachePrivate *priv = gsk_resource_cache_get_instance_private (cache);

  g_return_if_fail (GSK_IS_RESOURCE_CACHE (cache));
  g_return_if_fail (priv->resources == NULL);
  g_return_if_fail (value_type != G_TYPE_INVALID);

  if (G_TYPE_FUNDAMENTAL (value_type) != G_TYPE_POINTER ||
      G_TYPE_FUNDAMENTAL (value_type) != G_TYPE_BOXED ||
      G_TYPE_FUNDAMENTAL (value_type) != G_TYPE_OBJECT)
    {
      g_critical ("Unsupported resource type '%s'", g_type_name (value_type));
      return;
    }

  priv->value_type = value_type;
}

/**
 * gsk_resource_cache_set_name:
 * @cache: a #GskResourceCache
 * @name: a name for the @cache
 *
 * Sets a name for the resource @cache, for debugging purposes.
 *
 * Since: 3.22
 */
void
gsk_resource_cache_set_name (GskResourceCache *cache,
                             const char       *name)
{
  GskResourceCachePrivate *priv = gsk_resource_cache_get_instance_private (cache);

  g_return_if_fail (GSK_IS_RESOURCE_CACHE (cache));

  priv->name = g_intern_string (name);
}

static ResourceCacheItem *
resource_cache_item_new (GType    value_type,
                         gpointer value)
{
  ResourceCacheItem *item;

  if (value_type == G_TYPE_INVALID)
    {
      g_critical ("Invalid resource type; did you forget to call "
                  "gsk_resource_cache_set_value_type()?");
      return NULL;
    }

  item = g_slice_new0 (ResourceCacheItem);
  item->value_type = value_type;

  switch (G_TYPE_FUNDAMENTAL (item->value_type))
    {
    case G_TYPE_POINTER:
      item->value = value;
      break;

    case G_TYPE_BOXED:
      if (value != NULL)
        item->value = g_boxed_copy (item->value_type, value);
      break;

    case G_TYPE_OBJECT:
      if (value != NULL)
        item->value = g_object_ref (value);
      break;

    default:
      g_critical ("Unsupported resource type '%s'", g_type_name (value_type));
      g_slice_free (ResourceCacheItem, item);
      return NULL;
    }

  item->age = 1;

  return item;
}

static gpointer
resource_cache_item_get_value (ResourceCacheItem *item)
{
  item->last_access_time = g_get_monotonic_time ();

  return item->value;
}

static void
resource_cache_item_free (gpointer data)
{
  ResourceCacheItem *item = data;

  if (data == NULL)
    return;

  switch (G_TYPE_FUNDAMENTAL (item->value_type))
    {
    case G_TYPE_POINTER:
      break;

    case G_TYPE_BOXED:
      if (item->value != NULL)
        g_boxed_free (item->value_type, item->value);
      break;

    case G_TYPE_OBJECT:
      if (item->value != NULL)
        g_object_unref (item->value);
      break;

    default:
      g_critical ("Unsupported resource type '%s'", g_type_name (item->value_type));
    }

  g_slice_free (ResourceCacheItem, item);
}

void
gsk_resource_cache_add_item (GskResourceCache *cache,
                             gpointer          key,
                             gpointer          value)
{
  GskResourceCachePrivate *priv = gsk_resource_cache_get_instance_private (cache);
  ResourceCacheItem *item;

  g_return_if_fail (GSK_IS_RESOURCE_CACHE (cache));

  if (priv->resources == NULL)
    {
      priv->resources = g_hash_table_new_full (priv->key_hash, priv->key_equal,
                                               priv->key_notify,
                                               resource_cache_item_free);
    }

  item = g_hash_table_lookup (priv->resources, key);
  if (item != NULL)
    {
      item->age += 1;
      return;
    }

  item = resource_cache_item_new (priv->value_type, value);
  if (item != NULL)
    g_hash_table_insert (priv->resources, key, item);
}

gboolean
gsk_resource_cache_has_item (GskResourceCache *cache,
                             gpointer          key)
{
  GskResourceCachePrivate *priv = gsk_resource_cache_get_instance_private (cache);

  g_return_val_if_fail (GSK_IS_RESOURCE_CACHE (cache), FALSE);

  if (priv->resources == NULL)
    return FALSE;

  return g_hash_table_lookup (priv->resources, key) != NULL;
}

gpointer
gsk_resource_cache_get_item (GskResourceCache *cache,
                             gpointer          key)
{
  GskResourceCachePrivate *priv = gsk_resource_cache_get_instance_private (cache);
  ResourceCacheItem *item;

  g_return_val_if_fail (GSK_IS_RESOURCE_CACHE (cache), FALSE);

  if (priv->resources == NULL)
    return NULL;

  item = g_hash_table_lookup (priv->resources, key);
  if (item == NULL)
    return NULL;

  return resource_cache_item_get_value (item);
}

gboolean
gsk_resource_cache_invalidate_item (GskResourceCache *cache,
                                    gpointer          key)
{
  GskResourceCachePrivate *priv = gsk_resource_cache_get_instance_private (cache);
  ResourceCacheItem *item;

  g_return_val_if_fail (GSK_IS_RESOURCE_CACHE (cache), FALSE);

  if (priv->resources == NULL)
    return FALSE;

  item = g_hash_table_lookup (priv->resources, key);
  if (item == NULL)
    return FALSE;

  item->age -= 1;
  if (item->age < 0)
    {
      g_critical ("Too many invalidations for item of type '%s'", g_type_name (priv->value_type));
      item->age = 0;
    }

  return TRUE;
}

int
gsk_resource_cache_collect_items (GskResourceCache *cache)
{
  GskResourceCachePrivate *priv = gsk_resource_cache_get_instance_private (cache);
  GHashTableIter iter;
  gpointer item_p;
  int res = 0;

  g_return_val_if_fail (GSK_IS_RESOURCE_CACHE (cache), 0);

  if (priv->resources == NULL)
    return 0;

  g_hash_table_iter_init (&iter, priv->resources);
  while (g_hash_table_iter_next (&iter, NULL, &item_p))
    {
      ResourceCacheItem *item = item_p;

      if (item->age == 0)
        {
          g_hash_table_iter_remove (&iter);
          res += 1;
          continue;
        }

      item->age -= 1;
    }

  return res;
}
