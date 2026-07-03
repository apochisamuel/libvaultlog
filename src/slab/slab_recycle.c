#include "tm/slab.h"
#include <string.h>

tm_status_t tm_slab_recycle_type(tm_slab_t *slab, uint32_t slot_id,
                                    uint8_t new_type)
{
    if (!slab || slot_id >= slab->capacity) return TM_ERR_BAD_STATE;
    tm_slab_slot_t *slot = &slab->slots[slot_id];
    if (!slot->in_use) return TM_ERR_BAD_STATE;
    slot->type_tag = new_type;
    slot->generation++;
    memset(slot->ptr, 0, slab->slot_size);
    return TM_OK;
}

tm_status_t tm_slab_find_by_type(const tm_slab_t *slab, uint8_t type,
                                    uint32_t *out_ids, uint32_t *out_count,
                                    uint32_t max_results)
{
    if (!slab || !out_ids || !out_count) return TM_ERR_BAD_STATE;
    *out_count = 0;
    for (size_t i = 0; i < slab->capacity && *out_count < max_results; i++) {
        if (slab->slots[i].in_use && slab->slots[i].type_tag == type) {
            out_ids[(*out_count)++] = (uint32_t)i;
        }
    }
    return TM_OK;
}

tm_status_t tm_slab_defragment(tm_slab_t *slab)
{
    if (!slab || !slab->free_stack) return TM_ERR_BAD_STATE;
    /* Rebuild free stack in reverse order (highest slot first) */
    slab->free_top = 0;
    for (size_t i = slab->capacity; i > 0; i--) {
        if (!slab->slots[i - 1].in_use) {
            slab->free_stack[slab->free_top++] = (uint32_t)(i - 1);
        }
    }
    return TM_OK;
}

bool tm_slab_slot_was_recycled(const tm_slab_t *slab, uint32_t slot_id,
                                 uint32_t expected_gen)
{
    if (!slab || slot_id >= slab->capacity) return false;
    return slab->slots[slot_id].generation != expected_gen;
}

bool tm_slab_slot_type_changed(const tm_slab_t *slab, uint32_t slot_id,
                                  uint8_t expected_type)
{
    if (!slab || slot_id >= slab->capacity) return false;
    return slab->slots[slot_id].type_tag != expected_type;
}
