#include "tm/segment.h"
#include <string.h>

typedef struct {
    const tm_segment_table_t *table;
    size_t current_index;
    bool   active_only;
    bool   sealed_only;
} tm_segment_iterator_t;

tm_status_t tm_segment_iter_init(tm_segment_iterator_t *iter,
                                    const tm_segment_table_t *table,
                                    bool active_only, bool sealed_only)
{
    if (!iter || !table) return TM_ERR_BAD_STATE;
    iter->table = table;
    iter->current_index = 0;
    iter->active_only = active_only;
    iter->sealed_only = sealed_only;
    return TM_OK;
}

tm_segment_t *tm_segment_iter_next(tm_segment_iterator_t *iter)
{
    if (!iter || !iter->table) return NULL;

    while (iter->current_index < TM_MAX_SEGMENTS) {
        tm_segment_t *s = iter->table->segments[iter->current_index++];
        if (!s) continue;
        if (iter->active_only && !s->active) continue;
        if (iter->sealed_only && !s->sealed) continue;
        return s;
    }
    return NULL;
}

bool tm_segment_iter_has_next(const tm_segment_iterator_t *iter)
{
    if (!iter || !iter->table) return false;
    for (size_t i = iter->current_index; i < TM_MAX_SEGMENTS; i++) {
        tm_segment_t *s = iter->table->segments[i];
        if (!s) continue;
        if (iter->active_only && !s->active) continue;
        if (iter->sealed_only && !s->sealed) continue;
        return true;
    }
    return false;
}

void tm_segment_iter_reset(tm_segment_iterator_t *iter)
{
    if (iter) iter->current_index = 0;
}

uint32_t tm_segment_iter_count(const tm_segment_table_t *table,
                                  bool active_only, bool sealed_only)
{
    if (!table) return 0;
    uint32_t count = 0;
    for (size_t i = 0; i < TM_MAX_SEGMENTS; i++) {
        tm_segment_t *s = table->segments[i];
        if (!s) continue;
        if (active_only && !s->active) continue;
        if (sealed_only && !s->sealed) continue;
        count++;
    }
    return count;
}
