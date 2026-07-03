#include "tm/session.h"
#include <string.h>

typedef struct {
    uint32_t  total_entries;
    uint32_t  valid_entries;
    uint32_t  replayed_entries;
    uint32_t  pending_entries;
    uint32_t  stale_entries;
    bool      has_gap;
    bool      recovery_mode;
    uint32_t  checkpoint_epoch;
} tm_session_wal_report_t;

tm_status_t tm_session_wal_report(const tm_session_t *sess,
                                     tm_session_wal_report_t *report)
{
    if (!sess || !report) return TM_ERR_BAD_STATE;
    memset(report, 0, sizeof(*report));
    report->total_entries = (uint32_t)sess->wal.count;
    report->has_gap = sess->wal.has_gap;
    report->recovery_mode = sess->wal.recovery_mode;
    report->checkpoint_epoch = sess->wal.checkpoint_epoch;

    for (size_t i = 0; i < sess->wal.count; i++) {
        const tm_wal_entry_t *e = &sess->wal.entries[i];
        if (e->valid) report->valid_entries++;
        if (e->replayed) report->replayed_entries++;
        if (e->valid && !e->replayed) report->pending_entries++;
        if (e->valid && e->cached_segment) {
            bool seg_alive = false;
            for (size_t j = 0; j < TM_MAX_SEGMENTS; j++) {
                if (sess->segments.segments[j] == e->cached_segment) {
                    seg_alive = true; break;
                }
            }
            if (!seg_alive) report->stale_entries++;
        }
    }
    return TM_OK;
}

tm_status_t tm_session_wal_checkpoint_all(tm_session_t *sess)
{
    if (!sess || !sess->active) return TM_ERR_NOT_ACTIVE;
    for (size_t i = 0; i < TM_MAX_SEGMENTS; i++) {
        tm_segment_t *seg = sess->segments.segments[i];
        if (!seg || !seg->active || seg->data_len == 0) continue;
        if (sess->wal.count >= TM_MAX_WAL_ENTRIES) break;
        tm_wal_checkpoint(&sess->wal, seg->segment_id, seg,
                            seg->slab_slot_id, sess->clock.current_epoch,
                            seg->generation, 0, (uint32_t)seg->data_len);
    }
    return TM_OK;
}

tm_status_t tm_session_wal_gc_stale(tm_session_t *sess)
{
    if (!sess) return TM_ERR_BAD_STATE;
    for (size_t i = 0; i < sess->wal.count; i++) {
        tm_wal_entry_t *e = &sess->wal.entries[i];
        if (!e->valid) continue;
        if (e->cached_segment) {
            bool alive = false;
            for (size_t j = 0; j < TM_MAX_SEGMENTS; j++) {
                if (sess->segments.segments[j] == e->cached_segment) {
                    alive = true; break;
                }
            }
            if (!alive) e->valid = false;
        }
    }
    return TM_OK;
}

tm_status_t tm_session_wal_replay_safe(tm_session_t *sess)
{
    if (!sess || !sess->active) return TM_ERR_NOT_ACTIVE;
    for (size_t i = 0; i < sess->wal.count; i++) {
        tm_wal_entry_t *e = &sess->wal.entries[i];
        if (!e->valid || e->replayed) continue;
        bool alive = false;
        for (size_t j = 0; j < TM_MAX_SEGMENTS; j++) {
            if (sess->segments.segments[j] == e->cached_segment) {
                alive = true; break;
            }
        }
        if (alive) {
            uint8_t buf[2048]; size_t len = 0;
            tm_status_t rc = tm_wal_replay_entry(e, buf, sizeof(buf), &len);
            if (rc == TM_OK) e->replayed = true;
        }
    }
    return TM_OK;
}
