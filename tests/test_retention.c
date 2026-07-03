#include "tm/retention.h"
#include "tm/segment.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>

static void test_policy(void) {
    tm_retention_ctx_t ctx;
    tm_retention_init(&ctx);
    tm_retention_set_policy(&ctx, 1, 5);
    assert(!tm_retention_eligible(&ctx, 0, 3));
    assert(tm_retention_eligible(&ctx, 0, 5));
    assert(tm_retention_eligible(&ctx, 0, 10));
    printf("  policy: PASS\n");
}

static void test_multi_policy(void) {
    tm_retention_ctx_t ctx;
    tm_retention_init(&ctx);
    tm_retention_set_policy(&ctx, 1, 3);
    tm_retention_set_policy(&ctx, 2, 5);
    /* Both must agree */
    assert(!tm_retention_eligible(&ctx, 0, 3));
    assert(!tm_retention_eligible(&ctx, 0, 4));
    assert(tm_retention_eligible(&ctx, 0, 5));
    printf("  multi-policy: PASS\n");
}

static void test_sweep(void) {
    tm_retention_ctx_t ctx;
    tm_retention_init(&ctx);
    tm_retention_set_policy(&ctx, 1, 3);
    tm_segment_table_t table;
    tm_segment_table_init(&table);
    tm_segment_t *s = tm_segment_create(&table, 0, NULL, NULL);
    assert(s != NULL);
    assert(table.count == 1);
    tm_retention_sweep(&ctx, &table, NULL, NULL, 5);
    assert(table.count == 0);
    tm_segment_table_destroy(&table);
    printf("  sweep: PASS\n");
}

int main(void) {
    printf("Retention tests:\n");
    test_policy();
    test_multi_policy();
    test_sweep();
    printf("All retention tests passed.\n");
    return 0;
}
