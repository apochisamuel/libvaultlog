#ifndef TM_SEGMENT_H
#define TM_SEGMENT_H
#include "tm/types.h"
#include "tm/error.h"

typedef struct {
    uint32_t  segment_id;
    uint32_t  slab_slot_id;      /* which slab slot this lives in */
    uint32_t  epoch_created;
    uint32_t  generation;        /* incremented on structural changes */
    uint8_t  *data;
    size_t    data_len;
    size_t    data_cap;
    uint32_t  append_count;      /* how many appends have been done */
    uint32_t  byte_count;        /* total bytes appended */
    uint32_t  tidemark_zone;     /* which tidemark zone this belongs to */
    bool      active;
    bool      sealed;            /* no more appends allowed */
    bool      merged;            /* was consumed by a merge operation */
} tm_segment_t;

typedef struct {
    tm_segment_t *segments[TM_MAX_SEGMENTS];
    size_t        count;
    uint32_t      next_id;
} tm_segment_table_t;

tm_status_t    tm_segment_table_init(tm_segment_table_t *table);
void           tm_segment_table_destroy(tm_segment_table_t *table);
tm_segment_t  *tm_segment_create(tm_segment_table_t *table, uint32_t epoch,
                                   void *slab, uint32_t *out_slab_slot);
tm_segment_t  *tm_segment_lookup(tm_segment_table_t *table, uint32_t seg_id);
tm_status_t    tm_segment_append(tm_segment_t *seg, const uint8_t *data, size_t len);
tm_status_t    tm_segment_destroy(tm_segment_table_t *table, tm_segment_t *seg,
                                    void *slab);
size_t         tm_segment_data_size(const tm_segment_t *seg);
#endif
