#include "tm/core.h"
#include <string.h>
#include <stdlib.h>

/* Min-heap for priority queues (used by compact scheduling) */

typedef struct {
    uint32_t  key;
    uint32_t  priority;
    void     *data;
} tm_heap_entry_t;

typedef struct {
    tm_heap_entry_t *entries;
    size_t           capacity;
    size_t           count;
} tm_min_heap_t;

tm_status_t tm_heap_init(tm_min_heap_t *h, size_t capacity)
{
    if (!h || capacity == 0) return TM_ERR_BAD_STATE;
    h->entries = calloc(capacity, sizeof(tm_heap_entry_t));
    if (!h->entries) return TM_ERR_ALLOC;
    h->capacity = capacity;
    h->count = 0;
    return TM_OK;
}

void tm_heap_destroy(tm_min_heap_t *h)
{
    if (h) { free(h->entries); memset(h, 0, sizeof(*h)); }
}

static void heap_swap(tm_min_heap_t *h, size_t a, size_t b)
{
    tm_heap_entry_t tmp = h->entries[a];
    h->entries[a] = h->entries[b];
    h->entries[b] = tmp;
}

static void heap_sift_up(tm_min_heap_t *h, size_t idx)
{
    while (idx > 0) {
        size_t parent = (idx - 1) / 2;
        if (h->entries[idx].priority < h->entries[parent].priority) {
            heap_swap(h, idx, parent);
            idx = parent;
        } else break;
    }
}

static void heap_sift_down(tm_min_heap_t *h, size_t idx)
{
    while (true) {
        size_t smallest = idx;
        size_t left = 2 * idx + 1;
        size_t right = 2 * idx + 2;
        if (left < h->count && h->entries[left].priority < h->entries[smallest].priority)
            smallest = left;
        if (right < h->count && h->entries[right].priority < h->entries[smallest].priority)
            smallest = right;
        if (smallest != idx) { heap_swap(h, idx, smallest); idx = smallest; }
        else break;
    }
}

tm_status_t tm_heap_push(tm_min_heap_t *h, uint32_t key, uint32_t priority, void *data)
{
    if (!h || h->count >= h->capacity) return TM_ERR_OVERFLOW;
    h->entries[h->count].key = key;
    h->entries[h->count].priority = priority;
    h->entries[h->count].data = data;
    heap_sift_up(h, h->count);
    h->count++;
    return TM_OK;
}

tm_status_t tm_heap_pop(tm_min_heap_t *h, uint32_t *key, uint32_t *priority, void **data)
{
    if (!h || h->count == 0) return TM_ERR_NOT_FOUND;
    if (key) *key = h->entries[0].key;
    if (priority) *priority = h->entries[0].priority;
    if (data) *data = h->entries[0].data;
    h->entries[0] = h->entries[--h->count];
    if (h->count > 0) heap_sift_down(h, 0);
    return TM_OK;
}

tm_status_t tm_heap_peek(const tm_min_heap_t *h, uint32_t *key, uint32_t *priority)
{
    if (!h || h->count == 0) return TM_ERR_NOT_FOUND;
    if (key) *key = h->entries[0].key;
    if (priority) *priority = h->entries[0].priority;
    return TM_OK;
}

size_t tm_heap_size(const tm_min_heap_t *h) { return h ? h->count : 0; }
bool tm_heap_empty(const tm_min_heap_t *h) { return !h || h->count == 0; }

tm_status_t tm_heap_update_priority(tm_min_heap_t *h, uint32_t key, uint32_t new_priority)
{
    if (!h) return TM_ERR_BAD_STATE;
    for (size_t i = 0; i < h->count; i++) {
        if (h->entries[i].key == key) {
            uint32_t old = h->entries[i].priority;
            h->entries[i].priority = new_priority;
            if (new_priority < old) heap_sift_up(h, i);
            else heap_sift_down(h, i);
            return TM_OK;
        }
    }
    return TM_ERR_NOT_FOUND;
}
