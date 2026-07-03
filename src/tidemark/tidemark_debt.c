#include "tm/tidemark.h"
#include "tm/segment.h"
#include <string.h>

typedef struct {
    uint32_t  zone_id;
    uint32_t  segment_count;
    uint32_t  sealed_count;
    uint64_t  total_bytes;
    bool      compactable;
} tm_zone_summary_t;

tm_status_t tm_tidemark_zone_summary(const tm_tidemark_t *tm,
                                        const void *seg_table_ptr,
                                        uint32_t zone_id,
                                        tm_zone_summary_t *summary)
{
    if (!tm || !seg_table_ptr || !summary) return TM_ERR_BAD_STATE;
    memset(summary, 0, sizeof(*summary));
    summary->zone_id = zone_id;

    const tm_segment_table_t *table = (const tm_segment_table_t *)seg_table_ptr;
    uint32_t zone_start = zone_id * 4;
    uint32_t zone_end = zone_start + 4;

    for (size_t i = 0; i < TM_MAX_SEGMENTS; i++) {
        const tm_segment_t *s = table->segments[i];
        if (!s || !s->active) continue;
        if (s->epoch_created >= zone_start && s->epoch_created < zone_end) {
            summary->segment_count++;
            if (s->sealed) summary->sealed_count++;
            summary->total_bytes += s->data_len;
        }
    }

    summary->compactable = tm_tidemark_epoch_compactable(tm, zone_start);
    return TM_OK;
}

tm_status_t tm_tidemark_compute_total_debt(const tm_tidemark_t *tm,
                                              const void *seg_table_ptr,
                                              uint32_t *out_debt)
{
    if (!tm || !seg_table_ptr || !out_debt) return TM_ERR_BAD_STATE;
    *out_debt = 0;

    const tm_segment_table_t *table = (const tm_segment_table_t *)seg_table_ptr;
    for (size_t i = 0; i < TM_MAX_SEGMENTS; i++) {
        const tm_segment_t *s = table->segments[i];
        if (!s || !s->active || !s->sealed) continue;
        if (tm_tidemark_epoch_compactable(tm, s->epoch_created))
            (*out_debt)++;
    }

    return TM_OK;
}

tm_status_t tm_tidemark_advance_to_epoch(tm_tidemark_t *tm, uint32_t target_epoch)
{
    if (!tm) return TM_ERR_BAD_STATE;
    while (tm->watermark_epoch < target_epoch) {
        tm_tidemark_advance(tm, tm->watermark_epoch + 1);
    }
    return TM_OK;
}
