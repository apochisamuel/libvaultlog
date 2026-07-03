#include "tm/session.h"
#include "tm/wire.h"
#include "tm/crc.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>

/* Comprehensive end-to-end test suite for libtidemark */

static uint8_t test_buf[8192];
static size_t test_pos;

static void test_reset(void) { test_pos = 0; memset(test_buf, 0, sizeof(test_buf)); }

static void test_write_header(uint32_t sid, uint32_t epoch, uint16_t fc) {
    tm_wire_write_u32(test_buf + test_pos, TM_MAGIC); test_pos += 4;
    tm_wire_write_u16(test_buf + test_pos, TM_VERSION); test_pos += 2;
    tm_wire_write_u32(test_buf + test_pos, sid); test_pos += 4;
    tm_wire_write_u32(test_buf + test_pos, epoch); test_pos += 4;
    tm_wire_write_u16(test_buf + test_pos, fc); test_pos += 2;
    uint32_t crc = tm_crc32c(0, test_buf, 16);
    tm_wire_write_u32(test_buf + test_pos, crc); test_pos += 4;
}

static void test_write_frame(uint16_t ft, uint32_t sq, uint32_t si,
                             uint32_t ci, uint32_t ri, uint32_t ow,
                             uint16_t sk, uint32_t se, uint32_t sg, uint32_t sl,
                             const uint8_t *sp, uint16_t spl) {
    uint8_t sb[512]; size_t sn = 0;
    tm_wire_write_u16(sb+sn,sk);sn+=2;
    uint16_t st=TM_SUBRECORD_HEADER_WIRE_SIZE+spl;
    tm_wire_write_u16(sb+sn,st);sn+=2;
    tm_wire_write_u32(sb+sn,se);sn+=4;tm_wire_write_u32(sb+sn,sg);sn+=4;
    tm_wire_write_u32(sb+sn,sl);sn+=4;
    if(sp&&spl>0){memcpy(sb+sn,sp,spl);sn+=spl;}
    uint32_t pc=tm_crc32c(0,sb,sn);
    uint32_t tl=TM_FRAME_HEADER_WIRE_SIZE+(uint32_t)sn;
    tm_wire_write_u32(test_buf+test_pos,tl);test_pos+=4;
    tm_wire_write_u16(test_buf+test_pos,ft);test_pos+=2;
    tm_wire_write_u32(test_buf+test_pos,sq);test_pos+=4;
    tm_wire_write_u32(test_buf+test_pos,si);test_pos+=4;
    tm_wire_write_u32(test_buf+test_pos,ci);test_pos+=4;
    tm_wire_write_u32(test_buf+test_pos,ri);test_pos+=4;
    tm_wire_write_u32(test_buf+test_pos,ow);test_pos+=4;
    tm_wire_write_u16(test_buf+test_pos,0);test_pos+=2;
    tm_wire_write_u16(test_buf+test_pos,1);test_pos+=2;
    tm_wire_write_u32(test_buf+test_pos,pc);test_pos+=4;
    memcpy(test_buf+test_pos,sb,sn);test_pos+=sn;
}

static void test_full_lifecycle(void) {
    test_reset();
    uint8_t data[64]; memset(data, 0xEE, sizeof(data));
    test_write_header(1, 0, 8);
    test_write_frame(TM_FRAME_SESSION_INIT,  0,1,0,0,0xBEEF,TM_SUB_CONFIG,0,0,0,NULL,0);
    test_write_frame(TM_FRAME_SEGMENT_CREATE,1,1,0,0,0xBEEF,TM_SUB_SEGMENT_SPEC,0,0,0,NULL,0);
    test_write_frame(TM_FRAME_DATA_APPEND,   2,1,0,1,0xBEEF,TM_SUB_DATA_PAYLOAD,0,0,0,data,sizeof(data));
    test_write_frame(TM_FRAME_CURSOR_OPEN,   3,1,1,0,0xBEEF,TM_SUB_CURSOR_SPEC,0,0,0,NULL,0);
    test_write_frame(TM_FRAME_CURSOR_BIND,   4,1,1,1,0xBEEF,TM_SUB_CURSOR_SPEC,0,0,0,NULL,0);
    test_write_frame(TM_FRAME_CURSOR_READ,   5,1,1,0,0xBEEF,TM_SUB_CURSOR_SPEC,0,0,0,NULL,0);
    test_write_frame(TM_FRAME_CURSOR_CLOSE,  6,1,1,0,0xBEEF,TM_SUB_CURSOR_SPEC,0,0,0,NULL,0);
    test_write_frame(TM_FRAME_HEARTBEAT,     7,1,0,0,0xBEEF,TM_SUB_CONFIG,0,0,0,NULL,0);

    tm_session_t *sess = tm_session_create(1);
    tm_status_t rc = tm_session_ingest(sess, test_buf, test_pos);
    assert(rc == TM_OK);
    assert(sess->active);
    assert(sess->segments.count == 1);
    tm_session_destroy(sess);
    printf("  full lifecycle: PASS\n");
}

static void test_snapshot_lifecycle(void) {
    test_reset();
    uint8_t data[32]; memset(data, 0xFF, sizeof(data));
    test_write_header(1, 0, 5);
    test_write_frame(TM_FRAME_SESSION_INIT,    0,1,0,0,0xBEEF,TM_SUB_CONFIG,0,0,0,NULL,0);
    test_write_frame(TM_FRAME_SEGMENT_CREATE,  1,1,0,0,0xBEEF,TM_SUB_SEGMENT_SPEC,0,0,0,NULL,0);
    test_write_frame(TM_FRAME_DATA_APPEND,     2,1,0,1,0xBEEF,TM_SUB_DATA_PAYLOAD,0,0,0,data,sizeof(data));
    test_write_frame(TM_FRAME_SNAPSHOT_CAPTURE,3,1,1,0,0xBEEF,TM_SUB_SNAPSHOT_SPEC,0,0,0,NULL,0);
    test_write_frame(TM_FRAME_SNAPSHOT_READ,   4,1,1,1,0xBEEF,TM_SUB_SNAPSHOT_SPEC,0,0,0,NULL,0);

    tm_session_t *sess = tm_session_create(1);
    tm_session_ingest(sess, test_buf, test_pos);
    assert(sess->snapshots[0] != NULL);
    tm_session_destroy(sess);
    printf("  snapshot lifecycle: PASS\n");
}

static void test_lease_lifecycle(void) {
    test_reset();
    test_write_header(1, 0, 5);
    test_write_frame(TM_FRAME_SESSION_INIT,  0,1,0,0,0xBEEF,TM_SUB_CONFIG,0,0,0,NULL,0);
    test_write_frame(TM_FRAME_SEGMENT_CREATE,1,1,0,0,0xBEEF,TM_SUB_SEGMENT_SPEC,0,0,0,NULL,0);
    test_write_frame(TM_FRAME_LEASE_ACQUIRE, 2,1,0,1,0xBEEF,TM_SUB_LEASE_SPEC,0,4,0,NULL,0);
    test_write_frame(TM_FRAME_LEASE_RENEW,   3,1,0,1,0xBEEF,TM_SUB_LEASE_SPEC,0,3,0,NULL,0);
    test_write_frame(TM_FRAME_LEASE_RELEASE, 4,1,0,1,0xBEEF,TM_SUB_LEASE_SPEC,0,0,0,NULL,0);

    tm_session_t *sess = tm_session_create(1);
    tm_session_ingest(sess, test_buf, test_pos);
    tm_session_destroy(sess);
    printf("  lease lifecycle: PASS\n");
}

static void test_wal_lifecycle(void) {
    test_reset();
    uint8_t data[32]; memset(data, 0xAA, sizeof(data));
    test_write_header(1, 0, 5);
    test_write_frame(TM_FRAME_SESSION_INIT,   0,1,0,0,0xBEEF,TM_SUB_CONFIG,0,0,0,NULL,0);
    test_write_frame(TM_FRAME_SEGMENT_CREATE, 1,1,0,0,0xBEEF,TM_SUB_SEGMENT_SPEC,0,0,0,NULL,0);
    test_write_frame(TM_FRAME_DATA_APPEND,    2,1,0,1,0xBEEF,TM_SUB_DATA_PAYLOAD,0,0,0,data,sizeof(data));
    test_write_frame(TM_FRAME_WAL_CHECKPOINT, 3,1,0,1,0xBEEF,TM_SUB_WAL_SPEC,0,0,0,NULL,0);
    test_write_frame(TM_FRAME_WAL_REPLAY,     4,1,0,0,0xBEEF,TM_SUB_WAL_SPEC,0,0,0,NULL,0);

    tm_session_t *sess = tm_session_create(1);
    tm_session_ingest(sess, test_buf, test_pos);
    assert(sess->wal.count == 1);
    assert(sess->wal.entries[0].replayed);
    tm_session_destroy(sess);
    printf("  WAL lifecycle: PASS\n");
}

static void test_compact_lifecycle(void) {
    test_reset();
    uint8_t data[140]; memset(data, 0xBB, sizeof(data));
    test_write_header(1, 0, 8);
    test_write_frame(TM_FRAME_SESSION_INIT,   0,1,0,0,0xBEEF,TM_SUB_CONFIG,0,0,0,NULL,0);
    test_write_frame(TM_FRAME_SEGMENT_CREATE, 1,1,0,0,0xBEEF,TM_SUB_SEGMENT_SPEC,0,0,0,NULL,0);
    test_write_frame(TM_FRAME_SEGMENT_CREATE, 2,1,0,0,0xBEEF,TM_SUB_SEGMENT_SPEC,0,0,0,NULL,0);
    test_write_frame(TM_FRAME_DATA_APPEND,    3,1,0,1,0xBEEF,TM_SUB_DATA_PAYLOAD,0,0,0,data,sizeof(data));
    test_write_frame(TM_FRAME_DATA_APPEND,    4,1,0,1,0xBEEF,TM_SUB_DATA_PAYLOAD,0,0,0,data,sizeof(data));
    test_write_frame(TM_FRAME_DATA_APPEND,    5,1,0,2,0xBEEF,TM_SUB_DATA_PAYLOAD,0,0,0,data,sizeof(data));
    test_write_frame(TM_FRAME_DATA_APPEND,    6,1,0,2,0xBEEF,TM_SUB_DATA_PAYLOAD,0,0,0,data,sizeof(data));
    test_write_frame(TM_FRAME_COMPACT_TRIGGER,7,1,0,0,0xBEEF,TM_SUB_COMPACT_SPEC,0,0,0,NULL,0);

    tm_session_t *sess = tm_session_create(1);
    tm_session_ingest(sess, test_buf, test_pos);
    assert(sess->compact.merges_completed == 1);
    tm_session_destroy(sess);
    printf("  compact lifecycle: PASS\n");
}

int main(void) {
    printf("End-to-end tests:\n");
    test_full_lifecycle();
    test_snapshot_lifecycle();
    test_lease_lifecycle();
    test_wal_lifecycle();
    test_compact_lifecycle();
    printf("All e2e tests passed.\n");
    return 0;
}
