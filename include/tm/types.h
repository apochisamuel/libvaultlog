#ifndef TM_TYPES_H
#define TM_TYPES_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

/* Wire protocol constants */
#define TM_MAGIC           0x544D4B52  /* TMKR */
#define TM_VERSION         0x0001
#define TM_MAX_PAYLOAD     4096
#define TM_MAX_SUBRECORDS  8
#define TM_MAX_SEGMENTS    32
#define TM_MAX_CURSORS     16
#define TM_MAX_LEASES      16
#define TM_MAX_SNAPSHOTS   8
#define TM_MAX_STREAMS     16
#define TM_MAX_CHANNELS    16
#define TM_MAX_WAL_ENTRIES 64
#define TM_MAX_INDEX_NODES 64
#define TM_SLAB_CAPACITY   128

/* Wire sizes */
#define TM_SESSION_HEADER_WIRE_SIZE   20
#define TM_FRAME_HEADER_WIRE_SIZE     34
#define TM_SUBRECORD_HEADER_WIRE_SIZE 16

/* Frame types */
typedef enum {
    TM_FRAME_SESSION_INIT       = 0x0001,
    TM_FRAME_HEARTBEAT          = 0x0002,
    TM_FRAME_EPOCH_ADVANCE      = 0x0003,
    TM_FRAME_SEGMENT_CREATE     = 0x0004,
    TM_FRAME_SEGMENT_DESTROY    = 0x0005,
    TM_FRAME_DATA_APPEND        = 0x0006,
    TM_FRAME_INDEX_INSERT       = 0x0007,
    TM_FRAME_INDEX_REMOVE       = 0x0008,
    TM_FRAME_CURSOR_OPEN        = 0x0009,
    TM_FRAME_CURSOR_BIND        = 0x000A,
    TM_FRAME_CURSOR_SEEK        = 0x000B,
    TM_FRAME_CURSOR_READ        = 0x000C,
    TM_FRAME_CURSOR_CLOSE       = 0x000D,
    TM_FRAME_SNAPSHOT_CAPTURE   = 0x000E,
    TM_FRAME_SNAPSHOT_READ      = 0x000F,
    TM_FRAME_SNAPSHOT_RELEASE   = 0x0010,
    TM_FRAME_COMPACT_TRIGGER    = 0x0011,
    TM_FRAME_COMPACT_MERGE      = 0x0012,
    TM_FRAME_WAL_CHECKPOINT     = 0x0013,
    TM_FRAME_WAL_REPLAY         = 0x0014,
    TM_FRAME_LEASE_ACQUIRE      = 0x0015,
    TM_FRAME_LEASE_RELEASE      = 0x0016,
    TM_FRAME_LEASE_RENEW        = 0x0017,
    TM_FRAME_RETENTION_SET      = 0x0018,
    TM_FRAME_RETENTION_SWEEP    = 0x0019,
    TM_FRAME_CATALOG_REGISTER   = 0x001A,
    TM_FRAME_CATALOG_ACCESS     = 0x001B,
    TM_FRAME_CATALOG_EVICT      = 0x001C,
    TM_FRAME_TIDEMARK_ADVANCE   = 0x001D,
    TM_FRAME_MIGRATE_PREPARE    = 0x001E,
    TM_FRAME_MIGRATE_TRANSFER   = 0x001F,
    TM_FRAME_MIGRATE_COMMIT     = 0x0020,
    TM_FRAME_QUERY_EXECUTE      = 0x0021,
    TM_FRAME_ADMIN_DISPATCH     = 0x0022,
    TM_FRAME_LEASE_CHAIN_LINK   = 0x0023,
    TM_FRAME_LEASE_CHAIN_WALK   = 0x0024,
    TM_FRAME_INDEX_REBUILD      = 0x0025,
    TM_FRAME_WATERMARK_SET      = 0x0026,
    TM_FRAME_TYPE_MAX
} tm_frame_type_t;

/* Subrecord kinds */
typedef enum {
    TM_SUB_CONFIG        = 0x0001,
    TM_SUB_SEGMENT_SPEC  = 0x0002,
    TM_SUB_DATA_PAYLOAD  = 0x0003,
    TM_SUB_INDEX_SPEC    = 0x0004,
    TM_SUB_CURSOR_SPEC   = 0x0005,
    TM_SUB_SNAPSHOT_SPEC = 0x0006,
    TM_SUB_COMPACT_SPEC  = 0x0007,
    TM_SUB_WAL_SPEC      = 0x0008,
    TM_SUB_LEASE_SPEC    = 0x0009,
    TM_SUB_RETENTION_SPEC= 0x000A,
    TM_SUB_CATALOG_SPEC  = 0x000B,
    TM_SUB_TIDEMARK_SPEC = 0x000C,
    TM_SUB_MIGRATE_SPEC  = 0x000D,
    TM_SUB_EPOCH_SPEC    = 0x000E,
    TM_SUB_CHECKPOINT    = 0x000F,
    TM_SUB_QUERY_SPEC    = 0x0010,
    TM_SUB_CHAIN_SPEC    = 0x0011,
    TM_SUB_WATERMARK_SPEC= 0x0012,
    TM_SUB_KIND_MAX
} tm_subrecord_kind_t;

/* Wire structures */
typedef struct {
    uint32_t magic;
    uint16_t version;
    uint32_t session_id;
    uint32_t epoch;
    uint16_t frame_count;
    uint32_t header_crc;
} tm_session_header_t;

typedef struct {
    uint16_t kind;
    uint16_t length;
    uint32_t local_epoch;
    uint32_t generation;
    uint32_t lease_token;
    uint8_t *payload;
    uint16_t payload_length;
} tm_subrecord_t;

typedef struct {
    tm_session_header_t session;
    struct {
        uint32_t  total_length;
        uint16_t  frame_type;
        uint32_t  sequence;
        uint32_t  session_id;
        uint32_t  channel_id;
        uint32_t  route_id;
        uint32_t  owner_token;
        uint16_t  flags;
        uint16_t  subrecord_count;
        uint32_t  payload_crc;
    } header;
    tm_subrecord_t subrecords[TM_MAX_SUBRECORDS];
    uint16_t       subrecord_count;
} tm_frame_t;

#endif /* TM_TYPES_H */
