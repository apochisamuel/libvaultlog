#include "tm/segment.h"
#include "tm/slab.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

static void test_create_destroy(void)
{
    tm_segment_table_t table;
    tm_segment_table_init(&table);

    tm_segment_t *s = tm_segment_create(&table, 0, NULL, NULL);
    assert(s != NULL);
    assert(s->active);
    assert(s->segment_id == 1);
    assert(table.count == 1);

    tm_segment_destroy(&table, s, NULL);
    assert(table.count == 0);

    tm_segment_table_destroy(&table);
    printf("  create/destroy: PASS\n");
}

static void test_append(void)
{
    tm_segment_table_t table;
    tm_segment_table_init(&table);

    tm_segment_t *s = tm_segment_create(&table, 0, NULL, NULL);
    uint8_t data[] = {1, 2, 3, 4, 5};
    tm_status_t rc = tm_segment_append(s, data, sizeof(data));
    assert(rc == TM_OK);
    assert(s->data_len == 5);
    assert(s->append_count == 1);
    assert(memcmp(s->data, data, 5) == 0);

    tm_segment_table_destroy(&table);
    printf("  append: PASS\n");
}

static void test_lookup(void)
{
    tm_segment_table_t table;
    tm_segment_table_init(&table);

    tm_segment_t *s1 = tm_segment_create(&table, 0, NULL, NULL);
    tm_segment_t *s2 = tm_segment_create(&table, 0, NULL, NULL);

    tm_segment_t *found = tm_segment_lookup(&table, s2->segment_id);
    assert(found == s2);

    found = tm_segment_lookup(&table, 999);
    assert(found == NULL);

    tm_segment_table_destroy(&table);
    printf("  lookup: PASS\n");
}

static void test_seal(void)
{
    tm_segment_table_t table;
    tm_segment_table_init(&table);

    tm_segment_t *s = tm_segment_create(&table, 0, NULL, NULL);
    s->sealed = true;

    uint8_t data[] = {1, 2, 3};
    tm_status_t rc = tm_segment_append(s, data, sizeof(data));
    assert(rc == TM_ERR_BAD_STATE);

    tm_segment_table_destroy(&table);
    printf("  seal: PASS\n");
}

static void test_slab_integration(void)
{
    tm_slab_t slab;
    tm_slab_init(&slab, 16, 64);

    tm_segment_table_t table;
    tm_segment_table_init(&table);

    uint32_t slot;
    tm_segment_t *s = tm_segment_create(&table, 0, &slab, &slot);
    assert(s != NULL);
    assert(slot < 16);

    tm_segment_destroy(&table, s, &slab);

    tm_slab_destroy(&slab);
    tm_segment_table_destroy(&table);
    printf("  slab integration: PASS\n");
}

int main(void)
{
    printf("Segment tests:\n");
    test_create_destroy();
    test_append();
    test_lookup();
    test_seal();
    test_slab_integration();
    printf("All segment tests passed.\n");
    return 0;
}
