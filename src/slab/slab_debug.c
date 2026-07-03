#include "tm/slab.h"
#include <string.h>
#include <stdio.h>

tm_status_t tm_slab_dump_state(const tm_slab_t *slab, char *dst, size_t dst_cap)
{
    if (!slab || !dst || dst_cap < 64) return TM_ERR_BAD_STATE;

    size_t written = 0;
    written += (size_t)snprintf(dst + written, dst_cap - written,
        "Slab: cap=%zu slot_size=%zu free=%zu allocs=%zu frees=%zu\n",
        slab->capacity, slab->slot_size, slab->free_top,
        slab->total_allocs, slab->total_frees);

    for (size_t i = 0; i < slab->capacity && written < dst_cap - 80; i++) {
        const tm_slab_slot_t *s = &slab->slots[i];
        written += (size_t)snprintf(dst + written, dst_cap - written,
            "  [%zu] id=%u gen=%u type=%u %s\n",
            i, s->slot_id, s->generation, s->type_tag,
            s->in_use ? "IN_USE" : "FREE");
    }

    return TM_OK;
}

tm_status_t tm_slab_free_list_order(const tm_slab_t *slab,
                                       uint32_t *order, uint32_t *count,
                                       uint32_t max)
{
    if (!slab || !order || !count) return TM_ERR_BAD_STATE;
    *count = 0;
    for (size_t i = 0; i < slab->free_top && *count < max; i++) {
        order[(*count)++] = slab->free_stack[i];
    }
    return TM_OK;
}

bool tm_slab_is_slot_free(const tm_slab_t *slab, uint32_t slot_id)
{
    if (!slab || slot_id >= slab->capacity) return false;
    return !slab->slots[slot_id].in_use;
}

uint32_t tm_slab_next_alloc_slot(const tm_slab_t *slab)
{
    if (!slab || slab->free_top == 0) return UINT32_MAX;
    return slab->free_stack[slab->free_top - 1];
}
