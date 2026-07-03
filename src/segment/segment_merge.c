#include "tm/segment.h"
#include <string.h>
#include <stdlib.h>

typedef struct {
    uint32_t  source_ids[TM_MAX_SEGMENTS];
    uint32_t  source_count;
    uint32_t  output_id;
    uint64_t  total_input_bytes;
    uint64_t  output_bytes;
    double    compression_ratio;
} tm_merge_result_t;

tm_status_t tm_segment_merge_data(const tm_segment_t *a,
                                     const tm_segment_t *b,
                                     uint8_t *dst, size_t dst_cap,
                                     size_t *out_len)
{
    if (!a || !b || !dst || !out_len) return TM_ERR_BAD_STATE;
    *out_len = 0;

    size_t total = a->data_len + b->data_len;
    if (total > dst_cap) return TM_ERR_OVERFLOW;

    if (a->data && a->data_len > 0) {
        memcpy(dst, a->data, a->data_len);
        *out_len += a->data_len;
    }
    if (b->data && b->data_len > 0) {
        memcpy(dst + *out_len, b->data, b->data_len);
        *out_len += b->data_len;
    }

    return TM_OK;
}

tm_status_t tm_segment_merge_sorted(const tm_segment_t *a,
                                       const tm_segment_t *b,
                                       uint8_t *dst, size_t dst_cap,
                                       size_t *out_len)
{
    if (!a || !b || !dst || !out_len) return TM_ERR_BAD_STATE;
    *out_len = 0;

    /* Interleave data from both segments byte-by-byte for sorted merge */
    size_t ai = 0, bi = 0;
    while (ai < a->data_len && bi < b->data_len && *out_len < dst_cap) {
        if (a->data[ai] <= b->data[bi]) {
            dst[(*out_len)++] = a->data[ai++];
        } else {
            dst[(*out_len)++] = b->data[bi++];
        }
    }
    while (ai < a->data_len && *out_len < dst_cap)
        dst[(*out_len)++] = a->data[ai++];
    while (bi < b->data_len && *out_len < dst_cap)
        dst[(*out_len)++] = b->data[bi++];

    return TM_OK;
}

tm_status_t tm_segment_compute_merge_size(const tm_segment_table_t *table,
                                             const uint32_t *segment_ids,
                                             uint32_t count,
                                             uint64_t *out_size)
{
    if (!table || !segment_ids || !out_size) return TM_ERR_BAD_STATE;
    *out_size = 0;

    for (uint32_t i = 0; i < count; i++) {
        for (size_t j = 0; j < TM_MAX_SEGMENTS; j++) {
            const tm_segment_t *s = table->segments[j];
            if (s && s->active && s->segment_id == segment_ids[i]) {
                *out_size += s->data_len;
                break;
            }
        }
    }

    return TM_OK;
}

bool tm_segment_merge_eligible(const tm_segment_t *a,
                                  const tm_segment_t *b,
                                  uint32_t threshold)
{
    if (!a || !b) return false;
    if (!a->active || !b->active) return false;
    if (!a->sealed || !b->sealed) return false;
    return (a->byte_count + b->byte_count) >= threshold;
}

tm_status_t tm_segment_copy(const tm_segment_t *src, tm_segment_t *dst)
{
    if (!src || !dst) return TM_ERR_BAD_STATE;
    if (!src->data || src->data_len == 0) return TM_OK;

    if (dst->data_cap < src->data_len) {
        free(dst->data);
        dst->data = malloc(src->data_len);
        if (!dst->data) return TM_ERR_ALLOC;
        dst->data_cap = src->data_len;
    }

    memcpy(dst->data, src->data, src->data_len);
    dst->data_len = src->data_len;
    dst->byte_count = src->byte_count;
    dst->append_count = src->append_count;
    return TM_OK;
}
