#include "tm/slab.h"
#include <stdlib.h>
#include <string.h>

tm_status_t tm_slab_init(tm_slab_t *slab, size_t capacity, size_t slot_size)
{
    if (!slab || capacity == 0 || slot_size == 0) return TM_ERR_BAD_STATE;

    slab->slots = calloc(capacity, sizeof(tm_slab_slot_t));
    if (!slab->slots) return TM_ERR_ALLOC;

    slab->free_stack = calloc(capacity, sizeof(uint32_t));
    if (!slab->free_stack) { free(slab->slots); return TM_ERR_ALLOC; }

    slab->capacity = capacity;
    slab->slot_size = slot_size;
    slab->free_top = 0;
    slab->total_allocs = 0;
    slab->total_frees = 0;

    /* Initialize slots and push all onto free stack (LIFO) */
    for (size_t i = 0; i < capacity; i++) {
        slab->slots[i].ptr = malloc(slot_size);
        if (!slab->slots[i].ptr) {
            /* Cleanup on failure */
            for (size_t j = 0; j < i; j++) free(slab->slots[j].ptr);
            free(slab->slots); free(slab->free_stack);
            return TM_ERR_ALLOC;
        }
        memset(slab->slots[i].ptr, 0, slot_size);
        slab->slots[i].slot_id    = (uint32_t)i;
        slab->slots[i].generation = 0;
        slab->slots[i].type_tag   = TM_SLAB_TYPE_NONE;
        slab->slots[i].in_use     = false;

        /* Push onto free stack — last slot pushed = first allocated (LIFO) */
        slab->free_stack[slab->free_top++] = (uint32_t)i;
    }

    return TM_OK;
}

void tm_slab_destroy(tm_slab_t *slab)
{
    if (!slab) return;
    if (slab->slots) {
        for (size_t i = 0; i < slab->capacity; i++)
            free(slab->slots[i].ptr);
        free(slab->slots);
    }
    free(slab->free_stack);
    memset(slab, 0, sizeof(*slab));
}

void *tm_slab_alloc(tm_slab_t *slab, uint8_t type_tag, uint32_t *out_slot_id)
{
    if (!slab || slab->free_top == 0) return NULL;

    /* Pop from free stack (LIFO — most recently freed slot is reused first) */
    uint32_t slot_id = slab->free_stack[--slab->free_top];
    tm_slab_slot_t *slot = &slab->slots[slot_id];

    slot->in_use     = true;
    slot->type_tag   = type_tag;
    slot->generation++;
    slab->total_allocs++;

    memset(slot->ptr, 0, slab->slot_size);

    if (out_slot_id) *out_slot_id = slot_id;
    return slot->ptr;
}

tm_status_t tm_slab_free(tm_slab_t *slab, uint32_t slot_id)
{
    if (!slab || slot_id >= slab->capacity) return TM_ERR_BAD_STATE;

    tm_slab_slot_t *slot = &slab->slots[slot_id];
    if (!slot->in_use) return TM_ERR_BAD_STATE;

    slot->in_use = false;
    /* Push back onto free stack (LIFO) */
    slab->free_stack[slab->free_top++] = slot_id;
    slab->total_frees++;

    return TM_OK;
}

void *tm_slab_get(tm_slab_t *slab, uint32_t slot_id)
{
    if (!slab || slot_id >= slab->capacity) return NULL;
    return slab->slots[slot_id].ptr;
}

uint32_t tm_slab_slot_generation(const tm_slab_t *slab, uint32_t slot_id)
{
    if (!slab || slot_id >= slab->capacity) return 0;
    return slab->slots[slot_id].generation;
}

uint8_t tm_slab_slot_type(const tm_slab_t *slab, uint32_t slot_id)
{
    if (!slab || slot_id >= slab->capacity) return TM_SLAB_TYPE_NONE;
    return slab->slots[slot_id].type_tag;
}
