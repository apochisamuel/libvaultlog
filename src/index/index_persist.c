#include "tm/index.h"
#include "tm/crc.h"
#include <string.h>
#include <stdlib.h>

#define TM_INDEX_MAGIC 0x544D4958  /* TMIX */

typedef struct {
    uint32_t  magic;
    uint32_t  version;
    uint32_t  capacity;
    uint32_t  count;
    uint32_t  rebuild_gen;
    uint32_t  checksum;
} tm_index_file_header_t;

tm_status_t tm_index_serialize(const tm_index_t *idx,
                                  uint8_t *dst, size_t dst_cap,
                                  size_t *out_len)
{
    if (!idx || !dst || !out_len) return TM_ERR_BAD_STATE;

    size_t entry_size = sizeof(uint64_t) + sizeof(uint32_t) * 2 + 1;
    size_t data_size = idx->count * entry_size;
    size_t total = sizeof(tm_index_file_header_t) + data_size;
    if (total > dst_cap) return TM_ERR_OVERFLOW;

    tm_index_file_header_t hdr;
    hdr.magic = TM_INDEX_MAGIC;
    hdr.version = 1;
    hdr.capacity = (uint32_t)idx->capacity;
    hdr.count = (uint32_t)idx->count;
    hdr.rebuild_gen = idx->rebuild_generation;

    size_t pos = sizeof(hdr);
    for (size_t i = 0; i < idx->capacity; i++) {
        if (!idx->entries[i].valid) continue;
        memcpy(dst + pos, &idx->entries[i].timestamp, 8); pos += 8;
        memcpy(dst + pos, &idx->entries[i].segment_id, 4); pos += 4;
        memcpy(dst + pos, &idx->entries[i].offset, 4); pos += 4;
        dst[pos++] = 1; /* valid marker */
    }

    hdr.checksum = tm_crc32c(0, dst + sizeof(hdr), pos - sizeof(hdr));
    memcpy(dst, &hdr, sizeof(hdr));

    *out_len = pos;
    return TM_OK;
}

tm_status_t tm_index_deserialize(const uint8_t *src, size_t src_len,
                                    tm_index_t *idx)
{
    if (!src || !idx || src_len < sizeof(tm_index_file_header_t))
        return TM_ERR_TRUNCATED;

    tm_index_file_header_t hdr;
    memcpy(&hdr, src, sizeof(hdr));
    if (hdr.magic != TM_INDEX_MAGIC) return TM_ERR_BAD_MAGIC;

    /* Reinitialize index with stored capacity */
    tm_index_destroy(idx);
    tm_index_create(idx, hdr.capacity);
    idx->rebuild_generation = hdr.rebuild_gen;

    size_t pos = sizeof(hdr);
    for (uint32_t i = 0; i < hdr.count && pos + 17 <= src_len; i++) {
        uint64_t ts;
        uint32_t seg_id, offset;
        memcpy(&ts, src + pos, 8); pos += 8;
        memcpy(&seg_id, src + pos, 4); pos += 4;
        memcpy(&offset, src + pos, 4); pos += 4;
        pos++; /* valid marker */
        tm_index_insert(idx, ts, seg_id, offset, NULL, NULL);
    }

    return TM_OK;
}

size_t tm_index_serialized_size(const tm_index_t *idx)
{
    if (!idx) return 0;
    return sizeof(tm_index_file_header_t) + idx->count * 17;
}
