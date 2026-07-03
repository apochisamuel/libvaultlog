#include "tm/session.h"
#include <string.h>
#include <stdlib.h>

typedef struct {
    uint32_t  snapshot_ids[TM_MAX_SNAPSHOTS];
    uint32_t  segment_counts[TM_MAX_SNAPSHOTS];
    uint32_t  snapshot_count;
    uint32_t  total_segments_referenced;
    uint64_t  total_data_referenced;
} tm_snapshot_inventory_t;

tm_status_t tm_session_snapshot_inventory(const tm_session_t *sess,
                                            tm_snapshot_inventory_t *inv)
{
    if (!sess || !inv) return TM_ERR_BAD_STATE;
    memset(inv, 0, sizeof(*inv));
    for (size_t i = 0; i < TM_MAX_SNAPSHOTS; i++) {
        const tm_snapshot_t *snap = sess->snapshots[i];
        if (!snap || !snap->active) continue;
        inv->snapshot_ids[inv->snapshot_count] = snap->snapshot_id;
        inv->segment_counts[inv->snapshot_count] = snap->segment_count;
        inv->total_segments_referenced += snap->segment_count;
        for (uint32_t j = 0; j < snap->segment_count; j++) {
            if (snap->segments[j] && snap->segments[j]->data)
                inv->total_data_referenced += snap->segments[j]->data_len;
        }
        inv->snapshot_count++;
    }
    return TM_OK;
}

tm_status_t tm_session_gc_snapshots(tm_session_t *sess, uint32_t max_age)
{
    if (!sess) return TM_ERR_BAD_STATE;
    for (size_t i = 0; i < TM_MAX_SNAPSHOTS; i++) {
        tm_snapshot_t *snap = sess->snapshots[i];
        if (!snap || !snap->active) continue;
        if (sess->clock.current_epoch - snap->epoch_captured >= max_age) {
            tm_snapshot_release(snap);
            sess->snapshots[i] = NULL;
        }
    }
    return TM_OK;
}

tm_status_t tm_session_snapshot_validate(const tm_session_t *sess,
                                           uint32_t snapshot_id,
                                           bool *out_valid)
{
    if (!sess || !out_valid) return TM_ERR_BAD_STATE;
    *out_valid = false;
    for (size_t i = 0; i < TM_MAX_SNAPSHOTS; i++) {
        const tm_snapshot_t *snap = sess->snapshots[i];
        if (!snap || snap->snapshot_id != snapshot_id) continue;
        *out_valid = snap->active;
        for (uint32_t j = 0; j < snap->segment_count; j++) {
            if (!snap->segments[j]) { *out_valid = false; break; }
        }
        return TM_OK;
    }
    return TM_ERR_NOT_FOUND;
}
