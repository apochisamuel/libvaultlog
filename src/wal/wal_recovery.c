#include "tm/wal.h"
#include "tm/segment.h"
#include "tm/slab.h"
#include <string.h>

typedef struct {
    uint32_t  entries_checked;
    uint32_t  entries_recovered;
    uint32_t  entries_skipped;
    uint32_t  gaps_detected;
    bool      recovery_complete;
} tm_wal_recovery_stats_t;

tm_status_t tm_wal_begin_recovery(tm_wal_t *wal)
{
    if (!wal) return TM_ERR_BAD_STATE;
    wal->recovery_mode = true;
    wal->has_gap = false;
    wal->replay_sequence = 0;
    return TM_OK;
}

tm_status_t tm_wal_end_recovery(tm_wal_t *wal)
{
    if (!wal) return TM_ERR_BAD_STATE;
    wal->recovery_mode = false;
    return TM_OK;
}

tm_status_t tm_wal_verify_entries(const tm_wal_t *wal,
                                    tm_wal_recovery_stats_t *stats)
{
    if (!wal || !stats) return TM_ERR_BAD_STATE;
    memset(stats, 0, sizeof(*stats));

    uint32_t last_sequence = 0;
    bool seen_first = false;

    for (size_t i = 0; i < wal->count; i++) {
        const tm_wal_entry_t *e = &wal->entries[i];
        stats->entries_checked++;

        if (!e->valid) {
            stats->entries_skipped++;
            continue;
        }

        /* Check for sequence gaps */
        if (seen_first && e->sequence != last_sequence + 1) {
            stats->gaps_detected++;
        }
        last_sequence = e->sequence;
        seen_first = true;

        /* Verify cached pointer is non-NULL */
        if (e->cached_segment) {
            stats->entries_recovered++;
        } else {
            stats->entries_skipped++;
        }
    }

    stats->recovery_complete = (stats->gaps_detected == 0);
    return TM_OK;
}

tm_status_t tm_wal_selective_replay(tm_wal_t *wal,
                                       uint32_t start_epoch,
                                       uint32_t end_epoch,
                                       uint8_t *dst, size_t dst_cap,
                                       size_t *total_replayed)
{
    if (!wal || !dst || !total_replayed) return TM_ERR_BAD_STATE;
    *total_replayed = 0;

    for (size_t i = 0; i < wal->count; i++) {
        tm_wal_entry_t *e = &wal->entries[i];
        if (!e->valid || e->replayed) continue;
        if (e->epoch < start_epoch || e->epoch > end_epoch) continue;

        size_t entry_len = 0;
        tm_status_t rc = tm_wal_replay_entry(e, dst + *total_replayed,
                                               dst_cap - *total_replayed,
                                               &entry_len);
        if (rc == TM_OK) {
            e->replayed = true;
            *total_replayed += entry_len;
        }
    }

    return TM_OK;
}

tm_status_t tm_wal_compact_replayed(tm_wal_t *wal)
{
    if (!wal) return TM_ERR_BAD_STATE;
    /* Move unreplayed entries to the front */
    size_t write_pos = 0;
    for (size_t i = 0; i < wal->count; i++) {
        if (!wal->entries[i].replayed && wal->entries[i].valid) {
            if (write_pos != i) {
                wal->entries[write_pos] = wal->entries[i];
            }
            write_pos++;
        }
    }
    /* Clear remaining slots */
    for (size_t i = write_pos; i < wal->count; i++) {
        memset(&wal->entries[i], 0, sizeof(tm_wal_entry_t));
    }
    wal->count = write_pos;
    return TM_OK;
}
