#include "tm/wire.h"
#include "tm/crc.h"
#include <string.h>

/*
 * Wire encoding utilities for constructing protocol frames.
 * Used by tests, examples, and migration paths.
 */

tm_status_t tm_wire_encode_session_header(uint8_t *dst, size_t dst_cap,
                                             uint32_t session_id,
                                             uint32_t epoch,
                                             uint16_t frame_count)
{
    if (!dst || dst_cap < TM_SESSION_HEADER_WIRE_SIZE) return TM_ERR_TRUNCATED;

    tm_wire_write_u32(dst, TM_MAGIC);
    tm_wire_write_u16(dst + 4, TM_VERSION);
    tm_wire_write_u32(dst + 6, session_id);
    tm_wire_write_u32(dst + 10, epoch);
    tm_wire_write_u16(dst + 14, frame_count);
    uint32_t crc = tm_crc32c(0, dst, 16);
    tm_wire_write_u32(dst + 16, crc);

    return TM_OK;
}

tm_status_t tm_wire_encode_subrecord(uint8_t *dst, size_t dst_cap,
                                        uint16_t kind, uint32_t local_epoch,
                                        uint32_t generation, uint32_t lease_token,
                                        const uint8_t *payload, uint16_t payload_len,
                                        size_t *out_len)
{
    uint16_t total = TM_SUBRECORD_HEADER_WIRE_SIZE + payload_len;
    if (!dst || dst_cap < total) return TM_ERR_TRUNCATED;

    tm_wire_write_u16(dst, kind);
    tm_wire_write_u16(dst + 2, total);
    tm_wire_write_u32(dst + 4, local_epoch);
    tm_wire_write_u32(dst + 8, generation);
    tm_wire_write_u32(dst + 12, lease_token);

    if (payload && payload_len > 0)
        memcpy(dst + TM_SUBRECORD_HEADER_WIRE_SIZE, payload, payload_len);

    *out_len = total;
    return TM_OK;
}

tm_status_t tm_wire_encode_frame(uint8_t *dst, size_t dst_cap,
                                    uint16_t frame_type, uint32_t sequence,
                                    uint32_t session_id, uint32_t channel_id,
                                    uint32_t route_id, uint32_t owner_token,
                                    uint16_t flags,
                                    const uint8_t *subrecord_data,
                                    size_t subrecord_len,
                                    uint16_t subrecord_count,
                                    size_t *out_len)
{
    uint32_t total = TM_FRAME_HEADER_WIRE_SIZE + (uint32_t)subrecord_len;
    if (!dst || dst_cap < total) return TM_ERR_TRUNCATED;

    size_t pos = 0;
    tm_wire_write_u32(dst + pos, total); pos += 4;
    tm_wire_write_u16(dst + pos, frame_type); pos += 2;
    tm_wire_write_u32(dst + pos, sequence); pos += 4;
    tm_wire_write_u32(dst + pos, session_id); pos += 4;
    tm_wire_write_u32(dst + pos, channel_id); pos += 4;
    tm_wire_write_u32(dst + pos, route_id); pos += 4;
    tm_wire_write_u32(dst + pos, owner_token); pos += 4;
    tm_wire_write_u16(dst + pos, flags); pos += 2;
    tm_wire_write_u16(dst + pos, subrecord_count); pos += 2;

    /* Compute payload CRC */
    uint32_t pcrc = 0;
    if (subrecord_data && subrecord_len > 0) {
        pcrc = tm_crc32c(0, subrecord_data, subrecord_len);
        memcpy(dst + TM_FRAME_HEADER_WIRE_SIZE, subrecord_data, subrecord_len);
    }
    tm_wire_write_u32(dst + pos, pcrc);

    *out_len = total;
    return TM_OK;
}
