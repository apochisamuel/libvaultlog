#include "tm/wire.h"
#include "tm/crc.h"
#include <string.h>

/*
 * Wire protocol validation: deep frame validation,
 * sequence checking, and protocol conformance verification.
 */

typedef struct {
    uint32_t  expected_sequence;
    uint32_t  frames_validated;
    uint32_t  frames_rejected;
    uint32_t  crc_failures;
    uint32_t  sequence_gaps;
} tm_wire_validator_t;

tm_status_t tm_wire_validator_init(tm_wire_validator_t *v)
{
    if (!v) return TM_ERR_BAD_STATE;
    memset(v, 0, sizeof(*v));
    return TM_OK;
}

tm_status_t tm_wire_validate_frame(tm_wire_validator_t *v,
                                      const tm_frame_t *frame)
{
    if (!v || !frame) return TM_ERR_BAD_STATE;

    /* Check frame type range */
    if (frame->header.frame_type == 0 ||
        frame->header.frame_type >= TM_FRAME_TYPE_MAX) {
        v->frames_rejected++;
        return TM_ERR_BAD_FRAME_TYPE;
    }

    /* Check sequence ordering */
    if (frame->header.sequence < v->expected_sequence && v->frames_validated > 0) {
        v->sequence_gaps++;
    }
    v->expected_sequence = frame->header.sequence + 1;

    /* Validate subrecord kinds */
    for (uint16_t i = 0; i < frame->subrecord_count; i++) {
        if (frame->subrecords[i].kind == 0 ||
            frame->subrecords[i].kind >= TM_SUB_KIND_MAX) {
            v->frames_rejected++;
            return TM_ERR_BAD_FRAME;
        }
    }

    v->frames_validated++;
    return TM_OK;
}

tm_status_t tm_wire_validate_session(const uint8_t *data, size_t len)
{
    if (!data || len < TM_SESSION_HEADER_WIRE_SIZE)
        return TM_ERR_TRUNCATED;

    /* Validate magic */
    uint32_t magic = tm_wire_read_u32(data);
    if (magic != TM_MAGIC) return TM_ERR_BAD_MAGIC;

    /* Validate version */
    uint16_t version = tm_wire_read_u16(data + 4);
    if (version != TM_VERSION) return TM_ERR_BAD_VERSION;

    /* Validate header CRC */
    uint32_t stored_crc = tm_wire_read_u32(data + 16);
    uint32_t computed_crc = tm_crc32c(0, data, 16);
    if (stored_crc != computed_crc) return TM_ERR_BAD_CRC;

    /* Validate frame count is reasonable */
    uint16_t frame_count = tm_wire_read_u16(data + 14);
    if (frame_count > 1000) return TM_ERR_OVERFLOW;

    /* Walk frames and validate each */
    size_t pos = TM_SESSION_HEADER_WIRE_SIZE;
    tm_wire_validator_t validator;
    tm_wire_validator_init(&validator);

    for (uint16_t i = 0; i < frame_count && pos < len; i++) {
        tm_frame_t frame;
        memset(&frame, 0, sizeof(frame));
        size_t consumed = 0;

        tm_status_t rc = tm_wire_decode_frame(data + pos, len - pos,
                                                &frame, &consumed);
        if (rc != TM_OK) return rc;

        rc = tm_wire_validate_frame(&validator, &frame);
        if (rc != TM_OK) return rc;

        pos += consumed;
    }

    return TM_OK;
}

size_t tm_wire_compute_message_size(uint16_t frame_count,
                                      const size_t *payload_sizes)
{
    size_t total = TM_SESSION_HEADER_WIRE_SIZE;
    for (uint16_t i = 0; i < frame_count; i++) {
        total += TM_FRAME_HEADER_WIRE_SIZE;
        if (payload_sizes)
            total += payload_sizes[i];
    }
    return total;
}
