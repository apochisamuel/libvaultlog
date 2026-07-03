#include "tm/wire.h"
#include "tm/crc.h"
#include <string.h>

typedef struct {
    uint8_t  *buffer;
    size_t    capacity;
    size_t    position;
    uint16_t  frame_count;
} tm_frame_builder_t;

tm_status_t tm_frame_builder_init(tm_frame_builder_t *fb,
                                     uint8_t *buffer, size_t capacity)
{
    if (!fb || !buffer) return TM_ERR_BAD_STATE;
    fb->buffer = buffer;
    fb->capacity = capacity;
    fb->position = TM_SESSION_HEADER_WIRE_SIZE; /* reserve space for header */
    fb->frame_count = 0;
    return TM_OK;
}

tm_status_t tm_frame_builder_add(tm_frame_builder_t *fb,
                                    uint16_t frame_type, uint32_t sequence,
                                    uint32_t session_id, uint32_t channel_id,
                                    uint32_t route_id, uint32_t owner_token,
                                    uint16_t flags,
                                    uint16_t sub_kind, uint32_t sub_epoch,
                                    uint32_t sub_gen, uint32_t sub_lease,
                                    const uint8_t *sub_payload,
                                    uint16_t sub_payload_len)
{
    if (!fb) return TM_ERR_BAD_STATE;

    /* Build subrecord */
    uint8_t sub_buf[512];
    uint16_t sub_total = TM_SUBRECORD_HEADER_WIRE_SIZE + sub_payload_len;
    tm_wire_write_u16(sub_buf, sub_kind);
    tm_wire_write_u16(sub_buf + 2, sub_total);
    tm_wire_write_u32(sub_buf + 4, sub_epoch);
    tm_wire_write_u32(sub_buf + 8, sub_gen);
    tm_wire_write_u32(sub_buf + 12, sub_lease);
    if (sub_payload && sub_payload_len > 0)
        memcpy(sub_buf + TM_SUBRECORD_HEADER_WIRE_SIZE, sub_payload, sub_payload_len);

    uint32_t payload_crc = tm_crc32c(0, sub_buf, sub_total);
    uint32_t total_len = TM_FRAME_HEADER_WIRE_SIZE + sub_total;

    if (fb->position + total_len > fb->capacity) return TM_ERR_OVERFLOW;

    uint8_t *dst = fb->buffer + fb->position;
    size_t pos = 0;
    tm_wire_write_u32(dst + pos, total_len); pos += 4;
    tm_wire_write_u16(dst + pos, frame_type); pos += 2;
    tm_wire_write_u32(dst + pos, sequence); pos += 4;
    tm_wire_write_u32(dst + pos, session_id); pos += 4;
    tm_wire_write_u32(dst + pos, channel_id); pos += 4;
    tm_wire_write_u32(dst + pos, route_id); pos += 4;
    tm_wire_write_u32(dst + pos, owner_token); pos += 4;
    tm_wire_write_u16(dst + pos, flags); pos += 2;
    tm_wire_write_u16(dst + pos, 1); pos += 2; /* 1 subrecord */
    tm_wire_write_u32(dst + pos, payload_crc); pos += 4;
    memcpy(dst + pos, sub_buf, sub_total);

    fb->position += total_len;
    fb->frame_count++;
    return TM_OK;
}

tm_status_t tm_frame_builder_finalize(tm_frame_builder_t *fb,
                                         uint32_t session_id, uint32_t epoch)
{
    if (!fb) return TM_ERR_BAD_STATE;

    /* Write session header at the beginning */
    tm_wire_write_u32(fb->buffer, TM_MAGIC);
    tm_wire_write_u16(fb->buffer + 4, TM_VERSION);
    tm_wire_write_u32(fb->buffer + 6, session_id);
    tm_wire_write_u32(fb->buffer + 10, epoch);
    tm_wire_write_u16(fb->buffer + 14, fb->frame_count);
    uint32_t hdr_crc = tm_crc32c(0, fb->buffer, 16);
    tm_wire_write_u32(fb->buffer + 16, hdr_crc);

    return TM_OK;
}

size_t tm_frame_builder_size(const tm_frame_builder_t *fb)
{
    return fb ? fb->position : 0;
}
