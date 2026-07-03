#include "tm/index.h"
#include <string.h>
#include <stdlib.h>

typedef struct {
    uint32_t  valid_entries;
    uint32_t  invalid_entries;
    uint32_t  duplicates;
    double    fragmentation;
} tm_index_health_t;

tm_status_t tm_index_compute_health(const tm_index_t *idx,
                                       tm_index_health_t *health)
{
    if (!idx || !health) return TM_ERR_BAD_STATE;
    memset(health, 0, sizeof(*health));

    for (size_t i = 0; i < idx->capacity; i++) {
        if (idx->entries[i].valid)
            health->valid_entries++;
        else
            health->invalid_entries++;
    }

    /* Check for duplicate timestamps */
    for (size_t i = 0; i < idx->capacity; i++) {
        if (!idx->entries[i].valid) continue;
        for (size_t j = i + 1; j < idx->capacity; j++) {
            if (idx->entries[j].valid &&
                idx->entries[i].timestamp == idx->entries[j].timestamp)
                health->duplicates++;
        }
    }

    if (idx->capacity > 0)
        health->fragmentation = 1.0 - ((double)health->valid_entries / idx->capacity);

    return TM_OK;
}

tm_status_t tm_index_defragment(tm_index_t *idx)
{
    if (!idx || !idx->entries) return TM_ERR_BAD_STATE;

    /* Compact valid entries to the front */
    size_t write = 0;
    for (size_t read = 0; read < idx->capacity; read++) {
        if (idx->entries[read].valid) {
            if (write != read)
                idx->entries[write] = idx->entries[read];
            write++;
        }
    }

    /* Clear remaining slots */
    for (size_t i = write; i < idx->capacity; i++) {
        memset(&idx->entries[i], 0, sizeof(tm_index_entry_t));
    }

    return TM_OK;
}

tm_status_t tm_index_sort_by_timestamp(tm_index_t *idx)
{
    if (!idx || !idx->entries) return TM_ERR_BAD_STATE;

    /* First defragment to pack valid entries */
    tm_index_defragment(idx);

    /* Insertion sort by timestamp */
    for (size_t i = 1; i < idx->count; i++) {
        tm_index_entry_t key = idx->entries[i];
        int j = (int)i - 1;
        while (j >= 0 && idx->entries[j].timestamp > key.timestamp) {
            idx->entries[j + 1] = idx->entries[j];
            j--;
        }
        idx->entries[j + 1] = key;
    }

    return TM_OK;
}

tm_status_t tm_index_merge(tm_index_t *dst, const tm_index_t *src)
{
    if (!dst || !src) return TM_ERR_BAD_STATE;

    for (size_t i = 0; i < src->capacity; i++) {
        if (!src->entries[i].valid) continue;

        /* Check for duplicates */
        bool dup = false;
        for (size_t j = 0; j < dst->capacity; j++) {
            if (dst->entries[j].valid &&
                dst->entries[j].timestamp == src->entries[i].timestamp) {
                dup = true;
                break;
            }
        }
        if (dup) continue;

        /* Find empty slot in dst */
        for (size_t j = 0; j < dst->capacity; j++) {
            if (!dst->entries[j].valid) {
                dst->entries[j] = src->entries[i];
                dst->count++;
                break;
            }
        }
    }

    return TM_OK;
}

tm_status_t tm_index_split(tm_index_t *src, tm_index_t *lo, tm_index_t *hi,
                              uint64_t split_key)
{
    if (!src || !lo || !hi) return TM_ERR_BAD_STATE;

    for (size_t i = 0; i < src->capacity; i++) {
        if (!src->entries[i].valid) continue;

        tm_index_t *target = (src->entries[i].timestamp <= split_key) ? lo : hi;

        for (size_t j = 0; j < target->capacity; j++) {
            if (!target->entries[j].valid) {
                target->entries[j] = src->entries[i];
                target->count++;
                break;
            }
        }
    }

    return TM_OK;
}
