#include "tm/catalog.h"
#include <string.h>

tm_status_t tm_catalog_init(tm_catalog_t *cat)
{
    if (!cat) return TM_ERR_BAD_STATE;
    memset(cat, 0, sizeof(*cat));
    cat->capacity = TM_MAX_SEGMENTS;
    cat->eviction_threshold = TM_MAX_SEGMENTS * 3 / 4;
    cat->catalog_generation = 1;
    return TM_OK;
}

void tm_catalog_destroy(tm_catalog_t *cat)
{
    if (cat) memset(cat, 0, sizeof(*cat));
}

tm_status_t tm_catalog_register(tm_catalog_t *cat, uint32_t segment_id,
                                  void *segment_ptr, uint32_t epoch)
{
    if (!cat) return TM_ERR_BAD_STATE;
    for (size_t i = 0; i < cat->capacity; i++) {
        if (!cat->entries[i].valid) {
            cat->entries[i].segment_id      = segment_id;
            cat->entries[i].epoch_registered = epoch;
            cat->entries[i].access_count    = 0;
            cat->entries[i].generation      = cat->catalog_generation++;
            cat->entries[i].segment_ref     = segment_ptr;
            cat->entries[i].valid           = true;
            cat->entries[i].pinned          = false;
            cat->count++;
            return TM_OK;
        }
    }
    return TM_ERR_OVERFLOW;
}

tm_catalog_entry_t *tm_catalog_lookup(tm_catalog_t *cat, uint32_t segment_id)
{
    if (!cat) return NULL;
    for (size_t i = 0; i < cat->capacity; i++) {
        if (cat->entries[i].valid && cat->entries[i].segment_id == segment_id)
            return &cat->entries[i];
    }
    return NULL;
}

tm_status_t tm_catalog_access(tm_catalog_t *cat, uint32_t segment_id)
{
    tm_catalog_entry_t *e = tm_catalog_lookup(cat, segment_id);
    if (!e) return TM_ERR_NOT_FOUND;
    e->access_count++;
    return TM_OK;
}

tm_status_t tm_catalog_remove(tm_catalog_t *cat, uint32_t segment_id)
{
    if (!cat) return TM_ERR_BAD_STATE;
    for (size_t i = 0; i < cat->capacity; i++) {
        if (cat->entries[i].valid && cat->entries[i].segment_id == segment_id) {
            cat->entries[i].valid = false;
            cat->entries[i].segment_ref = NULL;
            cat->count--;
            return TM_OK;
        }
    }
    return TM_ERR_NOT_FOUND;
}

tm_status_t tm_catalog_evict_lru(tm_catalog_t *cat)
{
    if (!cat) return TM_ERR_BAD_STATE;
    /* Find entry with lowest access_count that isn't pinned */
    int32_t min_idx = -1;
    uint32_t min_access = UINT32_MAX;
    for (size_t i = 0; i < cat->capacity; i++) {
        if (cat->entries[i].valid && !cat->entries[i].pinned &&
            cat->entries[i].access_count < min_access) {
            min_access = cat->entries[i].access_count;
            min_idx = (int32_t)i;
        }
    }
    if (min_idx < 0) return TM_ERR_NOT_FOUND;

    cat->entries[min_idx].valid = false;
    cat->entries[min_idx].segment_ref = NULL;
    cat->count--;
    return TM_OK;
}

tm_status_t tm_catalog_pin(tm_catalog_t *cat, uint32_t segment_id)
{
    tm_catalog_entry_t *e = tm_catalog_lookup(cat, segment_id);
    if (!e) return TM_ERR_NOT_FOUND;
    e->pinned = true;
    return TM_OK;
}

tm_status_t tm_catalog_unpin(tm_catalog_t *cat, uint32_t segment_id)
{
    tm_catalog_entry_t *e = tm_catalog_lookup(cat, segment_id);
    if (!e) return TM_ERR_NOT_FOUND;
    e->pinned = false;
    return TM_OK;
}
