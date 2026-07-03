#ifndef TM_WAL_H
#define TM_WAL_H
#include "tm/types.h"
#include "tm/error.h"

typedef struct {
    uint32_t  entry_id;
    uint32_t  entry_type;        /* what operation this records */
    uint32_t  segment_id;        /* which segment this refers to */
    uint32_t  slab_slot_id;      /* slab slot of the segment at checkpoint time */
    void     *cached_segment;    /* RAW pointer to segment (BUG: not validated on replay) */
    uint32_t  epoch;
    uint32_t  sequence;
    uint32_t  data_offset;       /* offset into segment data */
    uint32_t  data_length;       /* length of data referenced */
    uint32_t  checkpoint_gen;    /* generation at checkpoint time */
    bool      replayed;
    bool      valid;
} tm_wal_entry_t;

typedef struct {
    tm_wal_entry_t  entries[TM_MAX_WAL_ENTRIES];
    size_t          count;
    uint32_t        next_id;
    uint32_t        checkpoint_epoch;
    uint32_t        replay_sequence;
    bool            recovery_mode;
    bool            has_gap;         /* true if replay detected a gap */
} tm_wal_t;

tm_status_t  tm_wal_init(tm_wal_t *wal);
void         tm_wal_destroy(tm_wal_t *wal);
tm_status_t  tm_wal_checkpoint(tm_wal_t *wal, uint32_t segment_id,
                                 void *segment_ptr, uint32_t slab_slot,
                                 uint32_t epoch, uint32_t gen,
                                 uint32_t offset, uint32_t length);
tm_status_t  tm_wal_replay(tm_wal_t *wal, uint8_t *dst, size_t dst_cap,
                             size_t *out_len);
tm_status_t  tm_wal_replay_entry(const tm_wal_entry_t *entry,
                                   uint8_t *dst, size_t dst_cap,
                                   size_t *out_len);
tm_wal_entry_t *tm_wal_find_entry(tm_wal_t *wal, uint32_t segment_id);
#endif
