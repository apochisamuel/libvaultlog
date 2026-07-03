#include "tm/index.h"
#include "tm/slab.h"
#include <stdlib.h>
#include <string.h>

tm_status_t tm_index_create(tm_index_t *idx, size_t capacity)
{
    if (!idx || capacity == 0) return TM_ERR_BAD_STATE;
    idx->entries = calloc(capacity, sizeof(tm_index_entry_t));
    if (!idx->entries) return TM_ERR_ALLOC;
    idx->capacity = capacity;
    idx->count = 0;
    idx->rebuild_generation = 1;
    return TM_OK;
}

void tm_index_destroy(tm_index_t *idx)
{
    if (!idx) return;
    free(idx->entries);
    idx->entries = NULL;
    idx->count = 0;
}

tm_status_t tm_index_insert(tm_index_t *idx, uint64_t timestamp,
                              uint32_t segment_id, uint32_t offset,
                              void *slab_ptr, uint32_t *out_slot)
{
    if (!idx || !idx->entries) return TM_ERR_BAD_STATE;
    if (idx->count >= idx->capacity) return TM_ERR_OVERFLOW;

    /* Find empty slot */
    for (size_t i = 0; i < idx->capacity; i++) {
        if (!idx->entries[i].valid) {
            idx->entries[i].timestamp  = timestamp;
            idx->entries[i].segment_id = segment_id;
            idx->entries[i].offset     = offset;
            idx->entries[i].valid      = true;

            /* Track in slab if provided */
            if (slab_ptr) {
                tm_slab_t *slab = (tm_slab_t *)slab_ptr;
                uint32_t slot_id;
                void *slot = tm_slab_alloc(slab, TM_SLAB_TYPE_INDEX, &slot_id);
                if (slot) {
                    idx->entries[i].slab_slot_id = slot_id;
                    if (out_slot) *out_slot = slot_id;
                }
            }

            idx->count++;
            return TM_OK;
        }
    }
    return TM_ERR_OVERFLOW;
}

tm_index_entry_t *tm_index_lookup(tm_index_t *idx, uint64_t timestamp)
{
    if (!idx || !idx->entries) return NULL;
    for (size_t i = 0; i < idx->capacity; i++) {
        if (idx->entries[i].valid && idx->entries[i].timestamp == timestamp)
            return &idx->entries[i];
    }
    return NULL;
}

tm_status_t tm_index_remove_segment(tm_index_t *idx, uint32_t segment_id)
{
    if (!idx || !idx->entries) return TM_ERR_BAD_STATE;
    for (size_t i = 0; i < idx->capacity; i++) {
        if (idx->entries[i].valid && idx->entries[i].segment_id == segment_id) {
            idx->entries[i].valid = false;
            idx->count--;
        }
    }
    return TM_OK;
}

tm_status_t tm_index_rebuild(tm_index_t *idx, size_t new_capacity)
{
    if (!idx) return TM_ERR_BAD_STATE;
    if (new_capacity < idx->count) new_capacity = idx->count;

    tm_index_entry_t *new_entries = calloc(new_capacity, sizeof(tm_index_entry_t));
    if (!new_entries) return TM_ERR_ALLOC;

    size_t j = 0;
    for (size_t i = 0; i < idx->capacity && j < new_capacity; i++) {
        if (idx->entries[i].valid) {
            new_entries[j++] = idx->entries[i];
        }
    }

    free(idx->entries);
    idx->entries = new_entries;
    idx->capacity = new_capacity;
    idx->count = j;
    idx->rebuild_generation++;
    return TM_OK;
}

size_t tm_index_count(const tm_index_t *idx)
{
    return idx ? idx->count : 0;
}
