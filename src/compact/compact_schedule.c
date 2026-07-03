#include "tm/compact.h"
#include "tm/segment.h"
#include "tm/tidemark.h"
#include <string.h>

typedef enum {
    TM_COMPACT_PRIORITY_NONE   = 0,
    TM_COMPACT_PRIORITY_LOW    = 1,
    TM_COMPACT_PRIORITY_MEDIUM = 2,
    TM_COMPACT_PRIORITY_HIGH   = 3,
    TM_COMPACT_PRIORITY_URGENT = 4
} tm_compact_priority_t;

typedef struct {
    tm_compact_priority_t priority;
    uint32_t              eligible_pairs;
    uint32_t              sealed_segments;
    uint32_t              total_segments;
    uint64_t              reclaimable_bytes;
    bool                  should_run;
} tm_compact_schedule_t;

tm_status_t tm_compact_evaluate_schedule(const tm_segment_table_t *table,
                                            const tm_tidemark_t *tidemark,
                                            uint32_t threshold,
                                            tm_compact_schedule_t *sched)
{
    if (!table || !sched) return TM_ERR_BAD_STATE;
    memset(sched, 0, sizeof(*sched));

    for (size_t i = 0; i < TM_MAX_SEGMENTS; i++) {
        const tm_segment_t *s = table->segments[i];
        if (!s || !s->active) continue;
        sched->total_segments++;
        if (s->sealed) {
            sched->sealed_segments++;
            sched->reclaimable_bytes += s->byte_count;
        }
    }

    for (size_t i = 0; i < TM_MAX_SEGMENTS; i++) {
        const tm_segment_t *a = table->segments[i];
        if (!a || !a->active || !a->sealed) continue;
        for (size_t j = i + 1; j < TM_MAX_SEGMENTS; j++) {
            const tm_segment_t *b = table->segments[j];
            if (!b || !b->active || !b->sealed) continue;
            if (a->byte_count + b->byte_count >= threshold)
                sched->eligible_pairs++;
        }
    }

    if (sched->eligible_pairs == 0) {
        sched->priority = TM_COMPACT_PRIORITY_NONE;
        sched->should_run = false;
    } else if (sched->sealed_segments > TM_MAX_SEGMENTS * 3 / 4) {
        sched->priority = TM_COMPACT_PRIORITY_URGENT;
        sched->should_run = true;
    } else if (sched->sealed_segments > TM_MAX_SEGMENTS / 2) {
        sched->priority = TM_COMPACT_PRIORITY_HIGH;
        sched->should_run = true;
    } else if (sched->eligible_pairs > 3) {
        sched->priority = TM_COMPACT_PRIORITY_MEDIUM;
        sched->should_run = true;
    } else {
        sched->priority = TM_COMPACT_PRIORITY_LOW;
        sched->should_run = (tidemark && tidemark->advance_count > 2);
    }

    return TM_OK;
}

tm_status_t tm_compact_run_scheduled(tm_compact_ctx_t *ctx,
                                        tm_segment_table_t *table,
                                        void *slab, void *index_ptr,
                                        uint32_t epoch,
                                        uint32_t max_merges)
{
    if (!ctx || !table) return TM_ERR_BAD_STATE;
    uint32_t merges = 0;
    bool found = true;
    while (found && merges < max_merges) {
        found = false;
        for (size_t i = 0; i < TM_MAX_SEGMENTS && !found; i++) {
            tm_segment_t *a = table->segments[i];
            if (!a || !a->active || !a->sealed) continue;
            for (size_t j = i + 1; j < TM_MAX_SEGMENTS; j++) {
                tm_segment_t *b = table->segments[j];
                if (!b || !b->active || !b->sealed) continue;
                if (a->byte_count + b->byte_count >= ctx->merge_threshold) {
                    uint32_t aid = a->segment_id, bid = b->segment_id;
                    tm_compact_merge_segments(ctx, table, slab, aid, bid, epoch);
                    merges++;
                    found = true;
                    break;
                }
            }
        }
    }
    return TM_OK;
}
