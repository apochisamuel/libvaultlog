#include "tm/wire.h"
#include "tm/crc.h"
#include <string.h>

typedef struct {
    uint32_t  frame_type_counts[TM_FRAME_TYPE_MAX];
    uint32_t  total_frames;
    uint32_t  total_bytes;
    uint32_t  total_subrecords;
    uint32_t  min_frame_size;
    uint32_t  max_frame_size;
    uint32_t  avg_frame_size;
    uint32_t  crc_mismatches;
    uint32_t  truncated_frames;
    uint32_t  invalid_types;
} tm_wire_metrics_t;

tm_status_t tm_wire_metrics_init(tm_wire_metrics_t *m)
{
    if (!m) return TM_ERR_BAD_STATE;
    memset(m, 0, sizeof(*m));
    m->min_frame_size = UINT32_MAX;
    return TM_OK;
}

tm_status_t tm_wire_metrics_record_frame(tm_wire_metrics_t *m,
                                            const tm_frame_t *frame)
{
    if (!m || !frame) return TM_ERR_BAD_STATE;

    if (frame->header.frame_type < TM_FRAME_TYPE_MAX)
        m->frame_type_counts[frame->header.frame_type]++;
    else
        m->invalid_types++;

    m->total_frames++;
    m->total_bytes += frame->header.total_length;
    m->total_subrecords += frame->subrecord_count;

    if (frame->header.total_length < m->min_frame_size)
        m->min_frame_size = frame->header.total_length;
    if (frame->header.total_length > m->max_frame_size)
        m->max_frame_size = frame->header.total_length;

    if (m->total_frames > 0)
        m->avg_frame_size = m->total_bytes / m->total_frames;

    return TM_OK;
}

tm_status_t tm_wire_analyze_message(const uint8_t *data, size_t len,
                                       tm_wire_metrics_t *metrics)
{
    if (!data || !metrics) return TM_ERR_BAD_STATE;
    tm_wire_metrics_init(metrics);

    if (len < TM_SESSION_HEADER_WIRE_SIZE) {
        metrics->truncated_frames++;
        return TM_ERR_TRUNCATED;
    }

    tm_session_header_t hdr;
    tm_status_t rc = tm_wire_decode_session_header(data, len, &hdr);
    if (rc != TM_OK) return rc;

    size_t pos = TM_SESSION_HEADER_WIRE_SIZE;
    for (uint16_t i = 0; i < hdr.frame_count && pos < len; i++) {
        tm_frame_t frame;
        memset(&frame, 0, sizeof(frame));
        size_t consumed = 0;

        rc = tm_wire_decode_frame(data + pos, len - pos, &frame, &consumed);
        if (rc == TM_ERR_BAD_CRC) {
            metrics->crc_mismatches++;
            break;
        }
        if (rc == TM_ERR_TRUNCATED) {
            metrics->truncated_frames++;
            break;
        }
        if (rc != TM_OK) break;

        tm_wire_metrics_record_frame(metrics, &frame);
        pos += consumed;
    }

    return TM_OK;
}

const char *tm_wire_frame_type_name(uint16_t frame_type)
{
    switch (frame_type) {
    case TM_FRAME_SESSION_INIT:     return "SESSION_INIT";
    case TM_FRAME_HEARTBEAT:        return "HEARTBEAT";
    case TM_FRAME_EPOCH_ADVANCE:    return "EPOCH_ADVANCE";
    case TM_FRAME_SEGMENT_CREATE:   return "SEGMENT_CREATE";
    case TM_FRAME_SEGMENT_DESTROY:  return "SEGMENT_DESTROY";
    case TM_FRAME_DATA_APPEND:      return "DATA_APPEND";
    case TM_FRAME_INDEX_INSERT:     return "INDEX_INSERT";
    case TM_FRAME_INDEX_REMOVE:     return "INDEX_REMOVE";
    case TM_FRAME_CURSOR_OPEN:      return "CURSOR_OPEN";
    case TM_FRAME_CURSOR_BIND:      return "CURSOR_BIND";
    case TM_FRAME_CURSOR_SEEK:      return "CURSOR_SEEK";
    case TM_FRAME_CURSOR_READ:      return "CURSOR_READ";
    case TM_FRAME_CURSOR_CLOSE:     return "CURSOR_CLOSE";
    case TM_FRAME_SNAPSHOT_CAPTURE: return "SNAPSHOT_CAPTURE";
    case TM_FRAME_SNAPSHOT_READ:    return "SNAPSHOT_READ";
    case TM_FRAME_SNAPSHOT_RELEASE: return "SNAPSHOT_RELEASE";
    case TM_FRAME_COMPACT_TRIGGER:  return "COMPACT_TRIGGER";
    case TM_FRAME_COMPACT_MERGE:    return "COMPACT_MERGE";
    case TM_FRAME_WAL_CHECKPOINT:   return "WAL_CHECKPOINT";
    case TM_FRAME_WAL_REPLAY:       return "WAL_REPLAY";
    case TM_FRAME_LEASE_ACQUIRE:    return "LEASE_ACQUIRE";
    case TM_FRAME_LEASE_RELEASE:    return "LEASE_RELEASE";
    case TM_FRAME_LEASE_RENEW:      return "LEASE_RENEW";
    case TM_FRAME_RETENTION_SET:    return "RETENTION_SET";
    case TM_FRAME_RETENTION_SWEEP:  return "RETENTION_SWEEP";
    case TM_FRAME_CATALOG_REGISTER: return "CATALOG_REGISTER";
    case TM_FRAME_CATALOG_ACCESS:   return "CATALOG_ACCESS";
    case TM_FRAME_CATALOG_EVICT:    return "CATALOG_EVICT";
    case TM_FRAME_TIDEMARK_ADVANCE: return "TIDEMARK_ADVANCE";
    case TM_FRAME_MIGRATE_PREPARE:  return "MIGRATE_PREPARE";
    case TM_FRAME_MIGRATE_TRANSFER: return "MIGRATE_TRANSFER";
    case TM_FRAME_MIGRATE_COMMIT:   return "MIGRATE_COMMIT";
    case TM_FRAME_LEASE_CHAIN_LINK: return "LEASE_CHAIN_LINK";
    case TM_FRAME_LEASE_CHAIN_WALK: return "LEASE_CHAIN_WALK";
    case TM_FRAME_INDEX_REBUILD:    return "INDEX_REBUILD";
    case TM_FRAME_WATERMARK_SET:    return "WATERMARK_SET";
    default:                        return "UNKNOWN";
    }
}

uint32_t tm_wire_subrecord_total_payload(const tm_frame_t *frame)
{
    if (!frame) return 0;
    uint32_t total = 0;
    for (uint16_t i = 0; i < frame->subrecord_count; i++) {
        total += frame->subrecords[i].payload_length;
    }
    return total;
}
