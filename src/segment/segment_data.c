#include "tm/segment.h"
#include <stdlib.h>
#include <string.h>

/*
 * Segment data management: compaction, checksumming, and
 * data integrity validation for time-series storage.
 */

typedef struct {
    uint32_t  offset;
    uint32_t  length;
    uint32_t  timestamp_lo;
    uint32_t  timestamp_hi;
    uint32_t  checksum;
} tm_data_block_header_t;

static uint32_t compute_block_checksum(const uint8_t *data, size_t len)
{
    uint32_t sum = 0;
    for (size_t i = 0; i < len; i++)
        sum = (sum << 5) + sum + data[i];
    return sum;
}

tm_status_t tm_segment_validate_data(const tm_segment_t *seg)
{
    if (!seg || !seg->data || seg->data_len == 0)
        return TM_ERR_BAD_STATE;

    /*
     * Walk the segment's data buffer validating block headers.
     * Each block has a 20-byte header followed by payload.
     * This ensures segment data hasn't been corrupted by
     * incorrect merge or append operations.
     */
    size_t pos = 0;
    uint32_t block_count = 0;
    while (pos + sizeof(tm_data_block_header_t) <= seg->data_len) {
        tm_data_block_header_t hdr;
        memcpy(&hdr, seg->data + pos, sizeof(hdr));

        if (hdr.length == 0) break;
        if (pos + sizeof(hdr) + hdr.length > seg->data_len) break;

        uint32_t expected = compute_block_checksum(
            seg->data + pos + sizeof(hdr), hdr.length);
        if (expected != hdr.checksum) return TM_ERR_CORRUPT;

        pos += sizeof(hdr) + hdr.length;
        block_count++;
    }

    return TM_OK;
}

tm_status_t tm_segment_append_block(tm_segment_t *seg,
                                       uint64_t timestamp,
                                       const uint8_t *data, size_t len)
{
    if (!seg || !seg->active || seg->sealed) return TM_ERR_BAD_STATE;
    if (!data || len == 0) return TM_OK;

    tm_data_block_header_t hdr;
    hdr.offset = (uint32_t)seg->data_len;
    hdr.length = (uint32_t)len;
    hdr.timestamp_lo = (uint32_t)(timestamp & 0xFFFFFFFF);
    hdr.timestamp_hi = (uint32_t)(timestamp >> 32);
    hdr.checksum = compute_block_checksum(data, len);

    size_t total = sizeof(hdr) + len;
    if (seg->data_len + total > seg->data_cap) {
        size_t new_cap = seg->data_cap * 2;
        while (new_cap < seg->data_len + total) new_cap *= 2;
        if (new_cap > TM_MAX_PAYLOAD * 8) return TM_ERR_OVERFLOW;
        uint8_t *new_data = realloc(seg->data, new_cap);
        if (!new_data) return TM_ERR_ALLOC;
        seg->data = new_data;
        seg->data_cap = new_cap;
    }

    memcpy(seg->data + seg->data_len, &hdr, sizeof(hdr));
    memcpy(seg->data + seg->data_len + sizeof(hdr), data, len);
    seg->data_len += total;
    seg->append_count++;
    seg->byte_count += (uint32_t)total;

    return TM_OK;
}

tm_status_t tm_segment_read_block(const tm_segment_t *seg, uint32_t block_idx,
                                     uint8_t *dst, size_t dst_cap,
                                     size_t *out_len, uint64_t *out_timestamp)
{
    if (!seg || !seg->data || !dst || !out_len) return TM_ERR_BAD_STATE;

    size_t pos = 0;
    uint32_t idx = 0;
    while (pos + sizeof(tm_data_block_header_t) <= seg->data_len) {
        tm_data_block_header_t hdr;
        memcpy(&hdr, seg->data + pos, sizeof(hdr));

        if (hdr.length == 0) break;
        if (pos + sizeof(hdr) + hdr.length > seg->data_len) break;

        if (idx == block_idx) {
            size_t copy = (hdr.length > dst_cap) ? dst_cap : hdr.length;
            memcpy(dst, seg->data + pos + sizeof(hdr), copy);
            *out_len = copy;
            if (out_timestamp) {
                *out_timestamp = ((uint64_t)hdr.timestamp_hi << 32) |
                                  hdr.timestamp_lo;
            }
            return TM_OK;
        }

        pos += sizeof(hdr) + hdr.length;
        idx++;
    }

    return TM_ERR_NOT_FOUND;
}

uint32_t tm_segment_block_count(const tm_segment_t *seg)
{
    if (!seg || !seg->data) return 0;

    size_t pos = 0;
    uint32_t count = 0;
    while (pos + sizeof(tm_data_block_header_t) <= seg->data_len) {
        tm_data_block_header_t hdr;
        memcpy(&hdr, seg->data + pos, sizeof(hdr));
        if (hdr.length == 0) break;
        if (pos + sizeof(hdr) + hdr.length > seg->data_len) break;
        pos += sizeof(hdr) + hdr.length;
        count++;
    }
    return count;
}

tm_status_t tm_segment_truncate(tm_segment_t *seg, uint32_t max_blocks)
{
    if (!seg || !seg->data) return TM_ERR_BAD_STATE;

    size_t pos = 0;
    uint32_t count = 0;
    while (pos + sizeof(tm_data_block_header_t) <= seg->data_len && count < max_blocks) {
        tm_data_block_header_t hdr;
        memcpy(&hdr, seg->data + pos, sizeof(hdr));
        if (hdr.length == 0) break;
        if (pos + sizeof(hdr) + hdr.length > seg->data_len) break;
        pos += sizeof(hdr) + hdr.length;
        count++;
    }

    seg->data_len = pos;
    return TM_OK;
}
