#include "tm/session.h"
#include "tm/wire.h"
#include "tm/crc.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>

static uint8_t buf[8192];
static size_t pos;

static void reset(void) { pos = 0; memset(buf, 0, sizeof(buf)); }

static void write_header(uint32_t sid, uint32_t epoch, uint16_t fc) {
    tm_wire_write_u32(buf + pos, TM_MAGIC); pos += 4;
    tm_wire_write_u16(buf + pos, TM_VERSION); pos += 2;
    tm_wire_write_u32(buf + pos, sid); pos += 4;
    tm_wire_write_u32(buf + pos, epoch); pos += 4;
    tm_wire_write_u16(buf + pos, fc); pos += 2;
    uint32_t crc = tm_crc32c(0, buf, 16);
    tm_wire_write_u32(buf + pos, crc); pos += 4;
}

static void write_frame(uint16_t ft, uint32_t sq, uint32_t si,
                        uint32_t ci, uint32_t ri, uint32_t ow,
                        uint16_t sk, uint32_t se, uint32_t sg, uint32_t sl,
                        const uint8_t *sp, uint16_t spl) {
    uint8_t sb[512];
    size_t sn = 0;
    tm_wire_write_u16(sb + sn, sk); sn += 2;
    uint16_t st = TM_SUBRECORD_HEADER_WIRE_SIZE + spl;
    tm_wire_write_u16(sb + sn, st); sn += 2;
    tm_wire_write_u32(sb + sn, se); sn += 4;
    tm_wire_write_u32(sb + sn, sg); sn += 4;
    tm_wire_write_u32(sb + sn, sl); sn += 4;
    if (sp && spl > 0) { memcpy(sb + sn, sp, spl); sn += spl; }
    uint32_t pc = tm_crc32c(0, sb, sn);
    uint32_t tl = TM_FRAME_HEADER_WIRE_SIZE + (uint32_t)sn;
    tm_wire_write_u32(buf+pos, tl); pos+=4;
    tm_wire_write_u16(buf+pos, ft); pos+=2;
    tm_wire_write_u32(buf+pos, sq); pos+=4;
    tm_wire_write_u32(buf+pos, si); pos+=4;
    tm_wire_write_u32(buf+pos, ci); pos+=4;
    tm_wire_write_u32(buf+pos, ri); pos+=4;
    tm_wire_write_u32(buf+pos, ow); pos+=4;
    tm_wire_write_u16(buf+pos, 0); pos+=2;
    tm_wire_write_u16(buf+pos, 1); pos+=2;
    tm_wire_write_u32(buf+pos, pc); pos+=4;
    memcpy(buf+pos, sb, sn); pos+=sn;
}

static void test_session_init(void) {
    reset();
    write_header(1, 0, 1);
    write_frame(TM_FRAME_SESSION_INIT, 0, 1, 0, 0, 0xDEAD,
                TM_SUB_CONFIG, 0, 0, 0, NULL, 0);

    tm_session_t *sess = tm_session_create(1);
    tm_status_t rc = tm_session_ingest(sess, buf, pos);
    assert(rc == TM_OK);
    assert(sess->active);
    tm_session_destroy(sess);
    printf("  session init via wire: PASS\n");
}

static void test_segment_lifecycle(void) {
    reset();
    uint8_t data[64];
    memset(data, 0xCC, sizeof(data));
    write_header(1, 0, 3);
    write_frame(TM_FRAME_SESSION_INIT,  0, 1, 0, 0, 0xDEAD,
                TM_SUB_CONFIG, 0, 0, 0, NULL, 0);
    write_frame(TM_FRAME_SEGMENT_CREATE, 1, 1, 0, 0, 0xDEAD,
                TM_SUB_SEGMENT_SPEC, 0, 0, 0, NULL, 0);
    write_frame(TM_FRAME_DATA_APPEND,   2, 1, 0, 1, 0xDEAD,
                TM_SUB_DATA_PAYLOAD, 0, 0, 0, data, sizeof(data));

    tm_session_t *sess = tm_session_create(1);
    tm_session_ingest(sess, buf, pos);
    assert(sess->segments.count == 1);
    tm_segment_t *seg = tm_segment_lookup(&sess->segments, 1);
    assert(seg != NULL);
    assert(seg->data_len == 64);
    tm_session_destroy(sess);
    printf("  segment lifecycle: PASS\n");
}

int main(void) {
    printf("Integration tests:\n");
    test_session_init();
    test_segment_lifecycle();
    printf("All integration tests passed.\n");
    return 0;
}
