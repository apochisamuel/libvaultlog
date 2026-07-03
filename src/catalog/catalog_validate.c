#include "tm/catalog.h"
#include "tm/segment.h"
#include <string.h>

tm_status_t tm_catalog_validate(const tm_catalog_t *cat,
                                   const tm_segment_table_t *segs,
                                   uint32_t *orphaned_count)
{
    if (!cat || !segs || !orphaned_count) return TM_ERR_BAD_STATE;
    *orphaned_count = 0;

    for (size_t i = 0; i < cat->capacity; i++) {
        const tm_catalog_entry_t *ce = &cat->entries[i];
        if (!ce->valid) continue;

        bool found = false;
        for (size_t j = 0; j < TM_MAX_SEGMENTS; j++) {
            const tm_segment_t *s = segs->segments[j];
            if (s && s->active && s->segment_id == ce->segment_id) {
                found = true;
                break;
            }
        }
        if (!found) (*orphaned_count)++;
    }

    return TM_OK;
}

tm_status_t tm_catalog_sync_with_segments(tm_catalog_t *cat,
                                             const tm_segment_table_t *segs)
{
    if (!cat || !segs) return TM_ERR_BAD_STATE;

    /* Remove catalog entries for dead segments */
    for (size_t i = 0; i < cat->capacity; i++) {
        tm_catalog_entry_t *ce = &cat->entries[i];
        if (!ce->valid) continue;

        bool found = false;
        for (size_t j = 0; j < TM_MAX_SEGMENTS; j++) {
            const tm_segment_t *s = segs->segments[j];
            if (s && s->active && s->segment_id == ce->segment_id) {
                found = true;
                /* Update segment_ref */
                ce->segment_ref = (void *)s;
                break;
            }
        }
        if (!found) {
            ce->valid = false;
            ce->segment_ref = NULL;
            cat->count--;
        }
    }

    return TM_OK;
}

size_t tm_catalog_memory_usage(const tm_catalog_t *cat)
{
    if (!cat) return 0;
    return sizeof(tm_catalog_t);
}
