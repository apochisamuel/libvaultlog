#include "tm/session.h"
#include <string.h>

typedef struct {
    uint32_t  eligible_pairs;
    uint32_t  sealed_segments;
    uint32_t  total_sealed_bytes;
    uint32_t  compaction_debt;
    bool      should_compact;
} tm_session_compact_assessment_t;

tm_status_t tm_session_assess_compaction(const tm_session_t *sess,
                                            tm_session_compact_assessment_t *a)
{
    if (!sess || !a) return TM_ERR_BAD_STATE;
    memset(a, 0, sizeof(*a));
    for (size_t i = 0; i < TM_MAX_SEGMENTS; i++) {
        const tm_segment_t *s = sess->segments.segments[i];
        if (!s || !s->active || !s->sealed) continue;
        a->sealed_segments++;
        a->total_sealed_bytes += s->byte_count;
        for (size_t j = i + 1; j < TM_MAX_SEGMENTS; j++) {
            const tm_segment_t *s2 = sess->segments.segments[j];
            if (!s2 || !s2->active || !s2->sealed) continue;
            if (s->byte_count + s2->byte_count >= sess->compact.merge_threshold)
                a->eligible_pairs++;
        }
    }
    a->compaction_debt = a->sealed_segments;
    a->should_compact = a->eligible_pairs > 0;
    return TM_OK;
}

tm_status_t tm_session_compact_with_invalidation(tm_session_t *sess)
{
    if (!sess || !sess->active) return TM_ERR_NOT_ACTIVE;
    uint32_t pre_count = 0;
    for (size_t i = 0; i < TM_MAX_SEGMENTS; i++) {
        if (sess->segments.segments[i] && sess->segments.segments[i]->active)
            pre_count++;
    }
    tm_status_t rc = tm_compact_trigger(&sess->compact, &sess->segments,
                                           &sess->slab, &sess->index,
                                           sess->clock.current_epoch);
    uint32_t post_count = 0;
    for (size_t i = 0; i < TM_MAX_SEGMENTS; i++) {
        if (sess->segments.segments[i] && sess->segments.segments[i]->active)
            post_count++;
    }
    if (post_count < pre_count) {
        tm_tidemark_advance(&sess->tidemark, sess->clock.current_epoch);
    }
    return rc;
}

tm_status_t tm_session_seal_full_segments(tm_session_t *sess,
                                             uint32_t threshold)
{
    if (!sess) return TM_ERR_BAD_STATE;
    for (size_t i = 0; i < TM_MAX_SEGMENTS; i++) {
        tm_segment_t *s = sess->segments.segments[i];
        if (s && s->active && !s->sealed && s->byte_count >= threshold)
            s->sealed = true;
    }
    return TM_OK;
}
