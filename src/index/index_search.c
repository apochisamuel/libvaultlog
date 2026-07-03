#include "tm/index.h"
#include <stdlib.h>
#include <string.h>

/*
 * Index range scanning and search operations for time-series
 * queries. Supports range queries over timestamp intervals
 * and segment-scoped lookups.
 */

typedef struct {
    uint64_t  start_timestamp;
    uint64_t  end_timestamp;
    uint32_t  max_results;
} tm_index_range_query_t;

typedef struct {
    tm_index_entry_t *results[64];
    size_t            count;
} tm_index_range_result_t;

tm_status_t tm_index_range_scan(tm_index_t *idx,
                                  uint64_t start_ts, uint64_t end_ts,
                                  tm_index_range_result_t *result)
{
    if (!idx || !idx->entries || !result) return TM_ERR_BAD_STATE;
    result->count = 0;

    for (size_t i = 0; i < idx->capacity && result->count < 64; i++) {
        if (idx->entries[i].valid &&
            idx->entries[i].timestamp >= start_ts &&
            idx->entries[i].timestamp <= end_ts) {
            result->results[result->count++] = &idx->entries[i];
        }
    }

    /* Sort by timestamp (insertion sort for small arrays) */
    for (size_t i = 1; i < result->count; i++) {
        tm_index_entry_t *key = result->results[i];
        size_t j = i;
        while (j > 0 && result->results[j-1]->timestamp > key->timestamp) {
            result->results[j] = result->results[j-1];
            j--;
        }
        result->results[j] = key;
    }

    return TM_OK;
}

tm_status_t tm_index_scan_segment(tm_index_t *idx, uint32_t segment_id,
                                    tm_index_range_result_t *result)
{
    if (!idx || !idx->entries || !result) return TM_ERR_BAD_STATE;
    result->count = 0;

    for (size_t i = 0; i < idx->capacity && result->count < 64; i++) {
        if (idx->entries[i].valid &&
            idx->entries[i].segment_id == segment_id) {
            result->results[result->count++] = &idx->entries[i];
        }
    }

    return TM_OK;
}

tm_index_entry_t *tm_index_floor(tm_index_t *idx, uint64_t timestamp)
{
    if (!idx || !idx->entries) return NULL;

    tm_index_entry_t *best = NULL;
    for (size_t i = 0; i < idx->capacity; i++) {
        if (idx->entries[i].valid && idx->entries[i].timestamp <= timestamp) {
            if (!best || idx->entries[i].timestamp > best->timestamp)
                best = &idx->entries[i];
        }
    }
    return best;
}

tm_index_entry_t *tm_index_ceiling(tm_index_t *idx, uint64_t timestamp)
{
    if (!idx || !idx->entries) return NULL;

    tm_index_entry_t *best = NULL;
    for (size_t i = 0; i < idx->capacity; i++) {
        if (idx->entries[i].valid && idx->entries[i].timestamp >= timestamp) {
            if (!best || idx->entries[i].timestamp < best->timestamp)
                best = &idx->entries[i];
        }
    }
    return best;
}

typedef struct {
    size_t   total_entries;
    size_t   valid_entries;
    uint64_t min_timestamp;
    uint64_t max_timestamp;
    size_t   unique_segments;
    double   fill_factor;
} tm_index_stats_t;

tm_status_t tm_index_compute_stats(const tm_index_t *idx, tm_index_stats_t *stats)
{
    if (!idx || !stats) return TM_ERR_BAD_STATE;
    memset(stats, 0, sizeof(*stats));
    stats->total_entries = idx->capacity;
    stats->min_timestamp = UINT64_MAX;

    uint32_t seen_segments[TM_MAX_SEGMENTS];
    size_t seen_count = 0;

    for (size_t i = 0; i < idx->capacity; i++) {
        if (!idx->entries[i].valid) continue;
        stats->valid_entries++;

        if (idx->entries[i].timestamp < stats->min_timestamp)
            stats->min_timestamp = idx->entries[i].timestamp;
        if (idx->entries[i].timestamp > stats->max_timestamp)
            stats->max_timestamp = idx->entries[i].timestamp;

        /* Count unique segments */
        bool found = false;
        for (size_t j = 0; j < seen_count; j++) {
            if (seen_segments[j] == idx->entries[i].segment_id) {
                found = true; break;
            }
        }
        if (!found && seen_count < TM_MAX_SEGMENTS) {
            seen_segments[seen_count++] = idx->entries[i].segment_id;
        }
    }

    stats->unique_segments = seen_count;
    if (stats->total_entries > 0)
        stats->fill_factor = (double)stats->valid_entries / (double)stats->total_entries;

    return TM_OK;
}
