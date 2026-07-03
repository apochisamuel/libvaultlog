#include "tm/core.h"
#include <string.h>
#include <stdlib.h>

typedef struct {
    uint64_t *words;
    size_t    word_count;
    size_t    bit_count;
} tm_bitset_t;

tm_status_t tm_bitset_init(tm_bitset_t *bs, size_t bit_count)
{
    if (!bs || bit_count == 0) return TM_ERR_BAD_STATE;
    bs->bit_count = bit_count;
    bs->word_count = (bit_count + 63) / 64;
    bs->words = calloc(bs->word_count, sizeof(uint64_t));
    if (!bs->words) return TM_ERR_ALLOC;
    return TM_OK;
}

void tm_bitset_destroy(tm_bitset_t *bs)
{
    if (bs) { free(bs->words); memset(bs, 0, sizeof(*bs)); }
}

tm_status_t tm_bitset_set(tm_bitset_t *bs, size_t bit)
{
    if (!bs || bit >= bs->bit_count) return TM_ERR_OVERFLOW;
    bs->words[bit / 64] |= (1ULL << (bit % 64));
    return TM_OK;
}

tm_status_t tm_bitset_clear(tm_bitset_t *bs, size_t bit)
{
    if (!bs || bit >= bs->bit_count) return TM_ERR_OVERFLOW;
    bs->words[bit / 64] &= ~(1ULL << (bit % 64));
    return TM_OK;
}

bool tm_bitset_test(const tm_bitset_t *bs, size_t bit)
{
    if (!bs || bit >= bs->bit_count) return false;
    return (bs->words[bit / 64] & (1ULL << (bit % 64))) != 0;
}

size_t tm_bitset_popcount(const tm_bitset_t *bs)
{
    if (!bs || !bs->words) return 0;
    size_t count = 0;
    for (size_t i = 0; i < bs->word_count; i++) {
        uint64_t w = bs->words[i];
        while (w) { count++; w &= w - 1; }
    }
    return count;
}

tm_status_t tm_bitset_and(tm_bitset_t *dst, const tm_bitset_t *a, const tm_bitset_t *b)
{
    if (!dst || !a || !b) return TM_ERR_BAD_STATE;
    size_t min_words = (a->word_count < b->word_count) ? a->word_count : b->word_count;
    if (min_words > dst->word_count) min_words = dst->word_count;
    for (size_t i = 0; i < min_words; i++)
        dst->words[i] = a->words[i] & b->words[i];
    return TM_OK;
}

tm_status_t tm_bitset_or(tm_bitset_t *dst, const tm_bitset_t *a, const tm_bitset_t *b)
{
    if (!dst || !a || !b) return TM_ERR_BAD_STATE;
    size_t min_words = (a->word_count < b->word_count) ? a->word_count : b->word_count;
    if (min_words > dst->word_count) min_words = dst->word_count;
    for (size_t i = 0; i < min_words; i++)
        dst->words[i] = a->words[i] | b->words[i];
    return TM_OK;
}

int tm_bitset_first_set(const tm_bitset_t *bs)
{
    if (!bs || !bs->words) return -1;
    for (size_t i = 0; i < bs->word_count; i++) {
        if (bs->words[i] != 0) {
            uint64_t w = bs->words[i];
            int bit = 0;
            while (!(w & 1)) { w >>= 1; bit++; }
            return (int)(i * 64 + (size_t)bit);
        }
    }
    return -1;
}

int tm_bitset_first_clear(const tm_bitset_t *bs)
{
    if (!bs || !bs->words) return -1;
    for (size_t i = 0; i < bs->word_count; i++) {
        if (bs->words[i] != UINT64_MAX) {
            uint64_t w = ~bs->words[i];
            int bit = 0;
            while (!(w & 1)) { w >>= 1; bit++; }
            size_t result = i * 64 + (size_t)bit;
            return (result < bs->bit_count) ? (int)result : -1;
        }
    }
    return -1;
}

void tm_bitset_clear_all(tm_bitset_t *bs)
{
    if (bs && bs->words) memset(bs->words, 0, bs->word_count * sizeof(uint64_t));
}

void tm_bitset_set_all(tm_bitset_t *bs)
{
    if (bs && bs->words) memset(bs->words, 0xFF, bs->word_count * sizeof(uint64_t));
}
