#include "tm/retention.h"
#include "tm/segment.h"
#include "tm/catalog.h"
#include "tm/lease.h"
#include <string.h>

typedef struct {
    uint32_t  segments_evaluated;
    uint32_t  segments_eligible;
    uint32_t  segments_protected;
    uint32_t  segments_swept;
    uint32_t  lease_conflicts;
} tm_retention_sweep_result_t;

tm_status_t tm_retention_evaluate_segment(tm_retention_ctx_t *ctx,
                                             const tm_segment_t *seg,
                                             uint32_t current_epoch,
                                             const tm_lease_table_t *lt,
                                             bool *out_eligible)
{
    if (!ctx || !seg || !out_eligible) return TM_ERR_BAD_STATE;
    *out_eligible = false;

    /* Check retention policies */
    if (!tm_retention_eligible(ctx, seg->epoch_created, current_epoch))
        return TM_OK;

    /* Check lease protection */
    if (lt) {
        for (size_t i = 0; i < TM_MAX_LEASES; i++) {
            const tm_lease_t *l = lt->leases[i];
            if (!l || !l->active) continue;
            if (l->segment_id == seg->segment_id && !l->expired) {
                return TM_OK; /* Protected by active lease */
            }
        }
    }

    *out_eligible = true;
    return TM_OK;
}

tm_status_t tm_retention_sweep_with_leases(tm_retention_ctx_t *ctx,
                                              void *seg_table_ptr,
                                              void *catalog_ptr,
                                              void *slab_ptr,
                                              const tm_lease_table_t *lt,
                                              uint32_t current_epoch,
                                              tm_retention_sweep_result_t *result)
{
    if (!ctx || !seg_table_ptr) return TM_ERR_BAD_STATE;
    tm_segment_table_t *table = (tm_segment_table_t *)seg_table_ptr;

    if (result) memset(result, 0, sizeof(*result));

    for (size_t i = 0; i < TM_MAX_SEGMENTS; i++) {
        tm_segment_t *seg = table->segments[i];
        if (!seg || !seg->active) continue;

        if (result) result->segments_evaluated++;

        bool eligible = false;
        tm_retention_evaluate_segment(ctx, seg, current_epoch, lt, &eligible);

        if (eligible) {
            if (result) result->segments_eligible++;
            if (catalog_ptr)
                tm_catalog_remove((tm_catalog_t *)catalog_ptr, seg->segment_id);
            tm_segment_destroy(table, seg, slab_ptr);
            if (result) result->segments_swept++;
            ctx->sweep_count++;
        } else {
            if (result) result->segments_protected++;
        }
    }

    return TM_OK;
}
