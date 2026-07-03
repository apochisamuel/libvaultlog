#include "tm/segment.h"
#include "tm/crc.h"
#include <string.h>

/*
 * Segment validation and integrity checking.
 */

typedef struct {
    uint32_t  segment_id;
    uint32_t  data_checksum;
    size_t    data_length;
    bool      checksum_valid;
    bool      structure_valid;
} tm_segment_validation_t;

tm_status_t tm_segment_validate(const tm_segment_t *seg,
                                   tm_segment_validation_t *result)
{
    if (!seg || !result) return TM_ERR_BAD_STATE;
    memset(result, 0, sizeof(*result));

    result->segment_id = seg->segment_id;
    result->data_length = seg->data_len;
    result->structure_valid = true;

    /* Validate data integrity */
    if (seg->data && seg->data_len > 0) {
        result->data_checksum = tm_crc32c(0, seg->data, seg->data_len);
        result->checksum_valid = true;
    }

    /* Validate structural consistency */
    if (seg->data_len > seg->data_cap)
        result->structure_valid = false;
    if (seg->byte_count > seg->data_len + 1024)
        result->structure_valid = false;

    return TM_OK;
}

tm_status_t tm_segment_table_validate(const tm_segment_table_t *table,
                                         uint32_t *valid_count,
                                         uint32_t *invalid_count)
{
    if (!table || !valid_count || !invalid_count) return TM_ERR_BAD_STATE;
    *valid_count = 0;
    *invalid_count = 0;

    for (size_t i = 0; i < TM_MAX_SEGMENTS; i++) {
        const tm_segment_t *s = table->segments[i];
        if (!s) continue;

        tm_segment_validation_t v;
        tm_segment_validate(s, &v);
        if (v.structure_valid && v.checksum_valid)
            (*valid_count)++;
        else
            (*invalid_count)++;
    }

    return TM_OK;
}

bool tm_segment_data_matches(const tm_segment_t *a, const tm_segment_t *b)
{
    if (!a || !b) return false;
    if (a->data_len != b->data_len) return false;
    if (!a->data || !b->data) return (a->data == b->data);
    return memcmp(a->data, b->data, a->data_len) == 0;
}

uint32_t tm_segment_compute_checksum(const tm_segment_t *seg)
{
    if (!seg || !seg->data || seg->data_len == 0) return 0;
    return tm_crc32c(0, seg->data, seg->data_len);
}
