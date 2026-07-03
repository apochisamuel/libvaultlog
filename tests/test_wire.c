#include "tm/wire.h"
#include "tm/crc.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

static void test_u16_roundtrip(void)
{
    uint8_t buf[2];
    for (uint16_t v = 0; v < 65535; v += 1000) {
        tm_wire_write_u16(buf, v);
        uint16_t r = tm_wire_read_u16(buf);
        assert(r == v);
    }
    printf("  u16 roundtrip: PASS\n");
}

static void test_u32_roundtrip(void)
{
    uint8_t buf[4];
    uint32_t vals[] = {0, 1, 255, 256, 65535, 65536, 0xDEADBEEF, 0xFFFFFFFF};
    for (size_t i = 0; i < sizeof(vals)/sizeof(vals[0]); i++) {
        tm_wire_write_u32(buf, vals[i]);
        uint32_t r = tm_wire_read_u32(buf);
        assert(r == vals[i]);
    }
    printf("  u32 roundtrip: PASS\n");
}

static void test_crc32c(void)
{
    uint8_t data[] = "Hello TideMark";
    uint32_t c1 = tm_crc32c(0, data, sizeof(data) - 1);
    uint32_t c2 = tm_crc32c(0, data, sizeof(data) - 1);
    assert(c1 == c2);
    assert(c1 != 0);

    data[0] = 'h';
    uint32_t c3 = tm_crc32c(0, data, sizeof(data) - 1);
    assert(c3 != c1);

    printf("  crc32c: PASS\n");
}

static void test_session_header_decode(void)
{
    uint8_t buf[TM_SESSION_HEADER_WIRE_SIZE];
    tm_wire_write_u32(buf, TM_MAGIC);
    tm_wire_write_u16(buf + 4, TM_VERSION);
    tm_wire_write_u32(buf + 6, 42);
    tm_wire_write_u32(buf + 10, 7);
    tm_wire_write_u16(buf + 14, 3);
    uint32_t crc = tm_crc32c(0, buf, 16);
    tm_wire_write_u32(buf + 16, crc);

    tm_session_header_t hdr;
    tm_status_t rc = tm_wire_decode_session_header(buf, sizeof(buf), &hdr);
    assert(rc == TM_OK);
    assert(hdr.session_id == 42);
    assert(hdr.epoch == 7);
    assert(hdr.frame_count == 3);

    printf("  session header decode: PASS\n");
}

static void test_bad_magic(void)
{
    uint8_t buf[TM_SESSION_HEADER_WIRE_SIZE];
    memset(buf, 0, sizeof(buf));
    tm_wire_write_u32(buf, 0xBADF00D);

    tm_session_header_t hdr;
    tm_status_t rc = tm_wire_decode_session_header(buf, sizeof(buf), &hdr);
    assert(rc == TM_ERR_BAD_MAGIC);

    printf("  bad magic: PASS\n");
}

int main(void)
{
    printf("Wire tests:\n");
    test_u16_roundtrip();
    test_u32_roundtrip();
    test_crc32c();
    test_session_header_decode();
    test_bad_magic();
    printf("All wire tests passed.\n");
    return 0;
}
