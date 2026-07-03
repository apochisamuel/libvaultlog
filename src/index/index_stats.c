#include "tm/index.h"
#include <string.h>
#include <stdlib.h>

typedef struct {
    size_t    total_entries;
    size_t    valid_entries;
    size_t    capacity;
    uint64_t  min_timestamp;
    uint64_t  max_timestamp;
    uint64_t  timestamp_span;
    double    fill_factor;
    uint32_t  unique_segments;
    uint32_t  max_entries_per_segment;
    uint32_t  rebuild_generation;
} tm_index_full_stats_t;

tm_status_t tm_index_full_stats(const tm_index_t *idx,
                                   tm_index_full_stats_t *stats)
{
    if (!idx || !stats) return TM_ERR_BAD_STATE;
    memset(stats, 0, sizeof(*stats));
    stats->total_entries = idx->capacity;
    stats->capacity = idx->capacity;
    stats->rebuild_generation = idx->rebuild_generation;
    stats->min_timestamp = UINT64_MAX;

    uint32_t seg_ids[TM_MAX_SEGMENTS];
    uint32_t seg_counts[TM_MAX_SEGMENTS];
    uint32_t seg_unique = 0;

    for (size_t i = 0; i < idx->capacity; i++) {
        if (!idx->entries[i].valid) continue;
        stats->valid_entries++;
        if (idx->entries[i].timestamp < stats->min_timestamp)
            stats->min_timestamp = idx->entries[i].timestamp;
        if (idx->entries[i].timestamp > stats->max_timestamp)
            stats->max_timestamp = idx->entries[i].timestamp;
        bool found = false;
        for (uint32_t j = 0; j < seg_unique; j++) {
            if (seg_ids[j] == idx->entries[i].segment_id) {
                seg_counts[j]++;
                found = true;
                break;
            }
        }
        if (!found && seg_unique < TM_MAX_SEGMENTS) {
            seg_ids[seg_unique] = idx->entries[i].segment_id;
            seg_counts[seg_unique] = 1;
            seg_unique++;
        }
    }

    stats->unique_segments = seg_unique;
    if (stats->min_timestamp != UINT64_MAX)
        stats->timestamp_span = stats->max_timestamp - stats->min_timestamp;
    else
        stats->min_timestamp = 0;
    if (stats->capacity > 0)
        stats->fill_factor = (double)stats->valid_entries / stats->capacity;
    for (uint32_t j = 0; j < seg_unique; j++) {
        if (seg_counts[j] > stats->max_entries_per_segment)
            stats->max_entries_per_segment = seg_counts[j];
    }
    return TM_OK;
}

tm_status_t tm_index_histogram(const tm_index_t *idx,
                                  uint32_t *buckets, uint32_t bucket_count,
                                  uint64_t bucket_width)
{
    if (!idx || !buckets || bucket_count == 0 || bucket_width == 0)
        return TM_ERR_BAD_STATE;
    memset(buckets, 0, bucket_count * sizeof(uint32_t));
    uint64_t min_ts = UINT64_MAX;
    for (size_t i = 0; i < idx->capacity; i++) {
        if (idx->entries[i].valid && idx->entries[i].timestamp < min_ts)
            min_ts = idx->entries[i].timestamp;
    }
    if (min_ts == UINT64_MAX) return TM_OK;
    for (size_t i = 0; i < idx->capacity; i++) {
        if (!idx->entries[i].valid) continue;
        uint32_t bucket = (uint32_t)((idx->entries[i].timestamp - min_ts) / bucket_width);
        if (bucket < bucket_count) buckets[bucket]++;
    }
    return TM_OK;
}
