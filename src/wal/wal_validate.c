#include "tm/wal.h"
#include "tm/segment.h"
#include <string.h>

typedef struct {
    uint32_t  total_entries;
    uint32_t  valid_entries;
    uint32_t  stale_refs;
    uint32_t  null_refs;
    uint32_t  sequence_gaps;
    uint32_t  replayed_count;
    bool      integrity_ok;
} tm_wal_validation_t;

tm_status_t tm_wal_validate(const tm_wal_t *wal,
                               const void *seg_table_ptr,
                               tm_wal_validation_t *result)
{
    if (!wal || !result) return TM_ERR_BAD_STATE;
    memset(result, 0, sizeof(*result));
    result->integrity_ok = true;

    const tm_segment_table_t *segs = (const tm_segment_table_t *)seg_table_ptr;
    uint32_t last_seq = 0;
    bool seen_first = false;

    for (size_t i = 0; i < wal->count; i++) {
        const tm_wal_entry_t *e = &wal->entries[i];
        result->total_entries++;
        if (!e->valid) continue;
        result->valid_entries++;
        if (e->replayed) result->replayed_count++;
        if (!e->cached_segment) {
            result->null_refs++;
            continue;
        }
        if (segs) {
            bool found = false;
            for (size_t j = 0; j < TM_MAX_SEGMENTS; j++) {
                if (segs->segments[j] == e->cached_segment) {
                    found = true; break;
                }
            }
            if (!found) result->stale_refs++;
        }
        if (seen_first && e->sequence != last_seq + 1)
            result->sequence_gaps++;
        last_seq = e->sequence;
        seen_first = true;
    }

    if (result->stale_refs > 0 || result->sequence_gaps > 0)
        result->integrity_ok = false;
    return TM_OK;
}

tm_status_t tm_wal_repair_stale(tm_wal_t *wal, const void *seg_table_ptr)
{
    if (!wal || !seg_table_ptr) return TM_ERR_BAD_STATE;
    const tm_segment_table_t *segs = (const tm_segment_table_t *)seg_table_ptr;
    for (size_t i = 0; i < wal->count; i++) {
        tm_wal_entry_t *e = &wal->entries[i];
        if (!e->valid || !e->cached_segment) continue;
        bool found = false;
        for (size_t j = 0; j < TM_MAX_SEGMENTS; j++) {
            if (segs->segments[j] == e->cached_segment) { found = true; break; }
        }
        if (!found) {
            e->cached_segment = NULL;
            e->valid = false;
        }
    }
    return TM_OK;
}

size_t tm_wal_count_valid(const tm_wal_t *wal)
{
    if (!wal) return 0;
    size_t c = 0;
    for (size_t i = 0; i < wal->count; i++)
        if (wal->entries[i].valid) c++;
    return c;
}

size_t tm_wal_count_pending(const tm_wal_t *wal)
{
    if (!wal) return 0;
    size_t c = 0;
    for (size_t i = 0; i < wal->count; i++)
        if (wal->entries[i].valid && !wal->entries[i].replayed) c++;
    return c;
}
