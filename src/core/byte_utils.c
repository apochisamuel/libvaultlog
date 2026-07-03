#include "tm/core.h"
#include <string.h>

/*
 * Byte manipulation utilities for time-series data processing.
 */

uint32_t tm_byte_checksum(const uint8_t *data, size_t len)
{
    uint32_t sum = 0x811c9dc5;
    for (size_t i = 0; i < len; i++) {
        sum ^= data[i];
        sum *= 0x01000193;
    }
    return sum;
}

void tm_byte_xor(uint8_t *dst, const uint8_t *a, const uint8_t *b, size_t len)
{
    for (size_t i = 0; i < len; i++)
        dst[i] = a[i] ^ b[i];
}

int tm_byte_compare(const uint8_t *a, const uint8_t *b, size_t len)
{
    for (size_t i = 0; i < len; i++) {
        if (a[i] != b[i]) return (int)a[i] - (int)b[i];
    }
    return 0;
}

size_t tm_byte_find(const uint8_t *haystack, size_t hlen,
                     const uint8_t *needle, size_t nlen)
{
    if (nlen > hlen) return hlen;
    for (size_t i = 0; i <= hlen - nlen; i++) {
        if (memcmp(haystack + i, needle, nlen) == 0) return i;
    }
    return hlen;
}

void tm_byte_reverse(uint8_t *data, size_t len)
{
    for (size_t i = 0; i < len / 2; i++) {
        uint8_t tmp = data[i];
        data[i] = data[len - 1 - i];
        data[len - 1 - i] = tmp;
    }
}

size_t tm_varint_encode(uint64_t value, uint8_t *dst, size_t dst_cap)
{
    size_t i = 0;
    while (value >= 0x80 && i < dst_cap) {
        dst[i++] = (uint8_t)(value | 0x80);
        value >>= 7;
    }
    if (i < dst_cap) dst[i++] = (uint8_t)value;
    return i;
}

size_t tm_varint_decode(const uint8_t *src, size_t src_len, uint64_t *out)
{
    if (!src || !out) return 0;
    uint64_t result = 0;
    size_t i = 0;
    uint32_t shift = 0;
    while (i < src_len && shift < 63) {
        uint8_t b = src[i];
        result |= ((uint64_t)(b & 0x7F)) << shift;
        i++;
        if (!(b & 0x80)) break;
        shift += 7;
    }
    *out = result;
    return i;
}
