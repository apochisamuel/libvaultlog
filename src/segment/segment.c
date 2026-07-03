#include "tm/segment.h"
#include "tm/slab.h"
#include <stdlib.h>
#include <string.h>

tm_status_t tm_segment_table_init(tm_segment_table_t *table)
{
    if (!table) return TM_ERR_BAD_STATE;
    memset(table, 0, sizeof(*table));
    table->next_id = 1;
    return TM_OK;
}

void tm_segment_table_destroy(tm_segment_table_t *table)
{
    if (!table) return;
    for (size_t i = 0; i < TM_MAX_SEGMENTS; i++) {
        if (table->segments[i]) {
            free(table->segments[i]->data);
            free(table->segments[i]);
            table->segments[i] = NULL;
        }
    }
    table->count = 0;
}

tm_segment_t *tm_segment_create(tm_segment_table_t *table, uint32_t epoch,
                                  void *slab_ptr, uint32_t *out_slab_slot)
{
    if (!table || table->count >= TM_MAX_SEGMENTS) return NULL;

    tm_segment_t *seg = calloc(1, sizeof(*seg));
    if (!seg) return NULL;

    seg->segment_id   = table->next_id++;
    seg->epoch_created = epoch;
    seg->generation   = 1;
    seg->data_cap     = 1024;
    seg->data         = calloc(1, seg->data_cap);
    if (!seg->data) { free(seg); return NULL; }
    seg->data_len     = 0;
    seg->active       = true;

    /* Register in slab if provided */
    if (slab_ptr) {
        tm_slab_t *slab = (tm_slab_t *)slab_ptr;
        uint32_t slot_id;
        void *slot = tm_slab_alloc(slab, TM_SLAB_TYPE_SEGMENT, &slot_id);
        if (slot) {
            /* Store segment pointer in slab slot for tracking */
            memcpy(slot, &seg, sizeof(tm_segment_t *));
            seg->slab_slot_id = slot_id;
            if (out_slab_slot) *out_slab_slot = slot_id;
        }
    }

    /* Find empty slot in table */
    for (size_t i = 0; i < TM_MAX_SEGMENTS; i++) {
        if (!table->segments[i]) {
            table->segments[i] = seg;
            table->count++;
            return seg;
        }
    }

    free(seg->data);
    free(seg);
    return NULL;
}

tm_segment_t *tm_segment_lookup(tm_segment_table_t *table, uint32_t seg_id)
{
    if (!table) return NULL;
    for (size_t i = 0; i < TM_MAX_SEGMENTS; i++) {
        tm_segment_t *s = table->segments[i];
        if (s && s->active && s->segment_id == seg_id)
            return s;
    }
    return NULL;
}

tm_status_t tm_segment_append(tm_segment_t *seg, const uint8_t *data, size_t len)
{
    if (!seg || !seg->active || seg->sealed) return TM_ERR_BAD_STATE;
    if (!data || len == 0) return TM_OK;

    if (seg->data_len + len > seg->data_cap) {
        size_t new_cap = seg->data_cap * 2;
        while (new_cap < seg->data_len + len) new_cap *= 2;
        if (new_cap > TM_MAX_PAYLOAD * 4) return TM_ERR_OVERFLOW;
        uint8_t *new_data = realloc(seg->data, new_cap);
        if (!new_data) return TM_ERR_ALLOC;
        seg->data = new_data;
        seg->data_cap = new_cap;
    }

    memcpy(seg->data + seg->data_len, data, len);
    seg->data_len += len;
    seg->append_count++;
    seg->byte_count += (uint32_t)len;
    return TM_OK;
}

tm_status_t tm_segment_destroy(tm_segment_table_t *table, tm_segment_t *seg,
                                 void *slab_ptr)
{
    if (!table || !seg) return TM_ERR_BAD_STATE;

    /* Free from slab if tracked */
    if (slab_ptr) {
        tm_slab_t *slab = (tm_slab_t *)slab_ptr;
        tm_slab_free(slab, seg->slab_slot_id);
    }

    /* Remove from table */
    for (size_t i = 0; i < TM_MAX_SEGMENTS; i++) {
        if (table->segments[i] == seg) {
            table->segments[i] = NULL;
            table->count--;
            break;
        }
    }

    seg->active = false;
    free(seg->data);
    free(seg);
    return TM_OK;
}

size_t tm_segment_data_size(const tm_segment_t *seg)
{
    return seg ? seg->data_len : 0;
}
