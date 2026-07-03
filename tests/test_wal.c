#include "tm/wal.h"
#include "tm/segment.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>

static void test_checkpoint(void) {
    tm_wal_t wal;
    tm_wal_init(&wal);
    tm_segment_table_t table;
    tm_segment_table_init(&table);
    tm_segment_t *s = tm_segment_create(&table, 0, NULL, NULL);
    uint8_t data[] = {10,20,30};
    tm_segment_append(s, data, sizeof(data));
    tm_wal_checkpoint(&wal, s->segment_id, s, 0, 0, s->generation, 0, 3);
    assert(wal.count == 1);
    assert(wal.entries[0].valid);
    assert(wal.entries[0].cached_segment == s);
    tm_segment_table_destroy(&table);
    printf("  checkpoint: PASS\n");
}

static void test_replay(void) {
    tm_wal_t wal;
    tm_wal_init(&wal);
    tm_segment_table_t table;
    tm_segment_table_init(&table);
    tm_segment_t *s = tm_segment_create(&table, 0, NULL, NULL);
    uint8_t data[] = {10,20,30,40,50};
    tm_segment_append(s, data, sizeof(data));
    tm_wal_checkpoint(&wal, s->segment_id, s, 0, 0, 1, 0, 5);
    uint8_t buf[64];
    size_t len;
    tm_wal_replay(&wal, buf, sizeof(buf), &len);
    assert(len == 5);
    assert(memcmp(buf, data, 5) == 0);
    tm_segment_table_destroy(&table);
    printf("  replay: PASS\n");
}

int main(void) {
    printf("WAL tests:\n");
    test_checkpoint();
    test_replay();
    printf("All WAL tests passed.\n");
    return 0;
}
