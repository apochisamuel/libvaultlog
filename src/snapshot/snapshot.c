#include "tm/snapshot.h"
#include <stdlib.h>
#include <string.h>

tm_snapshot_t *tm_snapshot_capture(uint32_t snap_id, uint32_t epoch,
                                     void *seg_table_ptr)
{
    if (!seg_table_ptr) return NULL;
    tm_segment_table_t *table = (tm_segment_table_t *)seg_table_ptr;

    tm_snapshot_t *snap = calloc(1, sizeof(*snap));
    if (!snap) return NULL;

    snap->snapshot_id = snap_id;
    snap->epoch_captured = epoch;
    snap->active = true;

    for (size_t i = 0; i < TM_MAX_SEGMENTS; i++) {
        tm_segment_t *seg = table->segments[i];
        if (seg && seg->active) {
            snap->segments[snap->segment_count] = seg;
            snap->segment_ids[snap->segment_count] = seg->segment_id;
            snap->segment_count++;
        }
    }

    return snap;
}

void tm_snapshot_release(tm_snapshot_t *snap)
{
    if (snap) {
        snap->active = false;
        free(snap);
    }
}

tm_status_t tm_snapshot_materialize(tm_snapshot_t *snap, uint32_t segment_id,
                                     uint8_t *dst, size_t dst_cap,
                                     size_t *out_len)
{
    if (!snap || !snap->active || !dst || !out_len) return TM_ERR_BAD_STATE;

    for (uint32_t i = 0; i < snap->segment_count; i++) {
        if (snap->segment_ids[i] == segment_id) {
            tm_segment_t *seg = snap->segments[i];
            if (!seg || !seg->data) return TM_ERR_NOT_FOUND;
            size_t len = (seg->data_len > dst_cap) ? dst_cap : seg->data_len;
            memcpy(dst, seg->data, len);
            *out_len = len;
            return TM_OK;
        }
    }

    return TM_ERR_NOT_FOUND;
}
