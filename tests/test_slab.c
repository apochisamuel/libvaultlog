#include "tm/slab.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

static void test_alloc_free(void)
{
    tm_slab_t slab;
    tm_slab_init(&slab, 4, 32);

    uint32_t s1, s2;
    void *p1 = tm_slab_alloc(&slab, TM_SLAB_TYPE_SEGMENT, &s1);
    void *p2 = tm_slab_alloc(&slab, TM_SLAB_TYPE_INDEX, &s2);
    assert(p1 != NULL);
    assert(p2 != NULL);
    assert(s1 != s2);

    tm_slab_free(&slab, s1);
    /* LIFO: next alloc should reuse s1's slot */
    uint32_t s3;
    void *p3 = tm_slab_alloc(&slab, TM_SLAB_TYPE_LEASE, &s3);
    assert(p3 == p1);  /* same slot reused */
    assert(s3 == s1);

    tm_slab_destroy(&slab);
    printf("  alloc/free LIFO: PASS\n");
}

static void test_generation(void)
{
    tm_slab_t slab;
    tm_slab_init(&slab, 2, 16);

    uint32_t slot;
    tm_slab_alloc(&slab, TM_SLAB_TYPE_SEGMENT, &slot);
    uint32_t gen1 = tm_slab_slot_generation(&slab, slot);

    tm_slab_free(&slab, slot);
    tm_slab_alloc(&slab, TM_SLAB_TYPE_INDEX, &slot);
    uint32_t gen2 = tm_slab_slot_generation(&slab, slot);

    assert(gen2 > gen1);

    tm_slab_destroy(&slab);
    printf("  generation tracking: PASS\n");
}

static void test_type_tag(void)
{
    tm_slab_t slab;
    tm_slab_init(&slab, 2, 16);

    uint32_t slot;
    tm_slab_alloc(&slab, TM_SLAB_TYPE_SEGMENT, &slot);
    assert(tm_slab_slot_type(&slab, slot) == TM_SLAB_TYPE_SEGMENT);

    tm_slab_free(&slab, slot);
    tm_slab_alloc(&slab, TM_SLAB_TYPE_INDEX, &slot);
    assert(tm_slab_slot_type(&slab, slot) == TM_SLAB_TYPE_INDEX);

    tm_slab_destroy(&slab);
    printf("  type tag: PASS\n");
}

int main(void)
{
    printf("Slab tests:\n");
    test_alloc_free();
    test_generation();
    test_type_tag();
    printf("All slab tests passed.\n");
    return 0;
}
