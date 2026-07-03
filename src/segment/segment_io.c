#include "tm/segment.h"
#include "tm/crc.h"
#include <string.h>
#include <stdlib.h>

typedef struct {
    uint32_t  magic;         /* 0x544D5347 = TMSG */
    uint32_t  version;
    uint32_t  segment_id;
    uint32_t  epoch_created;
    uint32_t  data_length;
    uint32_t  block_count;
    uint32_t  checksum;
    uint32_t  flags;
} tm_segment_header_t;

#define TM_SEGMENT_MAGIC 0x544D5347

tm_status_t tm_segment_serialize(const tm_segment_t *seg,
                                    uint8_t *dst, size_t dst_cap,
                                    size_t *out_len)
{
    if (!seg || !dst || !out_len) return TM_ERR_BAD_STATE;

    size_t total = sizeof(tm_segment_header_t) + seg->data_len;
    if (total > dst_cap) return TM_ERR_OVERFLOW;

    tm_segment_header_t hdr;
    hdr.magic = TM_SEGMENT_MAGIC;
    hdr.version = 1;
    hdr.segment_id = seg->segment_id;
    hdr.epoch_created = seg->epoch_created;
    hdr.data_length = (uint32_t)seg->data_len;
    hdr.block_count = seg->append_count;
    hdr.flags = 0;
    if (seg->sealed) hdr.flags |= 0x01;
    if (seg->merged) hdr.flags |= 0x02;

    hdr.checksum = 0;
    if (seg->data && seg->data_len > 0)
        hdr.checksum = tm_crc32c(0, seg->data, seg->data_len);

    memcpy(dst, &hdr, sizeof(hdr));
    if (seg->data && seg->data_len > 0)
        memcpy(dst + sizeof(hdr), seg->data, seg->data_len);

    *out_len = total;
    return TM_OK;
}

tm_status_t tm_segment_deserialize(const uint8_t *src, size_t src_len,
                                      tm_segment_t *seg)
{
    if (!src || !seg || src_len < sizeof(tm_segment_header_t))
        return TM_ERR_TRUNCATED;

    tm_segment_header_t hdr;
    memcpy(&hdr, src, sizeof(hdr));

    if (hdr.magic != TM_SEGMENT_MAGIC) return TM_ERR_BAD_MAGIC;
    if (src_len < sizeof(hdr) + hdr.data_length) return TM_ERR_TRUNCATED;

    /* Validate checksum */
    if (hdr.data_length > 0) {
        uint32_t computed = tm_crc32c(0, src + sizeof(hdr), hdr.data_length);
        if (computed != hdr.checksum) return TM_ERR_BAD_CRC;
    }

    seg->segment_id = hdr.segment_id;
    seg->epoch_created = hdr.epoch_created;
    seg->append_count = hdr.block_count;
    seg->sealed = (hdr.flags & 0x01) != 0;
    seg->merged = (hdr.flags & 0x02) != 0;
    seg->active = true;

    if (hdr.data_length > 0) {
        free(seg->data);
        seg->data = malloc(hdr.data_length);
        if (!seg->data) return TM_ERR_ALLOC;
        memcpy(seg->data, src + sizeof(hdr), hdr.data_length);
        seg->data_len = hdr.data_length;
        seg->data_cap = hdr.data_length;
        seg->byte_count = hdr.data_length;
    }

    return TM_OK;
}

size_t tm_segment_serialized_size(const tm_segment_t *seg)
{
    if (!seg) return 0;
    return sizeof(tm_segment_header_t) + seg->data_len;
}
