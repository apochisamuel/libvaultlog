#include "tm/session.h"
#include <string.h>

typedef struct {
    uint32_t  total;
    uint32_t  valid;
    uint32_t  pinned;
    uint32_t  orphaned;
    uint32_t  high_access;
    uint32_t  low_access;
    double    pressure;
    bool      needs_eviction;
} tm_session_catalog_report_t;

tm_status_t tm_session_catalog_report(const tm_session_t *sess,
                                         tm_session_catalog_report_t *r)
{
    if (!sess || !r) return TM_ERR_BAD_STATE;
    memset(r, 0, sizeof(*r));
    r->total = (uint32_t)sess->catalog.capacity;
    for (size_t i = 0; i < sess->catalog.capacity; i++) {
        const tm_catalog_entry_t *ce = &sess->catalog.entries[i];
        if (!ce->valid) continue;
        r->valid++;
        if (ce->pinned) r->pinned++;
        if (ce->access_count > 10) r->high_access++;
        else r->low_access++;
        bool seg_found = false;
        for (size_t j = 0; j < TM_MAX_SEGMENTS; j++) {
            const tm_segment_t *s = sess->segments.segments[j];
            if (s && s->active && s->segment_id == ce->segment_id) {
                seg_found = true; break;
            }
        }
        if (!seg_found) r->orphaned++;
    }
    if (r->total > 0) r->pressure = (double)r->valid / r->total;
    r->needs_eviction = (r->valid >= sess->catalog.eviction_threshold);
    return TM_OK;
}

tm_status_t tm_session_catalog_gc(tm_session_t *sess)
{
    if (!sess) return TM_ERR_BAD_STATE;
    for (size_t i = 0; i < sess->catalog.capacity; i++) {
        tm_catalog_entry_t *ce = &sess->catalog.entries[i];
        if (!ce->valid) continue;
        bool found = false;
        for (size_t j = 0; j < TM_MAX_SEGMENTS; j++) {
            if (sess->segments.segments[j] &&
                sess->segments.segments[j]->active &&
                sess->segments.segments[j]->segment_id == ce->segment_id) {
                found = true; break;
            }
        }
        if (!found) {
            ce->valid = false;
            ce->segment_ref = NULL;
            sess->catalog.count--;
        }
    }
    return TM_OK;
}

tm_status_t tm_session_catalog_register_all(tm_session_t *sess)
{
    if (!sess) return TM_ERR_BAD_STATE;
    for (size_t i = 0; i < TM_MAX_SEGMENTS; i++) {
        tm_segment_t *s = sess->segments.segments[i];
        if (!s || !s->active) continue;
        tm_catalog_entry_t *existing = tm_catalog_lookup(&sess->catalog, s->segment_id);
        if (!existing) {
            tm_catalog_register(&sess->catalog, s->segment_id,
                                  s, sess->clock.current_epoch);
        }
    }
    return TM_OK;
}

tm_status_t tm_session_catalog_pin_active_cursors(tm_session_t *sess)
{
    if (!sess) return TM_ERR_BAD_STATE;
    for (size_t i = 0; i < TM_MAX_CURSORS; i++) {
        const tm_cursor_t *c = sess->cursors[i];
        if (!c || !c->active || !c->bound) continue;
        tm_catalog_pin(&sess->catalog, c->bound_segment_id);
    }
    return TM_OK;
}

tm_status_t tm_session_catalog_unpin_all(tm_session_t *sess)
{
    if (!sess) return TM_ERR_BAD_STATE;
    for (size_t i = 0; i < sess->catalog.capacity; i++) {
        if (sess->catalog.entries[i].valid)
            sess->catalog.entries[i].pinned = false;
    }
    return TM_OK;
}
