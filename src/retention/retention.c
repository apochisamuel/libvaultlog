#include "tm/retention.h"
#include "tm/segment.h"
#include "tm/catalog.h"
#include "tm/slab.h"
#include <string.h>

tm_status_t tm_retention_init(tm_retention_ctx_t *ctx)
{
    if (!ctx) return TM_ERR_BAD_STATE;
    memset(ctx, 0, sizeof(*ctx));
    return TM_OK;
}

tm_status_t tm_retention_set_policy(tm_retention_ctx_t *ctx,
                                      uint32_t policy_id,
                                      uint32_t max_epoch_age)
{
    if (!ctx || ctx->policy_count >= 4) return TM_ERR_OVERFLOW;
    for (size_t i = 0; i < ctx->policy_count; i++) {
        if (ctx->policies[i].policy_id == policy_id) {
            ctx->policies[i].max_epoch_age = max_epoch_age;
            ctx->policies[i].active = true;
            return TM_OK;
        }
    }
    tm_retention_policy_t *p = &ctx->policies[ctx->policy_count++];
    p->policy_id = policy_id;
    p->max_epoch_age = max_epoch_age;
    p->active = true;
    return TM_OK;
}

bool tm_retention_eligible(tm_retention_ctx_t *ctx,
                             uint32_t segment_epoch, uint32_t current_epoch)
{
    if (!ctx || ctx->policy_count == 0) return false;
    /* ALL active policies must agree for eligibility */
    for (size_t i = 0; i < ctx->policy_count; i++) {
        if (!ctx->policies[i].active) continue;
        if (current_epoch < segment_epoch + ctx->policies[i].max_epoch_age)
            return false;
    }
    return true;
}

tm_status_t tm_retention_sweep(tm_retention_ctx_t *ctx,
                                 void *seg_table_ptr, void *catalog_ptr,
                                 void *slab_ptr, uint32_t current_epoch)
{
    if (!ctx || !seg_table_ptr) return TM_ERR_BAD_STATE;
    tm_segment_table_t *table = (tm_segment_table_t *)seg_table_ptr;
    tm_catalog_t *cat = (tm_catalog_t *)catalog_ptr;

    for (size_t i = 0; i < TM_MAX_SEGMENTS; i++) {
        tm_segment_t *seg = table->segments[i];
        if (!seg || !seg->active) continue;

        if (tm_retention_eligible(ctx, seg->epoch_created, current_epoch)) {
            /* Remove from catalog first */
            if (cat) tm_catalog_remove(cat, seg->segment_id);
            /* Destroy segment (frees slab slot) */
            tm_segment_destroy(table, seg, slab_ptr);
            ctx->sweep_count++;
        }
    }

    return TM_OK;
}
