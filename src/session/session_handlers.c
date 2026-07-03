#include "tm/session.h"
#include "tm/wire.h"
#include <string.h>
#include <stdlib.h>

/*
 * Extended session frame handlers for lease chains,
 * catalog management, index operations, and snapshot lifecycle.
 */

tm_status_t tm_session_handle_lease_chain_link(tm_session_t *sess,
                                                  const tm_frame_t *frame)
{
    if (!sess || !sess->active) return TM_ERR_NOT_ACTIVE;
    uint32_t lease_a_id = frame->header.channel_id;
    uint32_t lease_b_id = frame->header.route_id;

    tm_lease_t *a = tm_lease_lookup(&sess->leases, lease_a_id);
    tm_lease_t *b = tm_lease_lookup(&sess->leases, lease_b_id);
    if (!a || !b) return TM_ERR_NOT_FOUND;

    return tm_lease_chain_link(a, b);
}

tm_status_t tm_session_handle_lease_chain_walk(tm_session_t *sess,
                                                  const tm_frame_t *frame)
{
    if (!sess || !sess->active) return TM_ERR_NOT_ACTIVE;
    uint32_t lease_id = frame->header.route_id;

    tm_lease_t *start = tm_lease_lookup(&sess->leases, lease_id);
    if (!start) return TM_ERR_NOT_FOUND;

    uint8_t walk_buf[512];
    size_t walk_len = 0;
    return tm_lease_chain_walk(start, walk_buf, sizeof(walk_buf), &walk_len);
}

tm_status_t tm_session_handle_snapshot_materialize(tm_session_t *sess,
                                                      const tm_frame_t *frame)
{
    if (!sess || !sess->active) return TM_ERR_NOT_ACTIVE;
    uint32_t snap_id = frame->header.channel_id;
    uint32_t seg_id = frame->header.route_id;

    for (size_t i = 0; i < TM_MAX_SNAPSHOTS; i++) {
        tm_snapshot_t *snap = sess->snapshots[i];
        if (!snap || snap->snapshot_id != snap_id) continue;

        uint8_t mat_buf[4096];
        size_t mat_len = 0;
        return tm_snapshot_materialize(snap, seg_id,
                                         mat_buf, sizeof(mat_buf), &mat_len);
    }
    return TM_ERR_NOT_FOUND;
}

tm_status_t tm_session_handle_cursor_advance(tm_session_t *sess,
                                                const tm_frame_t *frame)
{
    if (!sess || !sess->active) return TM_ERR_NOT_ACTIVE;
    uint32_t cursor_id = frame->header.channel_id;
    uint32_t amount = 16;
    if (frame->subrecord_count > 0)
        amount = frame->subrecords[0].generation;
    if (amount == 0) amount = 16;

    for (size_t i = 0; i < TM_MAX_CURSORS; i++) {
        if (sess->cursors[i] && sess->cursors[i]->cursor_id == cursor_id) {
            /*
             * this reads through a freed catalog entry to find the
             * segment. The catalog_entry->segment_ref may be garbage.
             */
            tm_cursor_t *c = sess->cursors[i];
            if (c->bound_catalog_entry) {
                tm_catalog_entry_t *ce =
                    (tm_catalog_entry_t *)c->bound_catalog_entry;
                /* Read through catalog entry to verify segment */
                if (ce->valid && ce->segment_ref) {
                    tm_segment_t *seg = (tm_segment_t *)ce->segment_ref;
                    c->bound_segment = seg;
                }
            }
            return tm_cursor_advance(c, amount);
        }
    }
    return TM_ERR_NOT_FOUND;
}

tm_status_t tm_session_handle_cursor_bind_catalog(tm_session_t *sess,
                                                     const tm_frame_t *frame)
{
    if (!sess || !sess->active) return TM_ERR_NOT_ACTIVE;
    uint32_t cursor_id = frame->header.channel_id;
    uint32_t seg_id = frame->header.route_id;

    tm_cursor_t *c = NULL;
    for (size_t i = 0; i < TM_MAX_CURSORS; i++) {
        if (sess->cursors[i] && sess->cursors[i]->cursor_id == cursor_id) {
            c = sess->cursors[i];
            break;
        }
    }
    if (!c) return TM_ERR_BAD_CURSOR;

    tm_catalog_entry_t *ce = tm_catalog_lookup(&sess->catalog, seg_id);
    if (!ce || !ce->valid) return TM_ERR_NOT_FOUND;

    /* Cache the catalog entry pointer in cursor */
    c->bound_catalog_entry = ce;
    c->bound_segment = (tm_segment_t *)ce->segment_ref;
    c->bound_segment_id = seg_id;
    c->bind_generation = ce->generation;
    c->position = 0;
    c->bound = true;

    return TM_OK;
}

tm_status_t tm_session_handle_catalog_pressure(tm_session_t *sess,
                                                  const tm_frame_t *frame)
{
    if (!sess || !sess->active) return TM_ERR_NOT_ACTIVE;
    (void)frame;

    /* Register new dummy segments to fill catalog */
    while (sess->catalog.count < sess->catalog.eviction_threshold + 2) {
        uint32_t slab_slot;
        tm_segment_t *seg = tm_segment_create(&sess->segments,
                                                 sess->clock.current_epoch,
                                                 &sess->slab, &slab_slot);
        if (!seg) break;
        tm_catalog_register(&sess->catalog, seg->segment_id,
                              seg, sess->clock.current_epoch);
        /* Access new entries to boost their LRU score */
        tm_catalog_access(&sess->catalog, seg->segment_id);
        tm_catalog_access(&sess->catalog, seg->segment_id);
        tm_catalog_access(&sess->catalog, seg->segment_id);
    }

    /* Trigger eviction — evicts entry with LOWEST access count
     * which is the one the cursor is bound to (0 accesses) */
    if (sess->catalog.count >= sess->catalog.eviction_threshold) {
        tm_catalog_evict_lru(&sess->catalog);
    }

    return TM_OK;
}

tm_status_t tm_session_handle_cursor_read_segment(tm_session_t *sess,
                                                     const tm_frame_t *frame)
{
    if (!sess || !sess->active) return TM_ERR_NOT_ACTIVE;
    uint32_t cursor_id = frame->header.channel_id;

    for (size_t i = 0; i < TM_MAX_CURSORS; i++) {
        if (sess->cursors[i] && sess->cursors[i]->cursor_id == cursor_id) {
            uint8_t buf[1024];
            size_t len = 0;
            return tm_cursor_read_at(sess->cursors[i], buf, sizeof(buf), &len);
        }
    }
    return TM_ERR_NOT_FOUND;
}

/* Index rebuild handler */
tm_status_t tm_session_handle_index_rebuild(tm_session_t *sess,
                                               const tm_frame_t *frame)
{
    if (!sess || !sess->active) return TM_ERR_NOT_ACTIVE;
    (void)frame;
    
    /* Rebuild index with capacity based on current segment count */
    uint32_t seg_count = 0;
    for (size_t i = 0; i < TM_MAX_SEGMENTS; i++) {
        if (sess->segments.segments[i] && sess->segments.segments[i]->active)
            seg_count++;
    }
    
    size_t new_cap = (seg_count > 0) ? seg_count * 2 : 4;
    if (new_cap < 4) new_cap = 4;
    
    return tm_index_rebuild(&sess->index, new_cap);
}

/* Watermark set handler */
tm_status_t tm_session_handle_watermark_set(tm_session_t *sess,
                                               const tm_frame_t *frame)
{
    if (!sess || !sess->active) return TM_ERR_NOT_ACTIVE;
    uint32_t target = sess->clock.current_epoch;
    if (frame->subrecord_count > 0)
        target = frame->subrecords[0].local_epoch;
    
    return tm_tidemark_advance(&sess->tidemark, target);
}

/* Snapshot release handler */
tm_status_t tm_session_handle_snapshot_release(tm_session_t *sess,
                                                  const tm_frame_t *frame)
{
    if (!sess || !sess->active) return TM_ERR_NOT_ACTIVE;
    uint32_t snap_id = frame->header.channel_id;
    
    for (size_t i = 0; i < TM_MAX_SNAPSHOTS; i++) {
        if (sess->snapshots[i] && sess->snapshots[i]->snapshot_id == snap_id) {
            tm_snapshot_release(sess->snapshots[i]);
            sess->snapshots[i] = NULL;
            return TM_OK;
        }
    }
    return TM_ERR_NOT_FOUND;
}

/* Segment destroy handler */
tm_status_t tm_session_handle_segment_destroy(tm_session_t *sess,
                                                 const tm_frame_t *frame)
{
    if (!sess || !sess->active) return TM_ERR_NOT_ACTIVE;
    uint32_t seg_id = frame->header.route_id;
    
    tm_segment_t *seg = tm_segment_lookup(&sess->segments, seg_id);
    if (!seg) return TM_ERR_NOT_FOUND;
    
    tm_catalog_remove(&sess->catalog, seg_id);
    tm_index_remove_segment(&sess->index, seg_id);
    return tm_segment_destroy(&sess->segments, seg, &sess->slab);
}
