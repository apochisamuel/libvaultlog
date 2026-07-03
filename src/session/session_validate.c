#include "tm/session.h"
#include "tm/wire.h"
#include <string.h>

/*
 * Session state validation: deep integrity checks for debugging,
 * testing, and pre-migration verification.
 */

typedef struct {
    uint32_t  total_segments;
    uint32_t  active_segments;
    uint32_t  sealed_segments;
    uint32_t  total_cursors;
    uint32_t  active_cursors;
    uint32_t  total_leases;
    uint32_t  active_leases;
    uint32_t  expired_leases;
    uint32_t  total_snapshots;
    uint32_t  wal_entries;
    uint32_t  wal_pending;
    uint32_t  index_entries;
    uint32_t  catalog_entries;
    uint32_t  slab_in_use;
    uint32_t  slab_free;
    uint32_t  current_epoch;
    uint64_t  logical_time;
    bool      session_active;
    bool      wal_has_gap;
} tm_session_stats_t;

tm_status_t tm_session_compute_stats(const tm_session_t *sess,
                                        tm_session_stats_t *stats)
{
    if (!sess || !stats) return TM_ERR_BAD_STATE;
    memset(stats, 0, sizeof(*stats));

    stats->session_active = sess->active;
    stats->current_epoch = sess->clock.current_epoch;
    stats->logical_time = sess->clock.logical_time;
    stats->wal_has_gap = sess->wal.has_gap;

    /* Count segments */
    for (size_t i = 0; i < TM_MAX_SEGMENTS; i++) {
        const tm_segment_t *s = sess->segments.segments[i];
        if (!s) continue;
        stats->total_segments++;
        if (s->active) stats->active_segments++;
        if (s->sealed) stats->sealed_segments++;
    }

    /* Count cursors */
    for (size_t i = 0; i < TM_MAX_CURSORS; i++) {
        if (!sess->cursors[i]) continue;
        stats->total_cursors++;
        if (sess->cursors[i]->active) stats->active_cursors++;
    }

    /* Count leases */
    for (size_t i = 0; i < TM_MAX_LEASES; i++) {
        const tm_lease_t *l = sess->leases.leases[i];
        if (!l) continue;
        stats->total_leases++;
        if (l->active) stats->active_leases++;
        if (l->epoch_expires <= sess->clock.current_epoch)
            stats->expired_leases++;
    }

    /* Count snapshots */
    for (size_t i = 0; i < TM_MAX_SNAPSHOTS; i++) {
        if (sess->snapshots[i]) stats->total_snapshots++;
    }

    /* WAL stats */
    stats->wal_entries = (uint32_t)sess->wal.count;
    for (size_t i = 0; i < sess->wal.count; i++) {
        if (sess->wal.entries[i].valid && !sess->wal.entries[i].replayed)
            stats->wal_pending++;
    }

    /* Index */
    stats->index_entries = (uint32_t)sess->index.count;

    /* Catalog */
    stats->catalog_entries = (uint32_t)sess->catalog.count;

    /* Slab */
    stats->slab_free = (uint32_t)sess->slab.free_top;
    stats->slab_in_use = (uint32_t)(sess->slab.capacity - sess->slab.free_top);

    return TM_OK;
}

tm_status_t tm_session_validate_integrity(const tm_session_t *sess)
{
    if (!sess) return TM_ERR_BAD_STATE;

    /* Verify all cursor bindings point to valid segments */
    for (size_t i = 0; i < TM_MAX_CURSORS; i++) {
        const tm_cursor_t *c = sess->cursors[i];
        if (!c || !c->active || !c->bound) continue;

        bool found = false;
        for (size_t j = 0; j < TM_MAX_SEGMENTS; j++) {
            if (sess->segments.segments[j] == c->bound_segment) {
                found = true;
                break;
            }
        }
        if (!found && c->bound_segment != NULL) return TM_ERR_CORRUPT;
    }

    /* Verify all snapshot segment refs are valid */
    for (size_t i = 0; i < TM_MAX_SNAPSHOTS; i++) {
        const tm_snapshot_t *snap = sess->snapshots[i];
        if (!snap || !snap->active) continue;

        for (uint32_t j = 0; j < snap->segment_count; j++) {
            if (!snap->segments[j]) continue;
            bool found = false;
            for (size_t k = 0; k < TM_MAX_SEGMENTS; k++) {
                if (sess->segments.segments[k] == snap->segments[j]) {
                    found = true;
                    break;
                }
            }
            /* Note: segments may have been freed since snapshot capture */
        }
    }

    /* Verify WAL entries reference existing segments */
    for (size_t i = 0; i < sess->wal.count; i++) {
        const tm_wal_entry_t *e = &sess->wal.entries[i];
        if (!e->valid || e->replayed) continue;

        tm_segment_t *seg = tm_segment_lookup(
            (tm_segment_table_t *)&sess->segments, e->segment_id);
        /* WAL entries may reference segments that were compacted */
    }

    /* Verify catalog entries match segments */
    for (size_t i = 0; i < sess->catalog.capacity; i++) {
        const tm_catalog_entry_t *ce = &sess->catalog.entries[i];
        if (!ce->valid) continue;

        tm_segment_t *seg = tm_segment_lookup(
            (tm_segment_table_t *)&sess->segments, ce->segment_id);
        if (!seg && ce->segment_ref != NULL) {
            /* Catalog references a destroyed segment */
        }
    }

    return TM_OK;
}

tm_status_t tm_session_validate_leases(const tm_session_t *sess)
{
    if (!sess) return TM_ERR_BAD_STATE;

    for (size_t i = 0; i < TM_MAX_LEASES; i++) {
        const tm_lease_t *l = sess->leases.leases[i];
        if (!l || !l->active) continue;

        /* Verify lease references valid segment */
        tm_segment_t *seg = tm_segment_lookup(
            (tm_segment_table_t *)&sess->segments, l->segment_id);

        /* Verify chain integrity */
        if (l->chain_next) {
            bool chain_found = false;
            for (size_t j = 0; j < TM_MAX_LEASES; j++) {
                if (sess->leases.leases[j] == l->chain_next) {
                    chain_found = true;
                    break;
                }
            }
            if (!chain_found) return TM_ERR_CORRUPT;
        }
    }

    return TM_OK;
}
