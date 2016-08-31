#ifndef __GSK_RESOURCE_CACHE_PRIVATE_H__
#define __GSK_RESOURCE_CACHE_PRIVATE_H__

#include "gskresourcecache.h"

G_BEGIN_DECLS

#define GSK_RESOURCE_CACHE_CLASS(klass)         (G_TYPE_CHECK_CLASS_CAST ((klass), GSK_TYPE_RESOURCE_CACHE, GskResourceCacheClass))
#define GSK_IS_RESOURCE_CACHE_CLASS(klass)      (G_TYPE_CHECK_CLASS_TYPE ((klass), GSK_TYPE_RESOURCE_CACHE))
#define GSK_RESOURCE_CACHE_GET_CLASS(obj)       (G_TYPE_INSTANCE_GET_CLASS ((obj), GSK_TYPE_RESOURCE_CACHE, GskResourceCacheClass))

struct _GskResourceCache
{
  GObject parent_instance;
};

struct _GskResourceCacheClass
{
  GObjectClass parent_class;
};

int             gsk_resource_cache_collect_items        (GskResourceCache *cache);

G_END_DECLS

#endif /* __GSK_RESOURCE_CACHE_PRIVATE_H__ */
