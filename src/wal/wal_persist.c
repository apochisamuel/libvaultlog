#include "tm/wal.h"
#include "tm/crc.h"
#include <string.h>

#define TM_WAL_MAGIC 0x544D574C  /* TMWL */

typedef struct {
    uint32_t  magic;
    uint32_t  version;
    uint32_t  entry_count;
    uint32_t  checkpoint_epoch;
    uint32_t  checksum;
} tm_wal_file_header_t;

tm_status_t tm_wal_serialize(const tm_wal_t *wal,
                                uint8_t *dst, size_t dst_cap,
                                size_t *out_len)
{
    if (!wal || !dst || !out_len) return TM_ERR_BAD_STATE;

    size_t entry_size = 4 * 8 + 1; /* 8 uint32 fields + valid byte */
    size_t data_size = 0;
    for (size_t i = 0; i < wal->count; i++) {
        if (wal->entries[i].valid) data_size += entry_size;
    }
    size_t total = sizeof(tm_wal_file_header_t) + data_size;
    if (total > dst_cap) return TM_ERR_OVERFLOW;

    tm_wal_file_header_t hdr;
    hdr.magic = TM_WAL_MAGIC;
    hdr.version = 1;
    hdr.checkpoint_epoch = wal->checkpoint_epoch;

    size_t pos = sizeof(hdr);
    uint32_t count = 0;
    for (size_t i = 0; i < wal->count; i++) {
        const tm_wal_entry_t *e = &wal->entries[i];
        if (!e->valid) continue;

        memcpy(dst + pos, &e->entry_id, 4); pos += 4;
        memcpy(dst + pos, &e->entry_type, 4); pos += 4;
        memcpy(dst + pos, &e->segment_id, 4); pos += 4;
        memcpy(dst + pos, &e->slab_slot_id, 4); pos += 4;
        memcpy(dst + pos, &e->epoch, 4); pos += 4;
        memcpy(dst + pos, &e->sequence, 4); pos += 4;
        memcpy(dst + pos, &e->data_offset, 4); pos += 4;
        memcpy(dst + pos, &e->data_length, 4); pos += 4;
        dst[pos++] = e->replayed ? 1 : 0;
        count++;
    }

    hdr.entry_count = count;
    hdr.checksum = tm_crc32c(0, dst + sizeof(hdr), pos - sizeof(hdr));
    memcpy(dst, &hdr, sizeof(hdr));

    *out_len = pos;
    return TM_OK;
}

size_t tm_wal_serialized_size(const tm_wal_t *wal)
{
    if (!wal) return 0;
    size_t count = 0;
    for (size_t i = 0; i < wal->count; i++) {
        if (wal->entries[i].valid) count++;
    }
    return sizeof(tm_wal_file_header_t) + count * 33;
}
