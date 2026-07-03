#include "tm/cursor.h"
#include "tm/catalog.h"
#include <string.h>

/*
 * Cursor iteration and scan operations.
 * Cursors maintain position within a bound segment and support
 * forward iteration, range scans, and filtered reads.
 */

typedef struct {
    uint32_t  start_offset;
    uint32_t  end_offset;
    uint32_t  step_size;
    uint32_t  filter_mask;
    bool      reverse;
} tm_cursor_scan_params_t;

tm_status_t tm_cursor_scan_forward(tm_cursor_t *cursor,
                                     uint32_t step_size,
                                     uint8_t *dst, size_t dst_cap,
                                     size_t *out_len)
{
    if (!cursor || !cursor->active || !cursor->bound)
        return TM_ERR_BAD_STATE;
    if (!dst || !out_len) return TM_ERR_BAD_STATE;

    tm_segment_t *seg = cursor->bound_segment;
    if (!seg || !seg->data) return TM_ERR_NOT_FOUND;

    size_t total_read = 0;
    uint32_t pos = cursor->position;

    while (pos < seg->data_len && total_read + step_size <= dst_cap) {
        size_t avail = seg->data_len - pos;
        size_t chunk = (avail > step_size) ? step_size : avail;
        memcpy(dst + total_read, seg->data + pos, chunk);
        total_read += chunk;
        pos += (uint32_t)chunk;
    }

    cursor->position = pos;
    *out_len = total_read;
    return TM_OK;
}

tm_status_t tm_cursor_scan_range(tm_cursor_t *cursor,
                                    uint32_t start, uint32_t end,
                                    uint8_t *dst, size_t dst_cap,
                                    size_t *out_len)
{
    if (!cursor || !cursor->active || !cursor->bound)
        return TM_ERR_BAD_STATE;

    tm_segment_t *seg = cursor->bound_segment;
    if (!seg || !seg->data) return TM_ERR_NOT_FOUND;

    if (start >= seg->data_len) return TM_ERR_OVERFLOW;
    if (end > seg->data_len) end = (uint32_t)seg->data_len;
    if (start >= end) return TM_ERR_BAD_STATE;

    size_t range_len = end - start;
    size_t copy_len = (range_len > dst_cap) ? dst_cap : range_len;
    memcpy(dst, seg->data + start, copy_len);
    *out_len = copy_len;

    cursor->position = start + (uint32_t)copy_len;
    return TM_OK;
}

tm_status_t tm_cursor_filtered_read(tm_cursor_t *cursor,
                                       uint8_t filter_byte,
                                       uint8_t *dst, size_t dst_cap,
                                       size_t *out_len)
{
    if (!cursor || !cursor->active || !cursor->bound)
        return TM_ERR_BAD_STATE;

    tm_segment_t *seg = cursor->bound_segment;
    if (!seg || !seg->data) return TM_ERR_NOT_FOUND;

    size_t written = 0;
    for (uint32_t i = cursor->position; i < seg->data_len && written < dst_cap; i++) {
        if (seg->data[i] == filter_byte) {
            dst[written++] = seg->data[i];
        }
    }

    *out_len = written;
    return TM_OK;
}

bool tm_cursor_has_more(const tm_cursor_t *cursor)
{
    if (!cursor || !cursor->active || !cursor->bound) return false;
    tm_segment_t *seg = cursor->bound_segment;
    if (!seg || !seg->data) return false;
    return cursor->position < seg->data_len;
}

uint32_t tm_cursor_remaining(const tm_cursor_t *cursor)
{
    if (!cursor || !cursor->active || !cursor->bound) return 0;
    tm_segment_t *seg = cursor->bound_segment;
    if (!seg || !seg->data || cursor->position >= seg->data_len) return 0;
    return (uint32_t)(seg->data_len - cursor->position);
}
