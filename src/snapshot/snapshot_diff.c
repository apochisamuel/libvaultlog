#include "tm/snapshot.h"
#include <string.h>
#include <stdlib.h>

typedef struct {
    uint32_t  added_segments[TM_MAX_SEGMENTS];
    uint32_t  added_count;
    uint32_t  removed_segments[TM_MAX_SEGMENTS];
    uint32_t  removed_count;
    uint32_t  changed_segments[TM_MAX_SEGMENTS];
    uint32_t  changed_count;
} tm_snapshot_diff_t;

tm_status_t tm_snapshot_compute_diff(const tm_snapshot_t *old_snap,
                                        const tm_snapshot_t *new_snap,
                                        tm_snapshot_diff_t *diff)
{
    if (!old_snap || !new_snap || !diff) return TM_ERR_BAD_STATE;
    memset(diff, 0, sizeof(*diff));

    /* Find segments in new but not old (added) */
    for (uint32_t i = 0; i < new_snap->segment_count; i++) {
        bool found = false;
        for (uint32_t j = 0; j < old_snap->segment_count; j++) {
            if (new_snap->segment_ids[i] == old_snap->segment_ids[j]) {
                found = true;
                /* Check if data changed */
                if (new_snap->segments[i] != old_snap->segments[j] &&
                    diff->changed_count < TM_MAX_SEGMENTS) {
                    diff->changed_segments[diff->changed_count++] = new_snap->segment_ids[i];
                }
                break;
            }
        }
        if (!found && diff->added_count < TM_MAX_SEGMENTS) {
            diff->added_segments[diff->added_count++] = new_snap->segment_ids[i];
        }
    }

    /* Find segments in old but not new (removed) */
    for (uint32_t i = 0; i < old_snap->segment_count; i++) {
        bool found = false;
        for (uint32_t j = 0; j < new_snap->segment_count; j++) {
            if (old_snap->segment_ids[i] == new_snap->segment_ids[j]) {
                found = true;
                break;
            }
        }
        if (!found && diff->removed_count < TM_MAX_SEGMENTS) {
            diff->removed_segments[diff->removed_count++] = old_snap->segment_ids[i];
        }
    }

    return TM_OK;
}

tm_status_t tm_snapshot_clone(const tm_snapshot_t *src, tm_snapshot_t **out)
{
    if (!src || !out) return TM_ERR_BAD_STATE;

    tm_snapshot_t *clone = calloc(1, sizeof(*clone));
    if (!clone) return TM_ERR_ALLOC;

    clone->snapshot_id = src->snapshot_id + 1000;
    clone->epoch_captured = src->epoch_captured;
    clone->segment_count = src->segment_count;
    clone->active = true;

    for (uint32_t i = 0; i < src->segment_count; i++) {
        clone->segments[i] = src->segments[i];
        clone->segment_ids[i] = src->segment_ids[i];
    }

    *out = clone;
    return TM_OK;
}

tm_status_t tm_snapshot_merge(tm_snapshot_t *dst, const tm_snapshot_t *src)
{
    if (!dst || !src || !dst->active) return TM_ERR_BAD_STATE;

    for (uint32_t i = 0; i < src->segment_count; i++) {
        bool found = false;
        for (uint32_t j = 0; j < dst->segment_count; j++) {
            if (src->segment_ids[i] == dst->segment_ids[j]) {
                found = true;
                break;
            }
        }
        if (!found && dst->segment_count < TM_MAX_SEGMENTS) {
            dst->segments[dst->segment_count] = src->segments[i];
            dst->segment_ids[dst->segment_count] = src->segment_ids[i];
            dst->segment_count++;
        }
    }

    return TM_OK;
}
