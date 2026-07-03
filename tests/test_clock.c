#include "tm/clock.h"
#include <stdio.h>
#include <assert.h>

static void test_init(void) {
    tm_clock_t clk;
    tm_clock_init(&clk);
    assert(clk.current_epoch == 0);
    assert(clk.logical_time == 1);
    printf("  init: PASS\n");
}

static void test_advance(void) {
    tm_clock_t clk;
    tm_clock_init(&clk);
    tm_clock_advance_epoch(&clk);
    assert(clk.current_epoch == 1);
    tm_clock_advance_epoch(&clk);
    assert(clk.current_epoch == 2);
    assert(clk.advances == 2);
    printf("  advance: PASS\n");
}

static void test_tick(void) {
    tm_clock_t clk;
    tm_clock_init(&clk);
    uint64_t t0 = clk.logical_time;
    tm_clock_tick(&clk);
    assert(clk.logical_time == t0 + 1);
    printf("  tick: PASS\n");
}

int main(void) {
    printf("Clock tests:\n");
    test_init();
    test_advance();
    test_tick();
    printf("All clock tests passed.\n");
    return 0;
}
