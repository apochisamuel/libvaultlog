#include "tm/slab.h"
#include <string.h>

typedef struct {
    size_t    total_slots;
    size_t    in_use_slots;
    size_t    free_slots;
    size_t    total_allocs;
    size_t    total_frees;
    uint32_t  max_generation;
    size_t    type_counts[8];
} tm_slab_stats_t;

tm_status_t tm_slab_compute_stats(const tm_slab_t *slab, tm_slab_stats_t *stats)
{
    if (!slab || !stats) return TM_ERR_BAD_STATE;
    memset(stats, 0, sizeof(*stats));

    stats->total_slots = slab->capacity;
    stats->total_allocs = slab->total_allocs;
    stats->total_frees = slab->total_frees;
    stats->free_slots = slab->free_top;
    stats->in_use_slots = slab->capacity - slab->free_top;

    for (size_t i = 0; i < slab->capacity; i++) {
        if (slab->slots[i].generation > stats->max_generation)
            stats->max_generation = slab->slots[i].generation;
        if (slab->slots[i].in_use && slab->slots[i].type_tag < 8)
            stats->type_counts[slab->slots[i].type_tag]++;
    }

    return TM_OK;
}

tm_status_t tm_slab_verify_integrity(const tm_slab_t *slab)
{
    if (!slab || !slab->slots || !slab->free_stack) return TM_ERR_BAD_STATE;

    /* Verify free stack entries are valid */
    for (size_t i = 0; i < slab->free_top; i++) {
        uint32_t slot_id = slab->free_stack[i];
        if (slot_id >= slab->capacity) return TM_ERR_CORRUPT;
        if (slab->slots[slot_id].in_use) return TM_ERR_CORRUPT;
    }

    /* Verify in-use count matches */
    size_t in_use = 0;
    for (size_t i = 0; i < slab->capacity; i++) {
        if (slab->slots[i].in_use) in_use++;
    }
    if (in_use + slab->free_top != slab->capacity) return TM_ERR_CORRUPT;

    return TM_OK;
}
