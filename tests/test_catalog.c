#include "tm/catalog.h"
#include <stdio.h>
#include <assert.h>

static void test_register_lookup(void) {
    tm_catalog_t cat;
    tm_catalog_init(&cat);
    int dummy = 42;
    tm_catalog_register(&cat, 1, &dummy, 0);
    tm_catalog_entry_t *e = tm_catalog_lookup(&cat, 1);
    assert(e != NULL);
    assert(e->segment_id == 1);
    assert(e->segment_ref == &dummy);
    tm_catalog_destroy(&cat);
    printf("  register/lookup: PASS\n");
}

static void test_access_count(void) {
    tm_catalog_t cat;
    tm_catalog_init(&cat);
    int dummy = 42;
    tm_catalog_register(&cat, 1, &dummy, 0);
    tm_catalog_access(&cat, 1);
    tm_catalog_access(&cat, 1);
    tm_catalog_access(&cat, 1);
    tm_catalog_entry_t *e = tm_catalog_lookup(&cat, 1);
    assert(e->access_count == 3);
    tm_catalog_destroy(&cat);
    printf("  access count: PASS\n");
}

static void test_evict_lru(void) {
    tm_catalog_t cat;
    tm_catalog_init(&cat);
    int d1 = 1, d2 = 2, d3 = 3;
    tm_catalog_register(&cat, 1, &d1, 0);
    tm_catalog_register(&cat, 2, &d2, 0);
    tm_catalog_register(&cat, 3, &d3, 0);
    tm_catalog_access(&cat, 2);
    tm_catalog_access(&cat, 2);
    tm_catalog_access(&cat, 3);
    /* Segment 1 has 0 accesses, should be evicted */
    tm_catalog_evict_lru(&cat);
    assert(tm_catalog_lookup(&cat, 1) == NULL);
    assert(tm_catalog_lookup(&cat, 2) != NULL);
    tm_catalog_destroy(&cat);
    printf("  evict LRU: PASS\n");
}

static void test_pin(void) {
    tm_catalog_t cat;
    tm_catalog_init(&cat);
    int d1 = 1, d2 = 2;
    tm_catalog_register(&cat, 1, &d1, 0);
    tm_catalog_register(&cat, 2, &d2, 0);
    tm_catalog_pin(&cat, 1);
    /* Pinned entry should not be evicted */
    tm_catalog_evict_lru(&cat);
    assert(tm_catalog_lookup(&cat, 1) != NULL);
    assert(tm_catalog_lookup(&cat, 2) == NULL);
    tm_catalog_destroy(&cat);
    printf("  pin: PASS\n");
}

int main(void) {
    printf("Catalog tests:\n");
    test_register_lookup();
    test_access_count();
    test_evict_lru();
    test_pin();
    printf("All catalog tests passed.\n");
    return 0;
}
