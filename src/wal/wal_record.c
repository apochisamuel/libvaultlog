#include "tm/wal.h"
#include "tm/slab.h"
#include <string.h>

/*
 * WAL record management: creation, validation, and lifecycle
 * tracking for write-ahead log entries.
 */

typedef struct {
    size_t    total_entries;
    size_t    valid_entries;
    size_t    replayed_entries;
    uint32_t  min_epoch;
    uint32_t  max_epoch;
    uint32_t  total_data_bytes;
} tm_wal_stats_t;

tm_status_t tm_wal_compute_stats(const tm_wal_t *wal, tm_wal_stats_t *stats)
{
    if (!wal || !stats) return TM_ERR_BAD_STATE;
    memset(stats, 0, sizeof(*stats));
    stats->min_epoch = UINT32_MAX;

    for (size_t i = 0; i < wal->count; i++) {
        const tm_wal_entry_t *e = &wal->entries[i];
        stats->total_entries++;
        if (e->valid) stats->valid_entries++;
        if (e->replayed) stats->replayed_entries++;
        if (e->epoch < stats->min_epoch) stats->min_epoch = e->epoch;
        if (e->epoch > stats->max_epoch) stats->max_epoch = e->epoch;
        stats->total_data_bytes += e->data_length;
    }

    if (stats->total_entries == 0) stats->min_epoch = 0;
    return TM_OK;
}

tm_status_t tm_wal_mark_replayed(tm_wal_t *wal, uint32_t entry_id)
{
    if (!wal) return TM_ERR_BAD_STATE;
    for (size_t i = 0; i < wal->count; i++) {
        if (wal->entries[i].entry_id == entry_id) {
            wal->entries[i].replayed = true;
            return TM_OK;
        }
    }
    return TM_ERR_NOT_FOUND;
}

tm_status_t tm_wal_invalidate_segment(tm_wal_t *wal, uint32_t segment_id)
{
    if (!wal) return TM_ERR_BAD_STATE;
    for (size_t i = 0; i < wal->count; i++) {
        if (wal->entries[i].segment_id == segment_id) {
            wal->entries[i].valid = false;
        }
    }
    return TM_OK;
}

tm_status_t tm_wal_truncate_before_epoch(tm_wal_t *wal, uint32_t epoch)
{
    if (!wal) return TM_ERR_BAD_STATE;
    for (size_t i = 0; i < wal->count; i++) {
        if (wal->entries[i].epoch < epoch) {
            wal->entries[i].valid = false;
        }
    }
    return TM_OK;
}

size_t tm_wal_pending_count(const tm_wal_t *wal)
{
    if (!wal) return 0;
    size_t count = 0;
    for (size_t i = 0; i < wal->count; i++) {
        if (wal->entries[i].valid && !wal->entries[i].replayed)
            count++;
    }
    return count;
}

tm_status_t tm_wal_set_recovery_mode(tm_wal_t *wal, bool mode)
{
    if (!wal) return TM_ERR_BAD_STATE;
    wal->recovery_mode = mode;
    return TM_OK;
}
