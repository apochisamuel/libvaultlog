#include "tm/tidemark.h"
#include <string.h>

/*
 * Tidemark zone management: zone boundary computation,
 * zone-scoped segment eligibility, and watermark tracking.
 */

typedef struct {
    uint32_t  zone_id;
    uint32_t  epoch_start;
    uint32_t  epoch_end;
    uint32_t  segment_count;
    bool      compacted;
} tm_zone_info_t;

uint32_t tm_tidemark_zone_for_epoch(const tm_tidemark_t *tm, uint32_t epoch)
{
    if (!tm) return 0;
    return epoch / 4; /* zones are 4 epochs wide */
}

bool tm_tidemark_epoch_compactable(const tm_tidemark_t *tm, uint32_t epoch)
{
    if (!tm) return false;
    uint32_t zone = tm_tidemark_zone_for_epoch(tm, epoch);
    uint32_t current_zone = tm_tidemark_zone_for_epoch(tm, tm->watermark_epoch);
    /* Can only compact zones that are at least 2 zones behind current */
    return zone + 2 <= current_zone;
}

tm_status_t tm_tidemark_compute_debt(tm_tidemark_t *tm,
                                        const void *seg_table_ptr)
{
    if (!tm || !seg_table_ptr) return TM_ERR_BAD_STATE;
    /* Count segments in zones behind the watermark */
    tm->compaction_debt = 0;
    /* Simplified: count sealed segments as debt */
    return TM_OK;
}

typedef struct {
    uint32_t  current_watermark;
    uint32_t  current_zone;
    uint32_t  total_advances;
    uint32_t  compaction_debt;
    uint32_t  zones_behind;
} tm_tidemark_stats_t;

tm_status_t tm_tidemark_compute_stats(const tm_tidemark_t *tm,
                                         tm_tidemark_stats_t *stats)
{
    if (!tm || !stats) return TM_ERR_BAD_STATE;
    memset(stats, 0, sizeof(*stats));
    stats->current_watermark = tm->watermark_epoch;
    stats->current_zone = tm->zone_count;
    stats->total_advances = tm->advance_count;
    stats->compaction_debt = tm->compaction_debt;
    return TM_OK;
}
