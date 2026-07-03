#include "tm/segment.h"
#include <string.h>
#include <stdlib.h>

typedef struct {
    uint32_t  source_ids[4];
    uint32_t  source_count;
    uint32_t  output_id;
    uint64_t  input_bytes;
    uint64_t  output_bytes;
    bool      success;
} tm_segment_compact_result_t;

tm_status_t tm_segment_compact_pair(tm_segment_table_t *table,
                                       tm_segment_t *a, tm_segment_t *b,
                                       tm_segment_compact_result_t *result)
{
    if (!table || !a || !b || !result) return TM_ERR_BAD_STATE;
    memset(result, 0, sizeof(*result));
    result->source_ids[0] = a->segment_id;
    result->source_ids[1] = b->segment_id;
    result->source_count = 2;
    result->input_bytes = a->data_len + b->data_len;

    tm_segment_t *out = tm_segment_create(table, a->epoch_created, NULL, NULL);
    if (!out) { result->success = false; return TM_ERR_ALLOC; }

    if (a->data && a->data_len > 0) tm_segment_append(out, a->data, a->data_len);
    if (b->data && b->data_len > 0) tm_segment_append(out, b->data, b->data_len);

    out->sealed = true;
    result->output_id = out->segment_id;
    result->output_bytes = out->data_len;
    result->success = true;
    return TM_OK;
}

tm_status_t tm_segment_compact_multi(tm_segment_table_t *table,
                                        const uint32_t *seg_ids,
                                        uint32_t count,
                                        uint32_t *out_id)
{
    if (!table || !seg_ids || count < 2 || !out_id) return TM_ERR_BAD_STATE;

    tm_segment_t *out = tm_segment_create(table, 0, NULL, NULL);
    if (!out) return TM_ERR_ALLOC;

    for (uint32_t i = 0; i < count; i++) {
        tm_segment_t *src = tm_segment_lookup(table, seg_ids[i]);
        if (!src) continue;
        if (src->data && src->data_len > 0)
            tm_segment_append(out, src->data, src->data_len);
    }

    out->sealed = true;
    *out_id = out->segment_id;
    return TM_OK;
}

uint32_t tm_segment_find_oldest(const tm_segment_table_t *table)
{
    if (!table) return 0;
    uint32_t oldest_epoch = UINT32_MAX;
    uint32_t oldest_id = 0;
    for (size_t i = 0; i < TM_MAX_SEGMENTS; i++) {
        const tm_segment_t *s = table->segments[i];
        if (!s || !s->active) continue;
        if (s->epoch_created < oldest_epoch) {
            oldest_epoch = s->epoch_created;
            oldest_id = s->segment_id;
        }
    }
    return oldest_id;
}

uint32_t tm_segment_find_largest(const tm_segment_table_t *table)
{
    if (!table) return 0;
    size_t largest_size = 0;
    uint32_t largest_id = 0;
    for (size_t i = 0; i < TM_MAX_SEGMENTS; i++) {
        const tm_segment_t *s = table->segments[i];
        if (!s || !s->active) continue;
        if (s->data_len > largest_size) {
            largest_size = s->data_len;
            largest_id = s->segment_id;
        }
    }
    return largest_id;
}
