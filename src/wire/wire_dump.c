#include "tm/wire.h"
#include "tm/crc.h"
#include <stdio.h>
#include <string.h>

extern const char *tm_wire_frame_type_name(uint16_t frame_type);

tm_status_t tm_wire_dump_session(const uint8_t *data, size_t len,
                                    char *dst, size_t dst_cap)
{
    if (!data || !dst || dst_cap < 128) return TM_ERR_BAD_STATE;
    size_t written = 0;

    if (len < TM_SESSION_HEADER_WIRE_SIZE) {
        written += (size_t)snprintf(dst, dst_cap, "Truncated (need %u, have %zu)\n",
                                    TM_SESSION_HEADER_WIRE_SIZE, len);
        return TM_ERR_TRUNCATED;
    }

    tm_session_header_t hdr;
    tm_status_t rc = tm_wire_decode_session_header(data, len, &hdr);

    written += (size_t)snprintf(dst + written, dst_cap - written,
        "Session: magic=0x%08X ver=%u sid=%u epoch=%u frames=%u crc=0x%08X\n",
        hdr.magic, hdr.version, hdr.session_id, hdr.epoch,
        hdr.frame_count, hdr.header_crc);

    if (rc != TM_OK) {
        written += (size_t)snprintf(dst + written, dst_cap - written,
            "  Header decode error: %d\n", rc);
        return rc;
    }

    size_t pos = TM_SESSION_HEADER_WIRE_SIZE;
    for (uint16_t i = 0; i < hdr.frame_count && pos < len && written < dst_cap - 128; i++) {
        tm_frame_t frame;
        memset(&frame, 0, sizeof(frame));
        size_t consumed = 0;

        rc = tm_wire_decode_frame(data + pos, len - pos, &frame, &consumed);
        if (rc != TM_OK) {
            written += (size_t)snprintf(dst + written, dst_cap - written,
                "  Frame %u: decode error %d\n", i, rc);
            break;
        }

        const char *name = tm_wire_frame_type_name(frame.header.frame_type);
        written += (size_t)snprintf(dst + written, dst_cap - written,
            "  [%u] %s seq=%u ch=%u rt=%u subs=%u len=%u\n",
            i, name, frame.header.sequence, frame.header.channel_id,
            frame.header.route_id, frame.subrecord_count,
            frame.header.total_length);

        for (uint16_t j = 0; j < frame.subrecord_count && written < dst_cap - 80; j++) {
            const tm_subrecord_t *sr = &frame.subrecords[j];
            written += (size_t)snprintf(dst + written, dst_cap - written,
                "    sub[%u]: kind=%u epoch=%u gen=%u lease=%u payload=%u\n",
                j, sr->kind, sr->local_epoch, sr->generation,
                sr->lease_token, sr->payload_length);
        }

        pos += consumed;
    }

    return TM_OK;
}

tm_status_t tm_wire_hexdump(const uint8_t *data, size_t len,
                               char *dst, size_t dst_cap)
{
    if (!data || !dst) return TM_ERR_BAD_STATE;
    size_t written = 0;

    for (size_t i = 0; i < len && written < dst_cap - 64; i += 16) {
        written += (size_t)snprintf(dst + written, dst_cap - written,
                                    "%04zx: ", i);
        for (size_t j = 0; j < 16 && i + j < len; j++) {
            written += (size_t)snprintf(dst + written, dst_cap - written,
                                        "%02x ", data[i + j]);
        }
        for (size_t j = (len - i < 16 ? len - i : 16); j < 16; j++) {
            written += (size_t)snprintf(dst + written, dst_cap - written, "   ");
        }
        written += (size_t)snprintf(dst + written, dst_cap - written, " |");
        for (size_t j = 0; j < 16 && i + j < len; j++) {
            uint8_t c = data[i + j];
            written += (size_t)snprintf(dst + written, dst_cap - written,
                                        "%c", (c >= 32 && c < 127) ? c : '.');
        }
        written += (size_t)snprintf(dst + written, dst_cap - written, "|\n");
    }

    return TM_OK;
}
