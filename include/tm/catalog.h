#ifndef TM_CATALOG_H
#define TM_CATALOG_H
#include "tm/types.h"
#include "tm/error.h"

typedef struct {
    uint32_t  segment_id;
    uint32_t  epoch_registered;
    uint32_t  access_count;
    uint32_t  generation;
    void     *segment_ref;      /* raw pointer to segment */
    bool      valid;
    bool      pinned;           /* pinned by cursor — cannot evict */
} tm_catalog_entry_t;

typedef struct {
    tm_catalog_entry_t  entries[TM_MAX_SEGMENTS];
    size_t              count;
    size_t              capacity;
    uint32_t            eviction_threshold;
    uint32_t            catalog_generation;
} tm_catalog_t;

tm_status_t  tm_catalog_init(tm_catalog_t *cat);
void         tm_catalog_destroy(tm_catalog_t *cat);
tm_status_t  tm_catalog_register(tm_catalog_t *cat, uint32_t segment_id,
                                   void *segment_ptr, uint32_t epoch);
tm_catalog_entry_t *tm_catalog_lookup(tm_catalog_t *cat, uint32_t segment_id);
tm_status_t  tm_catalog_access(tm_catalog_t *cat, uint32_t segment_id);
tm_status_t  tm_catalog_remove(tm_catalog_t *cat, uint32_t segment_id);
tm_status_t  tm_catalog_evict_lru(tm_catalog_t *cat);
tm_status_t  tm_catalog_pin(tm_catalog_t *cat, uint32_t segment_id);
tm_status_t  tm_catalog_unpin(tm_catalog_t *cat, uint32_t segment_id);
#endif
