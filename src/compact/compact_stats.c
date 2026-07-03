#include "tm/compact.h"
#include <string.h>

typedef struct {
    uint32_t  total_merges;
    uint32_t  total_segments_freed;
    uint64_t  total_bytes_merged;
    uint32_t  failed_merges;
    double    avg_merge_ratio;
} tm_compact_summary_t;

tm_status_t tm_compact_summary(const tm_compact_ctx_t *ctx,
                                  tm_compact_summary_t *summary)
{
    if (!ctx || !summary) return TM_ERR_BAD_STATE;
    memset(summary, 0, sizeof(*summary));
    summary->total_merges = ctx->merges_completed;
    summary->total_segments_freed = ctx->segments_freed;
    if (ctx->merges_completed > 0)
        summary->avg_merge_ratio = (double)ctx->segments_freed / ctx->merges_completed;
    return TM_OK;
}

tm_status_t tm_compact_set_threshold(tm_compact_ctx_t *ctx, uint32_t threshold)
{
    if (!ctx) return TM_ERR_BAD_STATE;
    ctx->merge_threshold = threshold;
    return TM_OK;
}

bool tm_compact_active(const tm_compact_ctx_t *ctx)
{
    return ctx ? ctx->active : false;
}

uint32_t tm_compact_merges_done(const tm_compact_ctx_t *ctx)
{
    return ctx ? ctx->merges_completed : 0;
}
