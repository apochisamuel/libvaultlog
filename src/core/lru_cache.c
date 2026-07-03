#include "tm/core.h"
#include <string.h>
#include <stdlib.h>

#define TM_LRU_MAX 32

typedef struct tm_lru_node {
    uint32_t  key;
    void     *value;
    struct tm_lru_node *prev;
    struct tm_lru_node *next;
    bool      valid;
} tm_lru_node_t;

typedef struct {
    tm_lru_node_t  nodes[TM_LRU_MAX];
    tm_lru_node_t *head;
    tm_lru_node_t *tail;
    size_t          count;
    size_t          capacity;
    uint32_t        hits;
    uint32_t        misses;
    uint32_t        evictions;
} tm_lru_cache_t;

tm_status_t tm_lru_init(tm_lru_cache_t *cache, size_t capacity)
{
    if (!cache) return TM_ERR_BAD_STATE;
    memset(cache, 0, sizeof(*cache));
    cache->capacity = (capacity > TM_LRU_MAX) ? TM_LRU_MAX : capacity;
    return TM_OK;
}

static void lru_remove_node(tm_lru_cache_t *c, tm_lru_node_t *n)
{
    if (n->prev) n->prev->next = n->next;
    else c->head = n->next;
    if (n->next) n->next->prev = n->prev;
    else c->tail = n->prev;
    n->prev = n->next = NULL;
}

static void lru_push_front(tm_lru_cache_t *c, tm_lru_node_t *n)
{
    n->next = c->head;
    n->prev = NULL;
    if (c->head) c->head->prev = n;
    c->head = n;
    if (!c->tail) c->tail = n;
}

void *tm_lru_get(tm_lru_cache_t *cache, uint32_t key)
{
    if (!cache) return NULL;
    for (size_t i = 0; i < TM_LRU_MAX; i++) {
        if (cache->nodes[i].valid && cache->nodes[i].key == key) {
            lru_remove_node(cache, &cache->nodes[i]);
            lru_push_front(cache, &cache->nodes[i]);
            cache->hits++;
            return cache->nodes[i].value;
        }
    }
    cache->misses++;
    return NULL;
}

tm_status_t tm_lru_put(tm_lru_cache_t *cache, uint32_t key, void *value)
{
    if (!cache) return TM_ERR_BAD_STATE;
    for (size_t i = 0; i < TM_LRU_MAX; i++) {
        if (cache->nodes[i].valid && cache->nodes[i].key == key) {
            cache->nodes[i].value = value;
            lru_remove_node(cache, &cache->nodes[i]);
            lru_push_front(cache, &cache->nodes[i]);
            return TM_OK;
        }
    }
    if (cache->count >= cache->capacity) {
        tm_lru_node_t *victim = cache->tail;
        if (victim) {
            lru_remove_node(cache, victim);
            victim->valid = false;
            cache->count--;
            cache->evictions++;
        }
    }
    for (size_t i = 0; i < TM_LRU_MAX; i++) {
        if (!cache->nodes[i].valid) {
            cache->nodes[i].key = key;
            cache->nodes[i].value = value;
            cache->nodes[i].valid = true;
            lru_push_front(cache, &cache->nodes[i]);
            cache->count++;
            return TM_OK;
        }
    }
    return TM_ERR_OVERFLOW;
}

tm_status_t tm_lru_remove(tm_lru_cache_t *cache, uint32_t key)
{
    if (!cache) return TM_ERR_BAD_STATE;
    for (size_t i = 0; i < TM_LRU_MAX; i++) {
        if (cache->nodes[i].valid && cache->nodes[i].key == key) {
            lru_remove_node(cache, &cache->nodes[i]);
            cache->nodes[i].valid = false;
            cache->count--;
            return TM_OK;
        }
    }
    return TM_ERR_NOT_FOUND;
}

void tm_lru_clear(tm_lru_cache_t *cache)
{
    if (!cache) return;
    memset(cache->nodes, 0, sizeof(cache->nodes));
    cache->head = cache->tail = NULL;
    cache->count = 0;
}

double tm_lru_hit_rate(const tm_lru_cache_t *cache)
{
    if (!cache) return 0.0;
    uint32_t total = cache->hits + cache->misses;
    return total > 0 ? (double)cache->hits / total : 0.0;
}
