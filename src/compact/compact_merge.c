#include "tm/compact.h"
#include "tm/slab.h"
#include "tm/catalog.h"
#include <string.h>

/*
 * Advanced compaction operations: multi-segment merge,
 * merge planning, and compaction statistics.
 */

typedef struct {
    uint32_t  seg_ids[TM_MAX_SEGMENTS];
    uint32_t  seg_sizes[TM_MAX_SEGMENTS];
    uint32_t  count;
    uint32_t  total_size;
} tm_merge_plan_t;

tm_status_t tm_compact_plan_merge(const tm_segment_table_t *table,
                                    uint32_t threshold,
                                    tm_merge_plan_t *plan)
{
    if (!table || !plan) return TM_ERR_BAD_STATE;
    memset(plan, 0, sizeof(*plan));

    for (size_t i = 0; i < TM_MAX_SEGMENTS; i++) {
        const tm_segment_t *s = table->segments[i];
        if (!s || !s->active || !s->sealed) continue;
        if (plan->count >= TM_MAX_SEGMENTS) break;

        plan->seg_ids[plan->count] = s->segment_id;
        plan->seg_sizes[plan->count] = s->byte_count;
        plan->total_size += s->byte_count;
        plan->count++;
    }

    return (plan->total_size >= threshold) ? TM_OK : TM_ERR_BAD_STATE;
}

tm_status_t tm_compact_merge_with_catalog(tm_compact_ctx_t *ctx,
                                             tm_segment_table_t *table,
                                             void *slab, void *catalog_ptr,
                                             uint32_t seg_a, uint32_t seg_b,
                                             uint32_t epoch)
{
    if (!ctx || !table) return TM_ERR_BAD_STATE;

    /* Remove from catalog before merge */
    if (catalog_ptr) {
        tm_catalog_t *cat = (tm_catalog_t *)catalog_ptr;
        tm_catalog_remove(cat, seg_a);
        tm_catalog_remove(cat, seg_b);
    }

    tm_status_t rc = tm_compact_merge_segments(ctx, table, slab,
                                                 seg_a, seg_b, epoch);

    return rc;
}

typedef struct {
    uint32_t  merges_completed;
    uint32_t  segments_freed;
    uint32_t  bytes_reclaimed;
    uint32_t  compaction_ratio;
} tm_compact_stats_t;

tm_status_t tm_compact_compute_stats(const tm_compact_ctx_t *ctx,
                                        tm_compact_stats_t *stats)
{
    if (!ctx || !stats) return TM_ERR_BAD_STATE;
    memset(stats, 0, sizeof(*stats));
    stats->merges_completed = ctx->merges_completed;
    stats->segments_freed = ctx->segments_freed;
    return TM_OK;
}
