#include "tm/index.h"
#include <stdio.h>
#include <assert.h>

static void test_insert_lookup(void) {
    tm_index_t idx;
    tm_index_create(&idx, 16);
    tm_index_insert(&idx, 1000, 1, 0, NULL, NULL);
    tm_index_insert(&idx, 2000, 2, 100, NULL, NULL);
    tm_index_entry_t *e = tm_index_lookup(&idx, 1000);
    assert(e != NULL);
    assert(e->segment_id == 1);
    e = tm_index_lookup(&idx, 3000);
    assert(e == NULL);
    tm_index_destroy(&idx);
    printf("  insert/lookup: PASS\n");
}

static void test_remove(void) {
    tm_index_t idx;
    tm_index_create(&idx, 16);
    tm_index_insert(&idx, 1000, 1, 0, NULL, NULL);
    tm_index_insert(&idx, 2000, 1, 100, NULL, NULL);
    tm_index_insert(&idx, 3000, 2, 0, NULL, NULL);
    assert(idx.count == 3);
    tm_index_remove_segment(&idx, 1);
    assert(idx.count == 1);
    tm_index_destroy(&idx);
    printf("  remove segment: PASS\n");
}

static void test_rebuild(void) {
    tm_index_t idx;
    tm_index_create(&idx, 16);
    tm_index_insert(&idx, 1000, 1, 0, NULL, NULL);
    tm_index_insert(&idx, 2000, 2, 0, NULL, NULL);
    assert(idx.capacity == 16);
    tm_index_rebuild(&idx, 8);
    assert(idx.capacity == 8);
    assert(idx.count == 2);
    tm_index_destroy(&idx);
    printf("  rebuild: PASS\n");
}

int main(void) {
    printf("Index tests:\n");
    test_insert_lookup();
    test_remove();
    test_rebuild();
    printf("All index tests passed.\n");
    return 0;
}
