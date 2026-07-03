#include "tm/cursor.h"
#include <stdlib.h>
#include <string.h>

tm_cursor_t *tm_cursor_create(uint32_t cursor_id, uint32_t epoch)
{
    tm_cursor_t *c = calloc(1, sizeof(*c));
    if (!c) return NULL;
    c->cursor_id = cursor_id;
    c->epoch_opened = epoch;
    c->active = true;
    return c;
}

void tm_cursor_destroy(tm_cursor_t *cursor)
{
    free(cursor);
}

tm_status_t tm_cursor_bind_segment(tm_cursor_t *cursor, tm_segment_t *seg,
                                     uint32_t gen)
{
    if (!cursor || !cursor->active || !seg) return TM_ERR_BAD_STATE;
    cursor->bound_segment = seg;
    cursor->bound_segment_id = seg->segment_id;
    cursor->bind_generation = gen;
    cursor->position = 0;
    cursor->bound = true;
    return TM_OK;
}

tm_status_t tm_cursor_read_at(tm_cursor_t *cursor, uint8_t *dst,
                                size_t dst_cap, size_t *out_len)
{
    if (!cursor || !cursor->active || !cursor->bound)
        return TM_ERR_BAD_STATE;
    if (!dst || !out_len) return TM_ERR_BAD_STATE;

    tm_segment_t *seg = cursor->bound_segment;
    if (!seg) return TM_ERR_NOT_FOUND;

    if (cursor->position >= seg->data_len)
        return TM_ERR_OVERFLOW;

    size_t avail = seg->data_len - cursor->position;
    size_t copy_len = (avail > dst_cap) ? dst_cap : avail;
    memcpy(dst, seg->data + cursor->position, copy_len);
    *out_len = copy_len;
    return TM_OK;
}

tm_status_t tm_cursor_advance(tm_cursor_t *cursor, uint32_t amount)
{
    if (!cursor || !cursor->active) return TM_ERR_BAD_STATE;

    cursor->position += amount;

    /* Clamp to segment bounds */
    if (cursor->bound && cursor->bound_segment) {
        tm_segment_t *seg = cursor->bound_segment;
        if (cursor->position > seg->data_len)
            cursor->position = (uint32_t)seg->data_len;
    }

    return TM_OK;
}

tm_status_t tm_cursor_seek(tm_cursor_t *cursor, uint32_t position)
{
    if (!cursor || !cursor->active) return TM_ERR_BAD_STATE;

    /* Validate position against segment bounds */
    if (cursor->bound && cursor->bound_segment) {
        tm_segment_t *seg = cursor->bound_segment;
        if (position > seg->data_len)
            position = (uint32_t)seg->data_len;
    }

    cursor->position = position;
    return TM_OK;
}
