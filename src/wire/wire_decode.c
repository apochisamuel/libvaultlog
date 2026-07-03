#include "tm/wire.h"
#include "tm/crc.h"
#include <string.h>

void tm_wire_write_u16(uint8_t *d, uint16_t v) { d[0]=v&0xFF; d[1]=(v>>8)&0xFF; }
void tm_wire_write_u32(uint8_t *d, uint32_t v) { d[0]=v&0xFF; d[1]=(v>>8)&0xFF; d[2]=(v>>16)&0xFF; d[3]=(v>>24)&0xFF; }
uint16_t tm_wire_read_u16(const uint8_t *s) { return (uint16_t)s[0] | ((uint16_t)s[1]<<8); }
uint32_t tm_wire_read_u32(const uint8_t *s) { return (uint32_t)s[0] | ((uint32_t)s[1]<<8) | ((uint32_t)s[2]<<16) | ((uint32_t)s[3]<<24); }

tm_status_t tm_wire_decode_session_header(const uint8_t *data, size_t len,
                                            tm_session_header_t *out)
{
    if (len < TM_SESSION_HEADER_WIRE_SIZE) return TM_ERR_TRUNCATED;
    out->magic       = tm_wire_read_u32(data);
    out->version     = tm_wire_read_u16(data + 4);
    out->session_id  = tm_wire_read_u32(data + 6);
    out->epoch       = tm_wire_read_u32(data + 10);
    out->frame_count = tm_wire_read_u16(data + 14);
    out->header_crc  = tm_wire_read_u32(data + 16);

    if (out->magic != TM_MAGIC) return TM_ERR_BAD_MAGIC;
    if (out->version != TM_VERSION) return TM_ERR_BAD_VERSION;

    uint32_t computed = tm_crc32c(0, data, 16);
    if (computed != out->header_crc) return TM_ERR_BAD_CRC;

    return TM_OK;
}

tm_status_t tm_wire_decode_frame(const uint8_t *data, size_t len,
                                   tm_frame_t *out, size_t *consumed)
{
    if (len < TM_FRAME_HEADER_WIRE_SIZE) return TM_ERR_TRUNCATED;

    size_t pos = 0;
    out->header.total_length    = tm_wire_read_u32(data + pos); pos += 4;
    out->header.frame_type      = tm_wire_read_u16(data + pos); pos += 2;
    out->header.sequence        = tm_wire_read_u32(data + pos); pos += 4;
    out->header.session_id      = tm_wire_read_u32(data + pos); pos += 4;
    out->header.channel_id      = tm_wire_read_u32(data + pos); pos += 4;
    out->header.route_id        = tm_wire_read_u32(data + pos); pos += 4;
    out->header.owner_token     = tm_wire_read_u32(data + pos); pos += 4;
    out->header.flags           = tm_wire_read_u16(data + pos); pos += 2;
    out->header.subrecord_count = tm_wire_read_u16(data + pos); pos += 2;
    out->header.payload_crc     = tm_wire_read_u32(data + pos); pos += 4;

    if (out->header.frame_type == 0 ||
        out->header.frame_type >= TM_FRAME_TYPE_MAX)
        return TM_ERR_BAD_FRAME_TYPE;

    if (out->header.total_length < TM_FRAME_HEADER_WIRE_SIZE)
        return TM_ERR_TRUNCATED;
    if (out->header.total_length > len)
        return TM_ERR_TRUNCATED;

    size_t payload_start = pos;
    size_t payload_len = out->header.total_length - TM_FRAME_HEADER_WIRE_SIZE;
    uint16_t sr_count = out->header.subrecord_count;
    if (sr_count > TM_MAX_SUBRECORDS) sr_count = TM_MAX_SUBRECORDS;
    out->subrecord_count = 0;

    size_t sr_pos = 0;
    for (uint16_t i = 0; i < sr_count && sr_pos + TM_SUBRECORD_HEADER_WIRE_SIZE <= payload_len; i++) {
        const uint8_t *sr_data = data + payload_start + sr_pos;
        tm_subrecord_t *sr = &out->subrecords[i];
        sr->kind           = tm_wire_read_u16(sr_data);
        sr->length         = tm_wire_read_u16(sr_data + 2);
        sr->local_epoch    = tm_wire_read_u32(sr_data + 4);
        sr->generation     = tm_wire_read_u32(sr_data + 8);
        sr->lease_token    = tm_wire_read_u32(sr_data + 12);

        uint16_t sr_pl = 0;
        if (sr->length > TM_SUBRECORD_HEADER_WIRE_SIZE)
            sr_pl = sr->length - TM_SUBRECORD_HEADER_WIRE_SIZE;
        if (sr_pl > 0 && sr_pos + TM_SUBRECORD_HEADER_WIRE_SIZE + sr_pl <= payload_len) {
            sr->payload = (uint8_t *)(sr_data + TM_SUBRECORD_HEADER_WIRE_SIZE);
            sr->payload_length = sr_pl;
        } else {
            sr->payload = NULL;
            sr->payload_length = 0;
        }
        sr_pos += (sr->length > TM_SUBRECORD_HEADER_WIRE_SIZE) ?
                  sr->length : TM_SUBRECORD_HEADER_WIRE_SIZE;
        out->subrecord_count++;
    }

    if (payload_len > 0) {
        uint32_t pcrc = tm_crc32c(0, data + payload_start, payload_len);
        if (pcrc != out->header.payload_crc) return TM_ERR_BAD_CRC;
    }

    *consumed = out->header.total_length;
    return TM_OK;
}
