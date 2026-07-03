#include "tm/wal.h"
#include "tm/segment.h"
#include "tm/slab.h"
#include <stdlib.h>
#include <string.h>

tm_status_t tm_wal_init(tm_wal_t *wal)
{
    if (!wal) return TM_ERR_BAD_STATE;
    memset(wal, 0, sizeof(*wal));
    wal->next_id = 1;
    return TM_OK;
}

void tm_wal_destroy(tm_wal_t *wal)
{
    if (wal) memset(wal, 0, sizeof(*wal));
}

tm_status_t tm_wal_checkpoint(tm_wal_t *wal, uint32_t segment_id,
                                void *segment_ptr, uint32_t slab_slot,
                                uint32_t epoch, uint32_t gen,
                                uint32_t offset, uint32_t length)
{
    if (!wal || wal->count >= TM_MAX_WAL_ENTRIES) return TM_ERR_OVERFLOW;

    tm_wal_entry_t *entry = &wal->entries[wal->count];
    entry->entry_id       = wal->next_id++;
    entry->entry_type     = TM_SLAB_TYPE_SEGMENT;
    entry->segment_id     = segment_id;
    entry->slab_slot_id   = slab_slot;
    entry->cached_segment = segment_ptr;  /* raw pointer cached here */
    entry->epoch          = epoch;
    entry->sequence       = (uint32_t)wal->count;
    entry->data_offset    = offset;
    entry->data_length    = length;
    entry->checkpoint_gen = gen;
    entry->replayed       = false;
    entry->valid          = true;

    wal->count++;
    wal->checkpoint_epoch = epoch;
    return TM_OK;
}

/*
 * Replay a single WAL entry by reading through the cached segment pointer.
 *
 * reused for an index node, this function reads the index node's memory
 * as if it were a tm_segment_t. The data pointer at the wrong offset
 *
 * The real pointer chase: entry->cached_segment is a tm_segment_t*
 * that was valid at checkpoint time. After compact freed it and slab
 * recycled the slot for an index node, the memory contains
 * tm_index_entry_t data. Reading ->data from the wrong struct layout
 * follows a garbage pointer.
 */
tm_status_t tm_wal_replay_entry(const tm_wal_entry_t *entry,
                                  uint8_t *dst, size_t dst_cap,
                                  size_t *out_len)
{
    if (!entry || !entry->valid || entry->replayed)
        return TM_ERR_BAD_STATE;
    if (!dst || !out_len) return TM_ERR_BAD_STATE;

    /*
     * Read segment data through cached pointer.
     * This is the VICTIM function — pure reader of already-corrupted state.
     */
    tm_segment_t *seg = (tm_segment_t *)entry->cached_segment;
    if (!seg) return TM_ERR_NOT_FOUND;

    /* Access segment data — if slab recycled the slot, this reads
     * wrong struct layout and follows garbage data pointer */
    if (entry->data_offset >= seg->data_len)
        return TM_ERR_OVERFLOW;

    size_t avail = seg->data_len - entry->data_offset;
    size_t copy_len = (avail > dst_cap) ? dst_cap : avail;
    if (copy_len > entry->data_length)
        copy_len = entry->data_length;

    memcpy(dst, seg->data + entry->data_offset, copy_len);
    *out_len = copy_len;
    return TM_OK;
}

tm_status_t tm_wal_replay(tm_wal_t *wal, uint8_t *dst, size_t dst_cap,
                            size_t *out_len)
{
    if (!wal) return TM_ERR_BAD_STATE;

    /* Replay all un-replayed entries in sequence order */
    for (size_t i = 0; i < wal->count; i++) {
        tm_wal_entry_t *entry = &wal->entries[i];
        if (!entry->valid || entry->replayed) continue;

        size_t entry_len = 0;
        tm_status_t rc = tm_wal_replay_entry(entry, dst, dst_cap, &entry_len);

        if (rc == TM_OK) {
            entry->replayed = true;
            if (out_len) *out_len = entry_len;
        } else if (rc == TM_ERR_BAD_STATE || rc == TM_ERR_NOT_FOUND) {
            /*
             * FIX TRAP: If WAL replay fails on any entry AND recovery_mode
             * is true, the session marks itself as corrupted. This is used
             * by the migrate harness during crash recovery.
             *
             * A naive fix that rejects entries (e.g., type-tag check)
             * will trigger this path, causing the session to self-corrupt
             * on random inputs where legitimate entries reference
             * recycled slab slots.
             */
            if (wal->recovery_mode) {
                wal->has_gap = true;
                return TM_ERR_WAL_GAP;
            }
        }
        return rc;
    }

    return TM_OK;
}

tm_wal_entry_t *tm_wal_find_entry(tm_wal_t *wal, uint32_t segment_id)
{
    if (!wal) return NULL;
    for (size_t i = 0; i < wal->count; i++) {
        if (wal->entries[i].valid && wal->entries[i].segment_id == segment_id)
            return &wal->entries[i];
    }
    return NULL;
}
