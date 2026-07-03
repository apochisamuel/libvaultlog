#include "tm/segment.h"
#include <string.h>

typedef struct {
    uint32_t  total_segments;
    uint32_t  active_segments;
    uint32_t  sealed_segments;
    uint32_t  merged_segments;
    uint64_t  total_bytes;
    uint64_t  total_appends;
    uint32_t  avg_segment_size;
    uint32_t  max_segment_size;
    uint32_t  min_segment_size;
} tm_segment_stats_t;

tm_status_t tm_segment_table_stats(const tm_segment_table_t *table,
                                      tm_segment_stats_t *stats)
{
    if (!table || !stats) return TM_ERR_BAD_STATE;
    memset(stats, 0, sizeof(*stats));
    stats->min_segment_size = UINT32_MAX;

    for (size_t i = 0; i < TM_MAX_SEGMENTS; i++) {
        const tm_segment_t *s = table->segments[i];
        if (!s) continue;

        stats->total_segments++;
        if (s->active) stats->active_segments++;
        if (s->sealed) stats->sealed_segments++;
        if (s->merged) stats->merged_segments++;
        stats->total_bytes += s->data_len;
        stats->total_appends += s->append_count;

        if (s->data_len > stats->max_segment_size)
            stats->max_segment_size = (uint32_t)s->data_len;
        if (s->data_len < stats->min_segment_size)
            stats->min_segment_size = (uint32_t)s->data_len;
    }

    if (stats->active_segments > 0)
        stats->avg_segment_size = (uint32_t)(stats->total_bytes / stats->active_segments);
    if (stats->min_segment_size == UINT32_MAX)
        stats->min_segment_size = 0;

    return TM_OK;
}

tm_status_t tm_segment_table_compact_candidates(const tm_segment_table_t *table,
                                                   uint32_t threshold,
                                                   uint32_t *out_pairs)
{
    if (!table || !out_pairs) return TM_ERR_BAD_STATE;
    *out_pairs = 0;

    for (size_t i = 0; i < TM_MAX_SEGMENTS; i++) {
        const tm_segment_t *a = table->segments[i];
        if (!a || !a->active || !a->sealed) continue;

        for (size_t j = i + 1; j < TM_MAX_SEGMENTS; j++) {
            const tm_segment_t *b = table->segments[j];
            if (!b || !b->active || !b->sealed) continue;

            if (a->byte_count + b->byte_count >= threshold)
                (*out_pairs)++;
        }
    }

    return TM_OK;
}
