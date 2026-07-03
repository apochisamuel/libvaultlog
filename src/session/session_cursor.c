#include "tm/session.h"
#include <string.h>

typedef struct {
    uint32_t  cursor_ids[TM_MAX_CURSORS];
    uint32_t  bound_segments[TM_MAX_CURSORS];
    uint32_t  positions[TM_MAX_CURSORS];
    uint32_t  active_count;
    uint32_t  bound_count;
} tm_cursor_inventory_t;

tm_status_t tm_session_cursor_inventory(const tm_session_t *sess,
                                          tm_cursor_inventory_t *inv)
{
    if (!sess || !inv) return TM_ERR_BAD_STATE;
    memset(inv, 0, sizeof(*inv));
    for (size_t i = 0; i < TM_MAX_CURSORS; i++) {
        const tm_cursor_t *c = sess->cursors[i];
        if (!c || !c->active) continue;
        uint32_t idx = inv->active_count;
        inv->cursor_ids[idx] = c->cursor_id;
        inv->positions[idx] = c->position;
        if (c->bound) {
            inv->bound_segments[idx] = c->bound_segment_id;
            inv->bound_count++;
        }
        inv->active_count++;
    }
    return TM_OK;
}

tm_status_t tm_session_invalidate_cursors_for_segment(tm_session_t *sess,
                                                         uint32_t segment_id)
{
    if (!sess) return TM_ERR_BAD_STATE;
    for (size_t i = 0; i < TM_MAX_CURSORS; i++) {
        tm_cursor_t *c = sess->cursors[i];
        if (!c || !c->active || !c->bound) continue;
        if (c->bound_segment_id == segment_id) {
            c->bound = false;
            c->bound_segment = NULL;
            c->bound_catalog_entry = NULL;
        }
    }
    return TM_OK;
}

tm_status_t tm_session_close_all_cursors(tm_session_t *sess)
{
    if (!sess) return TM_ERR_BAD_STATE;
    for (size_t i = 0; i < TM_MAX_CURSORS; i++) {
        if (sess->cursors[i]) {
            tm_cursor_destroy(sess->cursors[i]);
            sess->cursors[i] = NULL;
        }
    }
    return TM_OK;
}

tm_status_t tm_session_cursor_rebind_all(tm_session_t *sess)
{
    if (!sess) return TM_ERR_BAD_STATE;
    for (size_t i = 0; i < TM_MAX_CURSORS; i++) {
        tm_cursor_t *c = sess->cursors[i];
        if (!c || !c->active || !c->bound) continue;
        tm_segment_t *seg = tm_segment_lookup(&sess->segments, c->bound_segment_id);
        if (seg) {
            c->bound_segment = seg;
        } else {
            c->bound = false;
            c->bound_segment = NULL;
        }
    }
    return TM_OK;
}
