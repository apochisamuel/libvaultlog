#include "tm/wal.h"
#include "tm/segment.h"
#include <string.h>

typedef struct {
    uint32_t  checkpoint_epoch;
    uint32_t  entry_count;
    uint32_t  total_data_bytes;
    bool      complete;
} tm_wal_checkpoint_info_t;

tm_status_t tm_wal_create_checkpoint(tm_wal_t *wal,
                                        const void *seg_table_ptr,
                                        uint32_t epoch)
{
    if (!wal || !seg_table_ptr) return TM_ERR_BAD_STATE;
    const tm_segment_table_t *table = (const tm_segment_table_t *)seg_table_ptr;

    for (size_t i = 0; i < TM_MAX_SEGMENTS; i++) {
        const tm_segment_t *seg = table->segments[i];
        if (!seg || !seg->active || seg->data_len == 0) continue;

        if (wal->count >= TM_MAX_WAL_ENTRIES) break;

        tm_wal_checkpoint(wal, seg->segment_id, (void *)seg,
                            seg->slab_slot_id, epoch,
                            seg->generation, 0, (uint32_t)seg->data_len);
    }

    wal->checkpoint_epoch = epoch;
    return TM_OK;
}

tm_status_t tm_wal_get_checkpoint_info(const tm_wal_t *wal,
                                          tm_wal_checkpoint_info_t *info)
{
    if (!wal || !info) return TM_ERR_BAD_STATE;
    memset(info, 0, sizeof(*info));

    info->checkpoint_epoch = wal->checkpoint_epoch;
    for (size_t i = 0; i < wal->count; i++) {
        if (wal->entries[i].valid) {
            info->entry_count++;
            info->total_data_bytes += wal->entries[i].data_length;
        }
    }
    info->complete = (info->entry_count > 0);
    return TM_OK;
}

tm_status_t tm_wal_discard_before(tm_wal_t *wal, uint32_t entry_id)
{
    if (!wal) return TM_ERR_BAD_STATE;
    for (size_t i = 0; i < wal->count; i++) {
        if (wal->entries[i].entry_id < entry_id)
            wal->entries[i].valid = false;
    }
    return TM_OK;
}

size_t tm_wal_memory_usage(const tm_wal_t *wal)
{
    if (!wal) return 0;
    size_t usage = sizeof(tm_wal_t);
    for (size_t i = 0; i < wal->count; i++) {
        usage += sizeof(tm_wal_entry_t);
    }
    return usage;
}
