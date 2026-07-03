#ifndef TM_COMPACT_H
#define TM_COMPACT_H
#include "tm/types.h"
#include "tm/error.h"
#include "tm/segment.h"

typedef struct {
    uint32_t  merge_threshold;   /* combined bytes to trigger merge */
    uint32_t  merges_completed;
    uint32_t  segments_freed;
    bool      active;
} tm_compact_ctx_t;

tm_status_t  tm_compact_init(tm_compact_ctx_t *ctx);
tm_status_t  tm_compact_merge_segments(tm_compact_ctx_t *ctx,
                tm_segment_table_t *table, void *slab,
                uint32_t seg_id_a, uint32_t seg_id_b,
                uint32_t epoch);
tm_status_t  tm_compact_trigger(tm_compact_ctx_t *ctx,
                tm_segment_table_t *table, void *slab,
                void *index_ptr, uint32_t epoch);
#endif
