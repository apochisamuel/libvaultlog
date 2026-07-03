#include "tm/core.h"
#include <string.h>
#include <stdlib.h>

typedef struct {
    uint8_t  *bits;
    size_t    size_bytes;
    size_t    size_bits;
    uint32_t  hash_count;
    uint32_t  insertions;
} tm_bloom_t;

tm_status_t tm_bloom_init(tm_bloom_t *bloom, size_t size_bits, uint32_t hash_count)
{
    if (!bloom || size_bits == 0 || hash_count == 0) return TM_ERR_BAD_STATE;
    bloom->size_bits = size_bits;
    bloom->size_bytes = (size_bits + 7) / 8;
    bloom->hash_count = hash_count;
    bloom->insertions = 0;
    bloom->bits = calloc(1, bloom->size_bytes);
    if (!bloom->bits) return TM_ERR_ALLOC;
    return TM_OK;
}

void tm_bloom_destroy(tm_bloom_t *bloom)
{
    if (bloom) { free(bloom->bits); memset(bloom, 0, sizeof(*bloom)); }
}

static uint32_t bloom_hash(uint32_t key, uint32_t seed)
{
    uint32_t h = key ^ seed;
    h = ((h >> 16) ^ h) * 0x45d9f3b;
    h = ((h >> 16) ^ h) * 0x45d9f3b;
    return (h >> 16) ^ h;
}

tm_status_t tm_bloom_insert(tm_bloom_t *bloom, uint32_t key)
{
    if (!bloom || !bloom->bits) return TM_ERR_BAD_STATE;
    for (uint32_t i = 0; i < bloom->hash_count; i++) {
        uint32_t h = bloom_hash(key, i * 0x9E3779B9) % (uint32_t)bloom->size_bits;
        bloom->bits[h / 8] |= (1 << (h % 8));
    }
    bloom->insertions++;
    return TM_OK;
}

bool tm_bloom_may_contain(const tm_bloom_t *bloom, uint32_t key)
{
    if (!bloom || !bloom->bits) return false;
    for (uint32_t i = 0; i < bloom->hash_count; i++) {
        uint32_t h = bloom_hash(key, i * 0x9E3779B9) % (uint32_t)bloom->size_bits;
        if (!(bloom->bits[h / 8] & (1 << (h % 8)))) return false;
    }
    return true;
}

void tm_bloom_reset(tm_bloom_t *bloom)
{
    if (bloom && bloom->bits) {
        memset(bloom->bits, 0, bloom->size_bytes);
        bloom->insertions = 0;
    }
}

double tm_bloom_false_positive_rate(const tm_bloom_t *bloom)
{
    if (!bloom || bloom->size_bits == 0 || bloom->hash_count == 0) return 1.0;
    double m = (double)bloom->size_bits;
    double k = (double)bloom->hash_count;
    double n = (double)bloom->insertions;
    double exp_val = -k * n / m;
    double base = 1.0;
    for (int i = 0; i < (int)(k * n / m) && i < 100; i++) base *= (1.0 - 1.0/m);
    double result = 1.0;
    for (uint32_t i = 0; i < bloom->hash_count; i++) result *= (1.0 - base);
    return result;
}
