#include "tm/catalog.h"
#include <string.h>

/*
 * Catalog LRU management: scoring, eviction policies, and
 * pressure tracking for segment metadata registry.
 */

typedef struct {
    uint32_t  segment_id;
    uint32_t  score;
    uint32_t  access_count;
    bool      pinned;
} tm_catalog_lru_entry_t;

static uint32_t compute_lru_score(const tm_catalog_entry_t *e)
{
    if (!e || !e->valid) return UINT32_MAX;
    /* Lower score = more eligible for eviction
     * Score = access_count * 4 + (generation / 16)
     * This weights frequent access heavily while giving
     * a small bonus to newer entries */
    return e->access_count * 4 + (e->generation / 16);
}

tm_status_t tm_catalog_evict_under_pressure(tm_catalog_t *cat, uint32_t count)
{
    if (!cat) return TM_ERR_BAD_STATE;

    for (uint32_t evicted = 0; evicted < count; evicted++) {
        int32_t min_idx = -1;
        uint32_t min_score = UINT32_MAX;

        for (size_t i = 0; i < cat->capacity; i++) {
            if (!cat->entries[i].valid || cat->entries[i].pinned) continue;
            uint32_t score = compute_lru_score(&cat->entries[i]);
            if (score < min_score) {
                min_score = score;
                min_idx = (int32_t)i;
            }
        }

        if (min_idx < 0) return TM_ERR_NOT_FOUND;

        cat->entries[min_idx].valid = false;
        cat->entries[min_idx].segment_ref = NULL;
        cat->count--;
    }

    return TM_OK;
}

bool tm_catalog_under_pressure(const tm_catalog_t *cat)
{
    if (!cat) return false;
    return cat->count >= cat->eviction_threshold;
}

tm_status_t tm_catalog_reset_access_counts(tm_catalog_t *cat)
{
    if (!cat) return TM_ERR_BAD_STATE;
    for (size_t i = 0; i < cat->capacity; i++) {
        if (cat->entries[i].valid)
            cat->entries[i].access_count = 0;
    }
    return TM_OK;
}

typedef struct {
    size_t    total_entries;
    size_t    valid_entries;
    size_t    pinned_entries;
    uint32_t  total_accesses;
    uint32_t  max_access_count;
    uint32_t  avg_access_count;
    double    pressure_ratio;
} tm_catalog_stats_t;

tm_status_t tm_catalog_compute_stats(const tm_catalog_t *cat,
                                        tm_catalog_stats_t *stats)
{
    if (!cat || !stats) return TM_ERR_BAD_STATE;
    memset(stats, 0, sizeof(*stats));

    for (size_t i = 0; i < cat->capacity; i++) {
        stats->total_entries++;
        if (!cat->entries[i].valid) continue;
        stats->valid_entries++;
        if (cat->entries[i].pinned) stats->pinned_entries++;
        stats->total_accesses += cat->entries[i].access_count;
        if (cat->entries[i].access_count > stats->max_access_count)
            stats->max_access_count = cat->entries[i].access_count;
    }

    if (stats->valid_entries > 0)
        stats->avg_access_count = stats->total_accesses / (uint32_t)stats->valid_entries;
    if (cat->capacity > 0)
        stats->pressure_ratio = (double)stats->valid_entries / (double)cat->capacity;

    return TM_OK;
}
