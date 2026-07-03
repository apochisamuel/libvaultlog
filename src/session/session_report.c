#include "tm/session.h"
#include <string.h>
#include <stdio.h>

typedef struct {
    char     session_status[32];
    uint32_t session_id;
    uint32_t current_epoch;
    uint64_t logical_time;
    uint32_t segment_count;
    uint32_t active_segments;
    uint32_t sealed_segments;
    uint64_t total_data_bytes;
    uint32_t cursor_count;
    uint32_t snapshot_count;
    uint32_t lease_count;
    uint32_t wal_entries;
    uint32_t wal_pending;
    uint32_t index_entries;
    uint32_t catalog_entries;
    uint32_t slab_in_use;
    uint32_t slab_free;
    uint32_t compact_merges;
    uint32_t retention_sweeps;
    uint32_t tidemark_watermark;
} tm_session_full_report_t;

tm_status_t tm_session_full_report(const tm_session_t *sess,
                                      tm_session_full_report_t *r)
{
    if (!sess || !r) return TM_ERR_BAD_STATE;
    memset(r, 0, sizeof(*r));
    if (sess->active) memcpy(r->session_status, "active", 6);
    else memcpy(r->session_status, "inactive", 8);
    r->session_id = sess->session_id;
    r->current_epoch = sess->clock.current_epoch;
    r->logical_time = sess->clock.logical_time;
    for (size_t i = 0; i < TM_MAX_SEGMENTS; i++) {
        const tm_segment_t *s = sess->segments.segments[i];
        if (!s) continue;
        r->segment_count++;
        if (s->active) { r->active_segments++; r->total_data_bytes += s->data_len; }
        if (s->sealed) r->sealed_segments++;
    }
    for (size_t i = 0; i < TM_MAX_CURSORS; i++)
        if (sess->cursors[i] && sess->cursors[i]->active) r->cursor_count++;
    for (size_t i = 0; i < TM_MAX_SNAPSHOTS; i++)
        if (sess->snapshots[i] && sess->snapshots[i]->active) r->snapshot_count++;
    for (size_t i = 0; i < TM_MAX_LEASES; i++)
        if (sess->leases.leases[i] && sess->leases.leases[i]->active) r->lease_count++;
    r->wal_entries = (uint32_t)sess->wal.count;
    for (size_t i = 0; i < sess->wal.count; i++)
        if (sess->wal.entries[i].valid && !sess->wal.entries[i].replayed) r->wal_pending++;
    r->index_entries = (uint32_t)sess->index.count;
    r->catalog_entries = (uint32_t)sess->catalog.count;
    r->slab_in_use = (uint32_t)(sess->slab.capacity - sess->slab.free_top);
    r->slab_free = (uint32_t)sess->slab.free_top;
    r->compact_merges = sess->compact.merges_completed;
    r->retention_sweeps = sess->retention.sweep_count;
    r->tidemark_watermark = sess->tidemark.watermark_epoch;
    return TM_OK;
}

tm_status_t tm_session_format_report(const tm_session_full_report_t *r,
                                        char *dst, size_t cap)
{
    if (!r || !dst || cap < 512) return TM_ERR_BAD_STATE;
    size_t w = 0;
    w += (size_t)snprintf(dst+w, cap-w, "Session %u [%s]\n", r->session_id, r->session_status);
    w += (size_t)snprintf(dst+w, cap-w, "  Epoch: %u  Time: %lu\n", r->current_epoch, (unsigned long)r->logical_time);
    w += (size_t)snprintf(dst+w, cap-w, "  Segments: %u active, %u sealed, %lu bytes\n",
                          r->active_segments, r->sealed_segments, (unsigned long)r->total_data_bytes);
    w += (size_t)snprintf(dst+w, cap-w, "  Cursors: %u  Snapshots: %u  Leases: %u\n",
                          r->cursor_count, r->snapshot_count, r->lease_count);
    w += (size_t)snprintf(dst+w, cap-w, "  WAL: %u entries (%u pending)\n", r->wal_entries, r->wal_pending);
    w += (size_t)snprintf(dst+w, cap-w, "  Index: %u entries  Catalog: %u entries\n",
                          r->index_entries, r->catalog_entries);
    w += (size_t)snprintf(dst+w, cap-w, "  Slab: %u in use, %u free\n", r->slab_in_use, r->slab_free);
    w += (size_t)snprintf(dst+w, cap-w, "  Compact: %u merges  Retention: %u sweeps\n",
                          r->compact_merges, r->retention_sweeps);
    w += (size_t)snprintf(dst+w, cap-w, "  Tidemark: watermark=%u\n", r->tidemark_watermark);
    return TM_OK;
}
