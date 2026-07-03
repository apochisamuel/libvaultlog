#include "tm/cursor.h"
#include "tm/segment.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>

static void test_create(void) {
    tm_cursor_t *c = tm_cursor_create(1, 0);
    assert(c != NULL);
    assert(c->cursor_id == 1);
    assert(c->active);
    assert(!c->bound);
    tm_cursor_destroy(c);
    printf("  create: PASS\n");
}

static void test_bind_read(void) {
    tm_segment_table_t table;
    tm_segment_table_init(&table);
    tm_segment_t *s = tm_segment_create(&table, 0, NULL, NULL);
    uint8_t data[] = {10,20,30,40,50};
    tm_segment_append(s, data, sizeof(data));

    tm_cursor_t *c = tm_cursor_create(1, 0);
    tm_cursor_bind_segment(c, s, 1);
    assert(c->bound);

    uint8_t buf[64];
    size_t len;
    tm_cursor_read_at(c, buf, sizeof(buf), &len);
    assert(len == 5);
    assert(buf[0] == 10);

    tm_cursor_destroy(c);
    tm_segment_table_destroy(&table);
    printf("  bind/read: PASS\n");
}

static void test_seek(void) {
    tm_segment_table_t table;
    tm_segment_table_init(&table);
    tm_segment_t *s = tm_segment_create(&table, 0, NULL, NULL);
    uint8_t data[] = {1,2,3,4,5,6,7,8};
    tm_segment_append(s, data, sizeof(data));

    tm_cursor_t *c = tm_cursor_create(1, 0);
    tm_cursor_bind_segment(c, s, 1);
    tm_cursor_seek(c, 4);

    uint8_t buf[64];
    size_t len;
    tm_cursor_read_at(c, buf, sizeof(buf), &len);
    assert(len == 4);
    assert(buf[0] == 5);

    tm_cursor_destroy(c);
    tm_segment_table_destroy(&table);
    printf("  seek: PASS\n");
}

int main(void) {
    printf("Cursor tests:\n");
    test_create();
    test_bind_read();
    test_seek();
    printf("All cursor tests passed.\n");
    return 0;
}
