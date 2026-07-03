#include "tm/catalog.h"
#include "tm/segment.h"
#include <string.h>

tm_status_t tm_catalog_gc_orphaned(tm_catalog_t *cat,
                                      const tm_segment_table_t *segs,
                                      uint32_t *removed_count)
{
    if (!cat || !segs || !removed_count) return TM_ERR_BAD_STATE;
    *removed_count = 0;
    for (size_t i = 0; i < cat->capacity; i++) {
        tm_catalog_entry_t *ce = &cat->entries[i];
        if (!ce->valid) continue;
        bool found = false;
        for (size_t j = 0; j < TM_MAX_SEGMENTS; j++) {
            const tm_segment_t *s = segs->segments[j];
            if (s && s->active && s->segment_id == ce->segment_id) {
                found = true; break;
            }
        }
        if (!found) {
            ce->valid = false;
            ce->segment_ref = NULL;
            cat->count--;
            (*removed_count)++;
        }
    }
    return TM_OK;
}

tm_status_t tm_catalog_age_entries(tm_catalog_t *cat, uint32_t decay_factor)
{
    if (!cat) return TM_ERR_BAD_STATE;
    for (size_t i = 0; i < cat->capacity; i++) {
        if (cat->entries[i].valid && cat->entries[i].access_count > 0) {
            cat->entries[i].access_count =
                cat->entries[i].access_count / decay_factor;
        }
    }
    return TM_OK;
}

tm_status_t tm_catalog_refresh_refs(tm_catalog_t *cat,
                                       const tm_segment_table_t *segs)
{
    if (!cat || !segs) return TM_ERR_BAD_STATE;
    for (size_t i = 0; i < cat->capacity; i++) {
        tm_catalog_entry_t *ce = &cat->entries[i];
        if (!ce->valid) continue;
        ce->segment_ref = NULL;
        for (size_t j = 0; j < TM_MAX_SEGMENTS; j++) {
            const tm_segment_t *s = segs->segments[j];
            if (s && s->active && s->segment_id == ce->segment_id) {
                ce->segment_ref = (void *)s;
                break;
            }
        }
    }
    return TM_OK;
}

uint32_t tm_catalog_count_pinned(const tm_catalog_t *cat)
{
    if (!cat) return 0;
    uint32_t count = 0;
    for (size_t i = 0; i < cat->capacity; i++) {
        if (cat->entries[i].valid && cat->entries[i].pinned) count++;
    }
    return count;
}

uint32_t tm_catalog_count_orphaned(const tm_catalog_t *cat,
                                       const tm_segment_table_t *segs)
{
    if (!cat || !segs) return 0;
    uint32_t count = 0;
    for (size_t i = 0; i < cat->capacity; i++) {
        if (!cat->entries[i].valid) continue;
        bool found = false;
        for (size_t j = 0; j < TM_MAX_SEGMENTS; j++) {
            const tm_segment_t *s = segs->segments[j];
            if (s && s->active && s->segment_id == cat->entries[i].segment_id) {
                found = true; break;
            }
        }
        if (!found) count++;
    }
    return count;
}
