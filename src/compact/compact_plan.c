#include "tm/compact.h"
#include "tm/tidemark.h"
#include "tm/index.h"
#include "tm/catalog.h"
#include <string.h>

typedef struct {
    uint32_t  pair_a;
    uint32_t  pair_b;
    uint32_t  combined_size;
    uint32_t  priority;     /* lower = higher priority */
} tm_compact_pair_t;

typedef struct {
    tm_compact_pair_t  pairs[TM_MAX_SEGMENTS];
    uint32_t           pair_count;
    uint32_t           total_bytes_reclaimable;
} tm_compact_plan_t;

tm_status_t tm_compact_build_plan(const tm_segment_table_t *table,
                                     uint32_t threshold,
                                     tm_compact_plan_t *plan)
{
    if (!table || !plan) return TM_ERR_BAD_STATE;
    memset(plan, 0, sizeof(*plan));

    for (size_t i = 0; i < TM_MAX_SEGMENTS; i++) {
        const tm_segment_t *a = table->segments[i];
        if (!a || !a->active || !a->sealed) continue;

        for (size_t j = i + 1; j < TM_MAX_SEGMENTS; j++) {
            const tm_segment_t *b = table->segments[j];
            if (!b || !b->active || !b->sealed) continue;

            uint32_t combined = a->byte_count + b->byte_count;
            if (combined >= threshold && plan->pair_count < TM_MAX_SEGMENTS) {
                tm_compact_pair_t *p = &plan->pairs[plan->pair_count++];
                p->pair_a = a->segment_id;
                p->pair_b = b->segment_id;
                p->combined_size = combined;
                p->priority = combined; /* larger pairs = lower priority number */
                plan->total_bytes_reclaimable += combined;
            }
        }
    }

    /* Sort by priority (insertion sort) */
    for (uint32_t i = 1; i < plan->pair_count; i++) {
        tm_compact_pair_t key = plan->pairs[i];
        int j = (int)i - 1;
        while (j >= 0 && plan->pairs[j].priority > key.priority) {
            plan->pairs[j + 1] = plan->pairs[j];
            j--;
        }
        plan->pairs[j + 1] = key;
    }

    return TM_OK;
}

tm_status_t tm_compact_execute_plan(tm_compact_ctx_t *ctx,
                                       tm_segment_table_t *table,
                                       void *slab, void *index_ptr,
                                       const tm_compact_plan_t *plan,
                                       uint32_t max_merges,
                                       uint32_t epoch)
{
    if (!ctx || !table || !plan) return TM_ERR_BAD_STATE;

    uint32_t merges = 0;
    for (uint32_t i = 0; i < plan->pair_count && merges < max_merges; i++) {
        tm_segment_t *a = tm_segment_lookup(table, plan->pairs[i].pair_a);
        tm_segment_t *b = tm_segment_lookup(table, plan->pairs[i].pair_b);
        if (!a || !b) continue;

        uint32_t aid = a->segment_id, bid = b->segment_id;
        tm_status_t rc = tm_compact_merge_segments(ctx, table, slab,
                                                      aid, bid, epoch);
        if (rc == TM_OK) {
            if (index_ptr) {
                tm_index_t *idx = (tm_index_t *)index_ptr;
                tm_index_remove_segment(idx, aid);
                tm_index_remove_segment(idx, bid);
            }
            merges++;
        }
    }

    return TM_OK;
}

bool tm_compact_should_trigger(const tm_segment_table_t *table,
                                 const tm_tidemark_t *tidemark,
                                 uint32_t threshold)
{
    if (!table || !tidemark) return false;
    uint32_t sealed = 0;
    for (size_t i = 0; i < TM_MAX_SEGMENTS; i++) {
        const tm_segment_t *s = table->segments[i];
        if (s && s->active && s->sealed) sealed++;
    }
    return sealed >= 2 && tidemark->advance_count > 0;
}
