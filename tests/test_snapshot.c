#include "tm/snapshot.h"
#include "tm/segment.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>

static void test_capture(void) {
    tm_segment_table_t table;
    tm_segment_table_init(&table);
    tm_segment_create(&table, 0, NULL, NULL);
    tm_segment_create(&table, 0, NULL, NULL);
    tm_snapshot_t *snap = tm_snapshot_capture(1, 0, &table);
    assert(snap != NULL);
    assert(snap->segment_count == 2);
    assert(snap->active);
    tm_snapshot_release(snap);
    tm_segment_table_destroy(&table);
    printf("  capture: PASS\n");
}

static void test_materialize(void) {
    tm_segment_table_t table;
    tm_segment_table_init(&table);
    tm_segment_t *s = tm_segment_create(&table, 0, NULL, NULL);
    uint8_t data[] = {1,2,3,4,5};
    tm_segment_append(s, data, sizeof(data));
    tm_snapshot_t *snap = tm_snapshot_capture(1, 0, &table);
    uint8_t buf[64];
    size_t len;
    tm_status_t rc = tm_snapshot_materialize(snap, s->segment_id, buf, sizeof(buf), &len);
    assert(rc == TM_OK);
    assert(len == 5);
    assert(memcmp(buf, data, 5) == 0);
    tm_snapshot_release(snap);
    tm_segment_table_destroy(&table);
    printf("  materialize: PASS\n");
}

int main(void) {
    printf("Snapshot tests:\n");
    test_capture();
    test_materialize();
    printf("All snapshot tests passed.\n");
    return 0;
}
