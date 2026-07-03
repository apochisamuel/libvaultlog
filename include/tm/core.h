#ifndef TM_CORE_H
#define TM_CORE_H
#include "tm/types.h"
#include "tm/error.h"

typedef struct {
    uint8_t *buffer;
    size_t   capacity;
    size_t   used;
} tm_arena_t;

tm_status_t  tm_arena_init(tm_arena_t *arena, size_t capacity);
void         tm_arena_destroy(tm_arena_t *arena);
void        *tm_arena_alloc(tm_arena_t *arena, size_t size);
void         tm_arena_reset(tm_arena_t *arena);
uint32_t     tm_hash32(uint32_t key);
uint64_t     tm_mix64(uint64_t a, uint64_t b);
#endif
