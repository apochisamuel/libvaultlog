#ifndef TM_SLAB_H
#define TM_SLAB_H

#include "tm/types.h"
#include "tm/error.h"

#define TM_SLAB_TYPE_NONE    0
#define TM_SLAB_TYPE_SEGMENT 1
#define TM_SLAB_TYPE_INDEX   2
#define TM_SLAB_TYPE_LEASE   3
#define TM_SLAB_TYPE_CURSOR  4
#define TM_SLAB_TYPE_WAL     5

typedef struct {
    void     *ptr;
    uint32_t  slot_id;
    uint32_t  generation;       /* incremented on each alloc/free cycle */
    uint8_t   type_tag;         /* what type was last allocated here */
    bool      in_use;
} tm_slab_slot_t;

typedef struct {
    tm_slab_slot_t *slots;
    size_t          capacity;
    size_t          slot_size;   /* fixed allocation size per slot */
    uint32_t       *free_stack; /* LIFO free-list */
    size_t          free_top;
    size_t          total_allocs;
    size_t          total_frees;
} tm_slab_t;

tm_status_t  tm_slab_init(tm_slab_t *slab, size_t capacity, size_t slot_size);
void         tm_slab_destroy(tm_slab_t *slab);
void        *tm_slab_alloc(tm_slab_t *slab, uint8_t type_tag, uint32_t *out_slot_id);
tm_status_t  tm_slab_free(tm_slab_t *slab, uint32_t slot_id);
void        *tm_slab_get(tm_slab_t *slab, uint32_t slot_id);
uint32_t     tm_slab_slot_generation(const tm_slab_t *slab, uint32_t slot_id);
uint8_t      tm_slab_slot_type(const tm_slab_t *slab, uint32_t slot_id);

#endif
