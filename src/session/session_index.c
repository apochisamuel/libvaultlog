#include "tm/session.h"
#include <string.h>

typedef struct {
    uint32_t  total_entries;
    uint32_t  valid_entries;
    uint32_t  orphaned_entries;
    uint32_t  duplicate_entries;
    uint32_t  rebuild_generation;
    size_t    memory_usage;
} tm_session_index_report_t;

tm_status_t tm_session_index_report(const tm_session_t *sess,
                                       tm_session_index_report_t *report)
{
    if (!sess || !report) return TM_ERR_BAD_STATE;
    memset(report, 0, sizeof(*report));
    report->total_entries = (uint32_t)sess->index.capacity;
    report->rebuild_generation = sess->index.rebuild_generation;
    report->memory_usage = sess->index.capacity * sizeof(tm_index_entry_t);

    for (size_t i = 0; i < sess->index.capacity; i++) {
        if (!sess->index.entries[i].valid) continue;
        report->valid_entries++;
        bool seg_exists = false;
        for (size_t j = 0; j < TM_MAX_SEGMENTS; j++) {
            const tm_segment_t *s = sess->segments.segments[j];
            if (s && s->active && s->segment_id == sess->index.entries[i].segment_id) {
                seg_exists = true; break;
            }
        }
        if (!seg_exists) report->orphaned_entries++;
        for (size_t k = i + 1; k < sess->index.capacity; k++) {
            if (sess->index.entries[k].valid &&
                sess->index.entries[k].timestamp == sess->index.entries[i].timestamp)
                report->duplicate_entries++;
        }
    }
    return TM_OK;
}

tm_status_t tm_session_index_gc(tm_session_t *sess)
{
    if (!sess) return TM_ERR_BAD_STATE;
    for (size_t i = 0; i < sess->index.capacity; i++) {
        if (!sess->index.entries[i].valid) continue;
        bool found = false;
        for (size_t j = 0; j < TM_MAX_SEGMENTS; j++) {
            const tm_segment_t *s = sess->segments.segments[j];
            if (s && s->active && s->segment_id == sess->index.entries[i].segment_id) {
                found = true; break;
            }
        }
        if (!found) {
            sess->index.entries[i].valid = false;
            sess->index.count--;
        }
    }
    return TM_OK;
}

tm_status_t tm_session_index_rebuild_optimal(tm_session_t *sess)
{
    if (!sess) return TM_ERR_BAD_STATE;
    uint32_t active_segments = 0;
    for (size_t i = 0; i < TM_MAX_SEGMENTS; i++) {
        if (sess->segments.segments[i] && sess->segments.segments[i]->active)
            active_segments++;
    }
    size_t optimal_cap = active_segments * 4;
    if (optimal_cap < 8) optimal_cap = 8;
    if (optimal_cap > TM_MAX_INDEX_NODES) optimal_cap = TM_MAX_INDEX_NODES;
    return tm_index_rebuild(&sess->index, optimal_cap);
}
