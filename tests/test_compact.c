#include "tm/compact.h"
#include "tm/segment.h"
#include "tm/slab.h"
#include "tm/index.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>

static void test_auto_trigger(void) {
    tm_slab_t slab;
    tm_slab_init(&slab, 32, 64);
    tm_segment_table_t table;
    tm_segment_table_init(&table);
    tm_index_t idx;
    tm_index_create(&idx, 16);

    tm_segment_t *a = tm_segment_create(&table, 0, &slab, NULL);
    tm_segment_t *b = tm_segment_create(&table, 0, &slab, NULL);

    uint8_t data[300];
    memset(data, 0xBB, sizeof(data));
    tm_segment_append(a, data, sizeof(data));
    tm_segment_append(b, data, sizeof(data));
    a->sealed = true;
    b->sealed = true;

    tm_compact_ctx_t ctx;
    tm_compact_init(&ctx);
    tm_compact_trigger(&ctx, &table, &slab, &idx, 1);
    assert(ctx.merges_completed == 1);

    tm_index_destroy(&idx);
    tm_slab_destroy(&slab);
    tm_segment_table_destroy(&table);
    printf("  auto trigger: PASS\n");
}

int main(void) {
    printf("Compact tests:\n");
    test_auto_trigger();
    printf("All compact tests passed.\n");
    return 0;
}
