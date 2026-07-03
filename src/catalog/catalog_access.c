#include "tm/catalog.h"
#include <string.h>

typedef struct {
    uint32_t  segment_id;
    uint32_t  access_count;
    uint32_t  last_access_epoch;
} tm_catalog_access_record_t;

tm_status_t tm_catalog_access_with_epoch(tm_catalog_t *cat,
                                            uint32_t segment_id,
                                            uint32_t epoch)
{
    if (!cat) return TM_ERR_BAD_STATE;
    tm_catalog_entry_t *e = tm_catalog_lookup(cat, segment_id);
    if (!e) return TM_ERR_NOT_FOUND;
    e->access_count++;
    return TM_OK;
}

tm_status_t tm_catalog_bulk_access(tm_catalog_t *cat,
                                      const uint32_t *segment_ids,
                                      uint32_t count)
{
    if (!cat || !segment_ids) return TM_ERR_BAD_STATE;
    for (uint32_t i = 0; i < count; i++) {
        tm_catalog_access(cat, segment_ids[i]);
    }
    return TM_OK;
}

tm_status_t tm_catalog_find_least_accessed(const tm_catalog_t *cat,
                                              uint32_t *out_segment_id)
{
    if (!cat || !out_segment_id) return TM_ERR_BAD_STATE;
    uint32_t min_access = UINT32_MAX;
    int32_t min_idx = -1;
    for (size_t i = 0; i < cat->capacity; i++) {
        if (cat->entries[i].valid && !cat->entries[i].pinned &&
            cat->entries[i].access_count < min_access) {
            min_access = cat->entries[i].access_count;
            min_idx = (int32_t)i;
        }
    }
    if (min_idx < 0) return TM_ERR_NOT_FOUND;
    *out_segment_id = cat->entries[min_idx].segment_id;
    return TM_OK;
}

tm_status_t tm_catalog_find_most_accessed(const tm_catalog_t *cat,
                                             uint32_t *out_segment_id)
{
    if (!cat || !out_segment_id) return TM_ERR_BAD_STATE;
    uint32_t max_access = 0;
    int32_t max_idx = -1;
    for (size_t i = 0; i < cat->capacity; i++) {
        if (cat->entries[i].valid &&
            cat->entries[i].access_count > max_access) {
            max_access = cat->entries[i].access_count;
            max_idx = (int32_t)i;
        }
    }
    if (max_idx < 0) return TM_ERR_NOT_FOUND;
    *out_segment_id = cat->entries[max_idx].segment_id;
    return TM_OK;
}
