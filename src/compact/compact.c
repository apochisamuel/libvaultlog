#include "tm/compact.h"
#include "tm/slab.h"
#include "tm/index.h"
#include <string.h>
#include <stdlib.h>

tm_status_t tm_compact_init(tm_compact_ctx_t *ctx)
{
    if (!ctx) return TM_ERR_BAD_STATE;
    memset(ctx, 0, sizeof(*ctx));
    ctx->merge_threshold = 512;
    return TM_OK;
}

/*
 * Merge two segments into one. Source segments are destroyed.
 * Their slab slots are freed and become available for reuse.
 *
 * destroyed, their slab slots return to the free-list. The LIFO
 * slab then reuses those slots for the next allocation (which
 * may be an index node, not a segment).
 */
tm_status_t tm_compact_merge_segments(tm_compact_ctx_t *ctx,
                                        tm_segment_table_t *table,
                                        void *slab,
                                        uint32_t seg_id_a,
                                        uint32_t seg_id_b,
                                        uint32_t epoch)
{
    if (!ctx || !table) return TM_ERR_BAD_STATE;

    tm_segment_t *a = tm_segment_lookup(table, seg_id_a);
    tm_segment_t *b = tm_segment_lookup(table, seg_id_b);
    if (!a || !b) return TM_ERR_NOT_FOUND;

    /* Check merge threshold */
    uint32_t combined = a->byte_count + b->byte_count;
    if (combined < ctx->merge_threshold) return TM_ERR_BAD_STATE;

    /* Create merged output segment */
    uint32_t out_slot;
    tm_segment_t *merged = tm_segment_create(table, epoch, slab, &out_slot);
    if (!merged) return TM_ERR_ALLOC;

    /* Copy data from both sources */
    if (a->data && a->data_len > 0)
        tm_segment_append(merged, a->data, a->data_len);
    if (b->data && b->data_len > 0)
        tm_segment_append(merged, b->data, b->data_len);

    merged->sealed = true;

    /* Destroy source segments — frees their slab slots (LIFO reuse!) */
    a->merged = true;
    b->merged = true;
    tm_segment_destroy(table, a, slab);
    tm_segment_destroy(table, b, slab);

    ctx->merges_completed++;
    ctx->segments_freed += 2;
    return TM_OK;
}

tm_status_t tm_compact_trigger(tm_compact_ctx_t *ctx,
                                 tm_segment_table_t *table,
                                 void *slab, void *index_ptr,
                                 uint32_t epoch)
{
    if (!ctx || !table) return TM_ERR_BAD_STATE;
    ctx->active = true;

    /* Find pairs eligible for merge */
    for (size_t i = 0; i < TM_MAX_SEGMENTS; i++) {
        tm_segment_t *a = table->segments[i];
        if (!a || !a->active || !a->sealed) continue;

        for (size_t j = i + 1; j < TM_MAX_SEGMENTS; j++) {
            tm_segment_t *b = table->segments[j];
            if (!b || !b->active || !b->sealed) continue;

            uint32_t combined = a->byte_count + b->byte_count;
            if (combined >= ctx->merge_threshold) {
                /* Save IDs before merge destroys segments */
                uint32_t aid = a->segment_id;
                uint32_t bid = b->segment_id;
                tm_status_t rc = tm_compact_merge_segments(
                    ctx, table, slab, aid, bid, epoch);
                if (rc == TM_OK && index_ptr) {
                    tm_index_t *idx = (tm_index_t *)index_ptr;
                    tm_index_remove_segment(idx, aid);
                    tm_index_remove_segment(idx, bid);
                }
                ctx->active = false;
                return rc;
            }
        }
    }

    ctx->active = false;
    return TM_OK;
}
