#include "tm/index.h"
#include "tm/segment.h"
#include <string.h>

/*
 * Index compaction: removes entries for destroyed segments,
 * rebuilds with optimal capacity.
 */

tm_status_t tm_index_compact(tm_index_t *idx, const tm_segment_table_t *segs)
{
    if (!idx || !segs) return TM_ERR_BAD_STATE;

    /* Invalidate entries for non-existent segments */
    for (size_t i = 0; i < idx->capacity; i++) {
        if (!idx->entries[i].valid) continue;

        bool found = false;
        for (size_t j = 0; j < TM_MAX_SEGMENTS; j++) {
            const tm_segment_t *s = segs->segments[j];
            if (s && s->active && s->segment_id == idx->entries[i].segment_id) {
                found = true;
                break;
            }
        }
        if (!found) {
            idx->entries[i].valid = false;
            idx->count--;
        }
    }

    return TM_OK;
}

tm_status_t tm_index_rebuild_from_segments(tm_index_t *idx,
                                              const tm_segment_table_t *segs)
{
    if (!idx || !segs) return TM_ERR_BAD_STATE;

    /* Clear index */
    for (size_t i = 0; i < idx->capacity; i++) {
        idx->entries[i].valid = false;
    }
    idx->count = 0;

    /* Re-insert entries for active segments */
    uint64_t timestamp = 1;
    for (size_t i = 0; i < TM_MAX_SEGMENTS; i++) {
        const tm_segment_t *s = segs->segments[i];
        if (!s || !s->active) continue;

        tm_index_insert(idx, timestamp, s->segment_id, 0, NULL, NULL);
        timestamp += 100;
    }

    idx->rebuild_generation++;
    return TM_OK;
}

size_t tm_index_memory_usage(const tm_index_t *idx)
{
    if (!idx) return 0;
    return sizeof(tm_index_t) + idx->capacity * sizeof(tm_index_entry_t);
}
