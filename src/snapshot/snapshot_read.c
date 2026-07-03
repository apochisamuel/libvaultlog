#include "tm/snapshot.h"
#include <string.h>

/*
 * Snapshot read operations: multi-segment materialization,
 * snapshot statistics, and validation.
 */

tm_status_t tm_snapshot_materialize_all(tm_snapshot_t *snap,
                                          uint8_t *dst, size_t dst_cap,
                                          size_t *out_len)
{
    if (!snap || !snap->active || !dst || !out_len) return TM_ERR_BAD_STATE;

    size_t total = 0;
    for (uint32_t i = 0; i < snap->segment_count; i++) {
        tm_segment_t *seg = snap->segments[i];
        if (!seg || !seg->data) continue;

        size_t remaining = dst_cap - total;
        if (remaining == 0) break;

        size_t copy = (seg->data_len > remaining) ? remaining : seg->data_len;
        memcpy(dst + total, seg->data, copy);
        total += copy;
    }

    *out_len = total;
    return TM_OK;
}

bool tm_snapshot_contains_segment(const tm_snapshot_t *snap, uint32_t segment_id)
{
    if (!snap || !snap->active) return false;
    for (uint32_t i = 0; i < snap->segment_count; i++) {
        if (snap->segment_ids[i] == segment_id) return true;
    }
    return false;
}

typedef struct {
    uint32_t  snapshot_id;
    uint32_t  epoch_captured;
    uint32_t  segment_count;
    uint64_t  total_data_size;
    bool      all_segments_valid;
} tm_snapshot_stats_t;

tm_status_t tm_snapshot_compute_stats(const tm_snapshot_t *snap,
                                         tm_snapshot_stats_t *stats)
{
    if (!snap || !stats) return TM_ERR_BAD_STATE;
    memset(stats, 0, sizeof(*stats));

    stats->snapshot_id = snap->snapshot_id;
    stats->epoch_captured = snap->epoch_captured;
    stats->segment_count = snap->segment_count;
    stats->all_segments_valid = true;

    for (uint32_t i = 0; i < snap->segment_count; i++) {
        tm_segment_t *seg = snap->segments[i];
        if (!seg || !seg->data) {
            stats->all_segments_valid = false;
            continue;
        }
        stats->total_data_size += seg->data_len;
    }

    return TM_OK;
}
