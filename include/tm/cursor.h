#ifndef TM_CURSOR_H
#define TM_CURSOR_H
#include "tm/types.h"
#include "tm/error.h"
#include "tm/segment.h"

typedef struct {
    uint32_t      cursor_id;
    uint32_t      bound_segment_id;
    tm_segment_t *bound_segment;     /* raw pointer — may become stale */
    void         *bound_catalog_entry; /* catalog entry pointer */
    uint32_t      position;
    uint32_t      bind_generation;
    uint32_t      epoch_opened;
    bool          active;
    bool          bound;
} tm_cursor_t;

tm_cursor_t *tm_cursor_create(uint32_t cursor_id, uint32_t epoch);
void         tm_cursor_destroy(tm_cursor_t *cursor);
tm_status_t  tm_cursor_bind_segment(tm_cursor_t *cursor, tm_segment_t *seg,
                                      uint32_t gen);
tm_status_t  tm_cursor_read_at(tm_cursor_t *cursor, uint8_t *dst,
                                 size_t dst_cap, size_t *out_len);
tm_status_t  tm_cursor_advance(tm_cursor_t *cursor, uint32_t amount);
tm_status_t  tm_cursor_seek(tm_cursor_t *cursor, uint32_t position);
#endif
