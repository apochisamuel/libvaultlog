#include "tm/retention.h"
#include <string.h>

typedef struct {
    uint32_t  active_policies;
    uint32_t  min_max_age;
    uint32_t  max_max_age;
    uint32_t  avg_max_age;
    uint32_t  total_sweeps;
} tm_retention_config_summary_t;

tm_status_t tm_retention_config_summary(const tm_retention_ctx_t *ctx,
                                           tm_retention_config_summary_t *summary)
{
    if (!ctx || !summary) return TM_ERR_BAD_STATE;
    memset(summary, 0, sizeof(*summary));
    summary->min_max_age = UINT32_MAX;
    summary->total_sweeps = ctx->sweep_count;
    uint32_t total_age = 0;
    for (size_t i = 0; i < ctx->policy_count; i++) {
        if (!ctx->policies[i].active) continue;
        summary->active_policies++;
        total_age += ctx->policies[i].max_epoch_age;
        if (ctx->policies[i].max_epoch_age < summary->min_max_age)
            summary->min_max_age = ctx->policies[i].max_epoch_age;
        if (ctx->policies[i].max_epoch_age > summary->max_max_age)
            summary->max_max_age = ctx->policies[i].max_epoch_age;
    }
    if (summary->active_policies > 0)
        summary->avg_max_age = total_age / summary->active_policies;
    if (summary->min_max_age == UINT32_MAX) summary->min_max_age = 0;
    return TM_OK;
}

tm_status_t tm_retention_set_default_policy(tm_retention_ctx_t *ctx,
                                               uint32_t max_age)
{
    if (!ctx) return TM_ERR_BAD_STATE;
    return tm_retention_set_policy(ctx, 0, max_age);
}

tm_status_t tm_retention_clear_policies(tm_retention_ctx_t *ctx)
{
    if (!ctx) return TM_ERR_BAD_STATE;
    for (size_t i = 0; i < ctx->policy_count; i++)
        ctx->policies[i].active = false;
    return TM_OK;
}

bool tm_retention_has_active_policy(const tm_retention_ctx_t *ctx)
{
    if (!ctx) return false;
    for (size_t i = 0; i < ctx->policy_count; i++) {
        if (ctx->policies[i].active) return true;
    }
    return false;
}

uint32_t tm_retention_sweep_count(const tm_retention_ctx_t *ctx)
{
    return ctx ? ctx->sweep_count : 0;
}
