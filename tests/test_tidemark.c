#include "tm/tidemark.h"
#include <stdio.h>
#include <assert.h>

static void test_init(void) {
    tm_tidemark_t tm;
    tm_tidemark_init(&tm);
    assert(tm.watermark_epoch == 0);
    printf("  init: PASS\n");
}

static void test_advance(void) {
    tm_tidemark_t tm;
    tm_tidemark_init(&tm);
    tm_tidemark_advance(&tm, 5);
    assert(tm.watermark_epoch == 5);
    assert(tm.advance_count == 1);
    printf("  advance: PASS\n");
}

static void test_zone(void) {
    tm_tidemark_t tm;
    tm_tidemark_init(&tm);
    tm_tidemark_advance(&tm, 10);
    assert(tm_tidemark_zone_eligible(&tm, 3));
    assert(!tm_tidemark_zone_eligible(&tm, 15));
    printf("  zone: PASS\n");
}

int main(void) {
    printf("Tidemark tests:\n");
    test_init();
    test_advance();
    test_zone();
    printf("All tidemark tests passed.\n");
    return 0;
}
