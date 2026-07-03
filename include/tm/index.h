#ifndef TM_INDEX_H
#define TM_INDEX_H
#include "tm/types.h"
#include "tm/error.h"

typedef struct {
    uint64_t  timestamp;
    uint32_t  segment_id;
    uint32_t  offset;
    uint32_t  slab_slot_id;
    bool      valid;
} tm_index_entry_t;

typedef struct {
    tm_index_entry_t *entries;
    size_t            capacity;
    size_t            count;
    uint32_t          rebuild_generation;
} tm_index_t;

tm_status_t  tm_index_create(tm_index_t *idx, size_t capacity);
void         tm_index_destroy(tm_index_t *idx);
tm_status_t  tm_index_insert(tm_index_t *idx, uint64_t timestamp,
                               uint32_t segment_id, uint32_t offset,
                               void *slab, uint32_t *out_slot);
tm_index_entry_t *tm_index_lookup(tm_index_t *idx, uint64_t timestamp);
tm_status_t  tm_index_remove_segment(tm_index_t *idx, uint32_t segment_id);
tm_status_t  tm_index_rebuild(tm_index_t *idx, size_t new_capacity);
size_t       tm_index_count(const tm_index_t *idx);
#endif
