#include "tm/retention.h"
#include <string.h>

/*
 * Retention policy management: multi-policy evaluation,
 * policy composition, and eligibility computation.
 */

typedef struct {
    uint32_t  eligible_count;
    uint32_t  protected_count;
    uint32_t  policy_evaluations;
} tm_retention_stats_t;

tm_status_t tm_retention_evaluate_all(tm_retention_ctx_t *ctx,
                                        uint32_t segment_epoch,
                                        uint32_t current_epoch,
                                        bool *out_eligible)
{
    if (!ctx || !out_eligible) return TM_ERR_BAD_STATE;
    *out_eligible = false;

    if (ctx->policy_count == 0) return TM_OK;

    /* All active policies must agree for eligibility */
    uint32_t agreed = 0;
    uint32_t active_count = 0;

    for (size_t i = 0; i < ctx->policy_count; i++) {
        if (!ctx->policies[i].active) continue;
        active_count++;

        uint32_t age = current_epoch - segment_epoch;
        if (age >= ctx->policies[i].max_epoch_age)
            agreed++;
    }

    if (active_count > 0 && agreed == active_count)
        *out_eligible = true;

    return TM_OK;
}

tm_status_t tm_retention_remove_policy(tm_retention_ctx_t *ctx,
                                         uint32_t policy_id)
{
    if (!ctx) return TM_ERR_BAD_STATE;
    for (size_t i = 0; i < ctx->policy_count; i++) {
        if (ctx->policies[i].policy_id == policy_id) {
            ctx->policies[i].active = false;
            return TM_OK;
        }
    }
    return TM_ERR_NOT_FOUND;
}

tm_status_t tm_retention_compute_stats(const tm_retention_ctx_t *ctx,
                                          tm_retention_stats_t *stats)
{
    if (!ctx || !stats) return TM_ERR_BAD_STATE;
    memset(stats, 0, sizeof(*stats));
    stats->policy_evaluations = ctx->sweep_count;
    return TM_OK;
}
