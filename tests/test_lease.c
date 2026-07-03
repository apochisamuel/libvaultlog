#include "tm/lease.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>

static void test_acquire_release(void) {
    tm_lease_table_t lt;
    tm_lease_table_init(&lt);
    tm_lease_t *l = tm_lease_acquire(&lt, 1, 0, 5);
    assert(l != NULL);
    assert(l->lease_id == 1);
    assert(lt.count == 1);
    tm_lease_release(&lt, l);
    assert(lt.count == 0);
    tm_lease_table_destroy(&lt);
    printf("  acquire/release: PASS\n");
}

static void test_renew(void) {
    tm_lease_table_t lt;
    tm_lease_table_init(&lt);
    tm_lease_t *l = tm_lease_acquire(&lt, 1, 0, 5);
    assert(l->epoch_expires == 5);
    tm_lease_renew(l, 3);
    assert(l->epoch_expires == 8);
    assert(l->renewal_count == 1);
    tm_lease_table_destroy(&lt);
    printf("  renew: PASS\n");
}

static void test_chain(void) {
    tm_lease_table_t lt;
    tm_lease_table_init(&lt);
    tm_lease_t *a = tm_lease_acquire(&lt, 1, 0, 5);
    tm_lease_t *b = tm_lease_acquire(&lt, 2, 0, 5);
    tm_lease_chain_link(a, b);
    assert(a->chain_next == b);
    assert(b->chain_prev == a);
    uint8_t buf[64];
    size_t len;
    tm_lease_chain_walk(a, buf, sizeof(buf), &len);
    assert(len == 16);
    tm_lease_table_destroy(&lt);
    printf("  chain: PASS\n");
}

int main(void) {
    printf("Lease tests:\n");
    test_acquire_release();
    test_renew();
    test_chain();
    printf("All lease tests passed.\n");
    return 0;
}
