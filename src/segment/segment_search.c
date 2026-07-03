#include "tm/segment.h"
#include <string.h>

tm_segment_t *tm_segment_find_by_epoch(const tm_segment_table_t *table,
                                          uint32_t epoch)
{
    if (!table) return NULL;
    for (size_t i = 0; i < TM_MAX_SEGMENTS; i++) {
        tm_segment_t *s = table->segments[i];
        if (s && s->active && s->epoch_created == epoch) return s;
    }
    return NULL;
}

uint32_t tm_segment_count_by_epoch_range(const tm_segment_table_t *table,
                                            uint32_t start_epoch,
                                            uint32_t end_epoch)
{
    if (!table) return 0;
    uint32_t count = 0;
    for (size_t i = 0; i < TM_MAX_SEGMENTS; i++) {
        const tm_segment_t *s = table->segments[i];
        if (s && s->active &&
            s->epoch_created >= start_epoch &&
            s->epoch_created <= end_epoch)
            count++;
    }
    return count;
}

uint64_t tm_segment_total_bytes(const tm_segment_table_t *table)
{
    if (!table) return 0;
    uint64_t total = 0;
    for (size_t i = 0; i < TM_MAX_SEGMENTS; i++) {
        const tm_segment_t *s = table->segments[i];
        if (s && s->active) total += s->data_len;
    }
    return total;
}

uint32_t tm_segment_count_sealed(const tm_segment_table_t *table)
{
    if (!table) return 0;
    uint32_t count = 0;
    for (size_t i = 0; i < TM_MAX_SEGMENTS; i++) {
        const tm_segment_t *s = table->segments[i];
        if (s && s->active && s->sealed) count++;
    }
    return count;
}

uint32_t tm_segment_count_active(const tm_segment_table_t *table)
{
    if (!table) return 0;
    uint32_t count = 0;
    for (size_t i = 0; i < TM_MAX_SEGMENTS; i++) {
        if (table->segments[i] && table->segments[i]->active) count++;
    }
    return count;
}

tm_status_t tm_segment_list_ids(const tm_segment_table_t *table,
                                   uint32_t *ids, uint32_t *count, uint32_t max)
{
    if (!table || !ids || !count) return TM_ERR_BAD_STATE;
    *count = 0;
    for (size_t i = 0; i < TM_MAX_SEGMENTS && *count < max; i++) {
        const tm_segment_t *s = table->segments[i];
        if (s && s->active) ids[(*count)++] = s->segment_id;
    }
    return TM_OK;
}
