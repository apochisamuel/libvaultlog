#include "tm/core.h"
#include <string.h>

typedef struct {
    uint32_t  segment_data_cap;
    uint32_t  compact_merge_threshold;
    uint32_t  slab_capacity;
    uint32_t  slab_slot_size;
    uint32_t  index_initial_capacity;
    uint32_t  catalog_eviction_threshold;
    uint32_t  wal_max_entries;
    uint32_t  lease_total_quota;
    uint32_t  max_segments;
    uint32_t  max_cursors;
    uint32_t  max_snapshots;
    uint32_t  max_leases;
} tm_config_t;

static tm_config_t g_config = {
    .segment_data_cap = 1024,
    .compact_merge_threshold = 512,
    .slab_capacity = 128,
    .slab_slot_size = 128,
    .index_initial_capacity = 64,
    .catalog_eviction_threshold = 24,
    .wal_max_entries = 64,
    .lease_total_quota = 16,
    .max_segments = 32,
    .max_cursors = 16,
    .max_snapshots = 8,
    .max_leases = 16
};

tm_status_t tm_config_set(const tm_config_t *config)
{
    if (!config) return TM_ERR_BAD_STATE;
    g_config = *config;
    return TM_OK;
}

tm_status_t tm_config_get(tm_config_t *config)
{
    if (!config) return TM_ERR_BAD_STATE;
    *config = g_config;
    return TM_OK;
}

tm_status_t tm_config_set_defaults(void)
{
    g_config.segment_data_cap = 1024;
    g_config.compact_merge_threshold = 512;
    g_config.slab_capacity = 128;
    g_config.slab_slot_size = 128;
    g_config.index_initial_capacity = 64;
    g_config.catalog_eviction_threshold = 24;
    g_config.wal_max_entries = 64;
    g_config.lease_total_quota = 16;
    g_config.max_segments = 32;
    g_config.max_cursors = 16;
    g_config.max_snapshots = 8;
    g_config.max_leases = 16;
    return TM_OK;
}

uint32_t tm_config_segment_cap(void) { return g_config.segment_data_cap; }
uint32_t tm_config_merge_threshold(void) { return g_config.compact_merge_threshold; }
uint32_t tm_config_slab_capacity(void) { return g_config.slab_capacity; }

uint32_t tm_config_index_capacity(void) { return g_config.index_initial_capacity; }
