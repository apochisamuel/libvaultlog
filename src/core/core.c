#include "tm/core.h"
#include <stdlib.h>
#include <string.h>

tm_status_t tm_arena_init(tm_arena_t *arena, size_t capacity)
{
    if (!arena || capacity == 0) return TM_ERR_BAD_STATE;
    arena->buffer = calloc(1, capacity);
    if (!arena->buffer) return TM_ERR_ALLOC;
    arena->capacity = capacity;
    arena->used = 0;
    return TM_OK;
}

void tm_arena_destroy(tm_arena_t *arena)
{
    if (arena) { free(arena->buffer); memset(arena, 0, sizeof(*arena)); }
}

void *tm_arena_alloc(tm_arena_t *arena, size_t size)
{
    if (!arena || !arena->buffer) return NULL;
    size = (size + 7) & ~(size_t)7; /* align to 8 bytes */
    if (arena->used + size > arena->capacity) return NULL;
    void *ptr = arena->buffer + arena->used;
    arena->used += size;
    return ptr;
}

void tm_arena_reset(tm_arena_t *arena)
{
    if (arena) { memset(arena->buffer, 0, arena->used); arena->used = 0; }
}

uint32_t tm_hash32(uint32_t key)
{
    key = ((key >> 16) ^ key) * 0x45d9f3b;
    key = ((key >> 16) ^ key) * 0x45d9f3b;
    key = (key >> 16) ^ key;
    return key;
}

uint64_t tm_mix64(uint64_t a, uint64_t b)
{
    a ^= b; a *= 0xff51afd7ed558ccdULL;
    a ^= a >> 33; a *= 0xc4ceb9fe1a85ec53ULL;
    return a ^ (a >> 33);
}
