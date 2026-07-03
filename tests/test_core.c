#include "tm/core.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>

static void test_arena(void) {
    tm_arena_t arena;
    tm_arena_init(&arena, 1024);
    void *p1 = tm_arena_alloc(&arena, 32);
    void *p2 = tm_arena_alloc(&arena, 64);
    assert(p1 != NULL);
    assert(p2 != NULL);
    assert(p2 > p1);
    tm_arena_reset(&arena);
    void *p3 = tm_arena_alloc(&arena, 32);
    assert(p3 == p1);
    tm_arena_destroy(&arena);
    printf("  arena: PASS\n");
}

static void test_hash(void) {
    uint32_t h1 = tm_hash32(42);
    uint32_t h2 = tm_hash32(42);
    uint32_t h3 = tm_hash32(43);
    assert(h1 == h2);
    assert(h1 != h3);
    printf("  hash: PASS\n");
}

static void test_mix(void) {
    uint64_t m1 = tm_mix64(1, 2);
    uint64_t m2 = tm_mix64(1, 2);
    uint64_t m3 = tm_mix64(2, 1);
    assert(m1 == m2);
    assert(m1 != m3);
    printf("  mix: PASS\n");
}

int main(void) {
    printf("Core tests:\n");
    test_arena();
    test_hash();
    test_mix();
    printf("All core tests passed.\n");
    return 0;
}
