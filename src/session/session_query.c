#include "tm/session.h"
#include <string.h>

/*
 * Session query engine: processes time-series queries by
 * coordinating cursor, index, and snapshot subsystems.
 */

typedef struct {
    uint64_t  start_timestamp;
    uint64_t  end_timestamp;
    uint32_t  segment_filter;   /* 0 = all segments */
    uint32_t  max_results;
    bool      use_snapshot;
    uint32_t  snapshot_id;
} tm_query_params_t;

typedef struct {
    uint8_t  *result_data;
    size_t    result_len;
    size_t    result_cap;
    uint32_t  rows_returned;
    uint32_t  segments_scanned;
    uint32_t  index_lookups;
    bool      truncated;
} tm_query_result_t;

tm_status_t tm_query_init_result(tm_query_result_t *result, size_t capacity)
{
    if (!result) return TM_ERR_BAD_STATE;
    memset(result, 0, sizeof(*result));
    result->result_data = calloc(1, capacity);
    if (!result->result_data) return TM_ERR_ALLOC;
    result->result_cap = capacity;
    return TM_OK;
}

void tm_query_free_result(tm_query_result_t *result)
{
    if (result) {
        free(result->result_data);
        memset(result, 0, sizeof(*result));
    }
}

tm_status_t tm_query_scan_segments(tm_session_t *sess,
                                      const tm_query_params_t *params,
                                      tm_query_result_t *result)
{
    if (!sess || !params || !result) return TM_ERR_BAD_STATE;

    for (size_t i = 0; i < TM_MAX_SEGMENTS; i++) {
        tm_segment_t *seg = sess->segments.segments[i];
        if (!seg || !seg->active) continue;

        if (params->segment_filter != 0 &&
            seg->segment_id != params->segment_filter)
            continue;

        result->segments_scanned++;

        /* Copy segment data to result buffer */
        size_t remaining = result->result_cap - result->result_len;
        if (remaining == 0) {
            result->truncated = true;
            break;
        }

        size_t copy = (seg->data_len > remaining) ? remaining : seg->data_len;
        if (seg->data && copy > 0) {
            memcpy(result->result_data + result->result_len,
                   seg->data, copy);
            result->result_len += copy;
            result->rows_returned++;
        }
    }

    return TM_OK;
}

tm_status_t tm_query_scan_with_index(tm_session_t *sess,
                                        uint64_t start_ts, uint64_t end_ts,
                                        tm_query_result_t *result)
{
    if (!sess || !result) return TM_ERR_BAD_STATE;

    /* Scan index for matching entries */
    for (size_t i = 0; i < sess->index.capacity; i++) {
        tm_index_entry_t *ie = &sess->index.entries[i];
        if (!ie->valid) continue;
        if (ie->timestamp < start_ts || ie->timestamp > end_ts) continue;

        result->index_lookups++;

        /* Find corresponding segment */
        tm_segment_t *seg = tm_segment_lookup(&sess->segments, ie->segment_id);
        if (!seg || !seg->data) continue;

        /* Read data at index offset */
        if (ie->offset < seg->data_len) {
            size_t remaining = result->result_cap - result->result_len;
            size_t avail = seg->data_len - ie->offset;
            size_t copy = (avail > remaining) ? remaining : avail;
            if (copy > 0) {
                memcpy(result->result_data + result->result_len,
                       seg->data + ie->offset, copy);
                result->result_len += copy;
                result->rows_returned++;
            }
        }
    }

    return TM_OK;
}

tm_status_t tm_query_via_snapshot(tm_session_t *sess,
                                    uint32_t snapshot_id,
                                    uint32_t segment_id,
                                    tm_query_result_t *result)
{
    if (!sess || !result) return TM_ERR_BAD_STATE;

    for (size_t i = 0; i < TM_MAX_SNAPSHOTS; i++) {
        tm_snapshot_t *snap = sess->snapshots[i];
        if (!snap || snap->snapshot_id != snapshot_id) continue;

        size_t remaining = result->result_cap - result->result_len;
        size_t out_len = 0;
        tm_status_t rc = tm_snapshot_materialize(snap, segment_id,
                                                    result->result_data + result->result_len,
                                                    remaining, &out_len);
        if (rc == TM_OK) {
            result->result_len += out_len;
            result->rows_returned++;
        }
        return rc;
    }

    return TM_ERR_NOT_FOUND;
}
