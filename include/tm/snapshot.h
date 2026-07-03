#ifndef TM_SNAPSHOT_H
#define TM_SNAPSHOT_H
#include "tm/types.h"
#include "tm/error.h"
#include "tm/segment.h"

typedef struct {
    uint32_t      snapshot_id;
    uint32_t      epoch_captured;
    uint32_t      segment_count;
    tm_segment_t *segments[TM_MAX_SEGMENTS]; /* raw pointers at capture time */
    uint32_t      segment_ids[TM_MAX_SEGMENTS];
    bool          active;
} tm_snapshot_t;

tm_snapshot_t *tm_snapshot_capture(uint32_t snap_id, uint32_t epoch,
                                     void *segment_table);
void           tm_snapshot_release(tm_snapshot_t *snap);
tm_status_t    tm_snapshot_materialize(tm_snapshot_t *snap,
                                        uint32_t segment_id,
                                        uint8_t *dst, size_t dst_cap,
                                        size_t *out_len);
#endif
