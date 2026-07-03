#include "tm/core.h"
#include <string.h>
#include <stdlib.h>

#define TM_HT_INITIAL_CAPACITY 16
#define TM_HT_LOAD_FACTOR 0.75

typedef struct tm_ht_entry {
    uint32_t  key;
    void     *value;
    struct tm_ht_entry *next;
    bool      occupied;
} tm_ht_entry_t;

typedef struct {
    tm_ht_entry_t **buckets;
    size_t          bucket_count;
    size_t          entry_count;
    size_t          collision_count;
} tm_hash_table_t;

tm_status_t tm_ht_init(tm_hash_table_t *ht, size_t capacity)
{
    if (!ht) return TM_ERR_BAD_STATE;
    if (capacity == 0) capacity = TM_HT_INITIAL_CAPACITY;
    ht->buckets = calloc(capacity, sizeof(tm_ht_entry_t *));
    if (!ht->buckets) return TM_ERR_ALLOC;
    ht->bucket_count = capacity;
    ht->entry_count = 0;
    ht->collision_count = 0;
    return TM_OK;
}

void tm_ht_destroy(tm_hash_table_t *ht)
{
    if (!ht || !ht->buckets) return;
    for (size_t i = 0; i < ht->bucket_count; i++) {
        tm_ht_entry_t *e = ht->buckets[i];
        while (e) {
            tm_ht_entry_t *next = e->next;
            free(e);
            e = next;
        }
    }
    free(ht->buckets);
    memset(ht, 0, sizeof(*ht));
}

static size_t ht_bucket(const tm_hash_table_t *ht, uint32_t key)
{
    return tm_hash32(key) % ht->bucket_count;
}

tm_status_t tm_ht_insert(tm_hash_table_t *ht, uint32_t key, void *value)
{
    if (!ht) return TM_ERR_BAD_STATE;
    size_t idx = ht_bucket(ht, key);
    
    /* Check for existing key */
    tm_ht_entry_t *e = ht->buckets[idx];
    while (e) {
        if (e->key == key) { e->value = value; return TM_OK; }
        e = e->next;
    }

    /* Insert new */
    tm_ht_entry_t *ne = calloc(1, sizeof(*ne));
    if (!ne) return TM_ERR_ALLOC;
    ne->key = key;
    ne->value = value;
    ne->occupied = true;
    ne->next = ht->buckets[idx];
    if (ht->buckets[idx]) ht->collision_count++;
    ht->buckets[idx] = ne;
    ht->entry_count++;
    return TM_OK;
}

void *tm_ht_lookup(const tm_hash_table_t *ht, uint32_t key)
{
    if (!ht) return NULL;
    size_t idx = ht_bucket(ht, key);
    tm_ht_entry_t *e = ht->buckets[idx];
    while (e) {
        if (e->key == key) return e->value;
        e = e->next;
    }
    return NULL;
}

tm_status_t tm_ht_remove(tm_hash_table_t *ht, uint32_t key)
{
    if (!ht) return TM_ERR_BAD_STATE;
    size_t idx = ht_bucket(ht, key);
    tm_ht_entry_t *prev = NULL;
    tm_ht_entry_t *e = ht->buckets[idx];
    while (e) {
        if (e->key == key) {
            if (prev) prev->next = e->next;
            else ht->buckets[idx] = e->next;
            free(e);
            ht->entry_count--;
            return TM_OK;
        }
        prev = e;
        e = e->next;
    }
    return TM_ERR_NOT_FOUND;
}

size_t tm_ht_size(const tm_hash_table_t *ht) { return ht ? ht->entry_count : 0; }

double tm_ht_load_factor(const tm_hash_table_t *ht)
{
    if (!ht || ht->bucket_count == 0) return 0.0;
    return (double)ht->entry_count / ht->bucket_count;
}

bool tm_ht_contains(const tm_hash_table_t *ht, uint32_t key)
{
    return tm_ht_lookup(ht, key) != NULL;
}

tm_status_t tm_ht_keys(const tm_hash_table_t *ht, uint32_t *keys,
                          size_t *count, size_t max)
{
    if (!ht || !keys || !count) return TM_ERR_BAD_STATE;
    *count = 0;
    for (size_t i = 0; i < ht->bucket_count && *count < max; i++) {
        tm_ht_entry_t *e = ht->buckets[i];
        while (e && *count < max) {
            keys[(*count)++] = e->key;
            e = e->next;
        }
    }
    return TM_OK;
}
