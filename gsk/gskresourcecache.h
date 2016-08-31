/* GSK - The GTK Scene Kit
 *
 * Copyright 2016  Endless
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __GSK_RESOURCE_CACHE_H__
#define __GSK_RESOURCE_CACHE_H__

#if !defined (__GSK_H_INSIDE__) && !defined (GSK_COMPILATION)
#error "Only <gsk/gsk.h> can be included directly."
#endif

#include <gsk/gsktypes.h>

G_BEGIN_DECLS

#define GSK_TYPE_RESOURCE_CACHE (gsk_resource_cache_get_type())

#define GSK_RESOURCE_CACHE(obj)         (G_TYPE_CHECK_INSTANCE_CAST ((obj), GSK_TYPE_RESOURCE_CACHE, GskResourceCache))
#define GSK_IS_RESOURCE_CACHE(obj)      (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GSK_TYPE_RESOURCE_CACHE))

typedef struct _GskResourceCache        GskResourceCache;
typedef struct _GskResourceCacheClass   GskResourceCacheClass;

GDK_AVAILABLE_IN_3_22
GType gsk_resource_cache_get_type (void) G_GNUC_CONST;

GDK_AVAILABLE_IN_3_22
void            gsk_resource_cache_set_hash_func        (GskResourceCache *cache,
                                                         GHashFunc         key_hash);
GDK_AVAILABLE_IN_3_22
void            gsk_resource_cache_set_equal_func       (GskResourceCache *cache,
                                                         GEqualFunc        key_equal);
GDK_AVAILABLE_IN_3_22
void            gsk_resource_cache_set_free_func        (GskResourceCache *cache,
                                                         GDestroyNotify    key_notify);
GDK_AVAILABLE_IN_3_22
void            gsk_resource_cache_set_value_type       (GskResourceCache *cache,
                                                         GType             value_type);
GDK_AVAILABLE_IN_3_22
void            gsk_resource_cache_set_name             (GskResourceCache *cache,
                                                         const char       *name);

GDK_AVAILABLE_IN_3_22
void            gsk_resource_cache_add_item             (GskResourceCache *cache,
                                                         gpointer          key,
                                                         gpointer          value);
GDK_AVAILABLE_IN_3_22
gboolean        gsk_resource_cache_has_item             (GskResourceCache *cache,
                                                         gpointer          key);
GDK_AVAILABLE_IN_3_22
gpointer        gsk_resource_cache_get_item             (GskResourceCache *cache,
                                                         gpointer          key);
GDK_AVAILABLE_IN_3_22
gboolean        gsk_resource_cache_invalidate_item      (GskResourceCache *cache,
                                                         gpointer          key);

G_END_DECLS

#endif /* __GSK_RESOURCE_CACHE_H__ */
