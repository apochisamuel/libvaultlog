#include "tm/snapshot.h"
#include <string.h>

typedef struct {
    uint32_t  snapshots_checked;
    uint32_t  snapshots_valid;
    uint32_t  snapshots_stale;
    uint32_t  stale_refs;
    uint32_t  total_segments_ref;
} tm_snapshot_gc_report_t;

tm_status_t tm_snapshot_gc_check(tm_snapshot_t *const *snapshots,
                                    size_t max_snapshots,
                                    const void *seg_table_ptr,
                                    tm_snapshot_gc_report_t *report)
{
    if (!snapshots || !report) return TM_ERR_BAD_STATE;
    memset(report, 0, sizeof(*report));
    const tm_segment_table_t *segs = (const tm_segment_table_t *)seg_table_ptr;

    for (size_t i = 0; i < max_snapshots; i++) {
        const tm_snapshot_t *snap = snapshots[i];
        if (!snap || !snap->active) continue;
        report->snapshots_checked++;
        bool all_valid = true;

        for (uint32_t j = 0; j < snap->segment_count; j++) {
            report->total_segments_ref++;
            if (!snap->segments[j]) { all_valid = false; report->stale_refs++; continue; }
            if (segs) {
                bool found = false;
                for (size_t k = 0; k < TM_MAX_SEGMENTS; k++) {
                    if (segs->segments[k] == snap->segments[j]) { found = true; break; }
                }
                if (!found) { all_valid = false; report->stale_refs++; }
            }
        }
        if (all_valid) report->snapshots_valid++;
        else report->snapshots_stale++;
    }
    return TM_OK;
}

tm_status_t tm_snapshot_invalidate_segment(tm_snapshot_t *const *snapshots,
                                              size_t max_snapshots,
                                              uint32_t segment_id)
{
    if (!snapshots) return TM_ERR_BAD_STATE;
    for (size_t i = 0; i < max_snapshots; i++) {
        tm_snapshot_t *snap = snapshots[i];
        if (!snap || !snap->active) continue;
        for (uint32_t j = 0; j < snap->segment_count; j++) {
            if (snap->segment_ids[j] == segment_id) {
                snap->segments[j] = NULL;
            }
        }
    }
    return TM_OK;
}

uint32_t tm_snapshot_count_active(tm_snapshot_t *const *snapshots,
                                    size_t max_snapshots)
{
    if (!snapshots) return 0;
    uint32_t count = 0;
    for (size_t i = 0; i < max_snapshots; i++) {
        if (snapshots[i] && snapshots[i]->active) count++;
    }
    return count;
}

uint32_t tm_snapshot_oldest_epoch(tm_snapshot_t *const *snapshots,
                                    size_t max_snapshots)
{
    uint32_t oldest = UINT32_MAX;
    if (!snapshots) return 0;
    for (size_t i = 0; i < max_snapshots; i++) {
        if (snapshots[i] && snapshots[i]->active &&
            snapshots[i]->epoch_captured < oldest)
            oldest = snapshots[i]->epoch_captured;
    }
    return (oldest == UINT32_MAX) ? 0 : oldest;
}
