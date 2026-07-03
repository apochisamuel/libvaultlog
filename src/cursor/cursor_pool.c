#include "tm/cursor.h"
#include <string.h>
#include <stdlib.h>

typedef struct {
    tm_cursor_t *cursors[TM_MAX_CURSORS];
    uint32_t     active_count;
    uint32_t     total_created;
    uint32_t     total_destroyed;
    uint32_t     peak_active;
} tm_cursor_pool_t;

tm_status_t tm_cursor_pool_init(tm_cursor_pool_t *pool)
{
    if (!pool) return TM_ERR_BAD_STATE;
    memset(pool, 0, sizeof(*pool));
    return TM_OK;
}

void tm_cursor_pool_destroy(tm_cursor_pool_t *pool)
{
    if (!pool) return;
    for (size_t i = 0; i < TM_MAX_CURSORS; i++) {
        if (pool->cursors[i]) {
            tm_cursor_destroy(pool->cursors[i]);
            pool->cursors[i] = NULL;
        }
    }
}

tm_cursor_t *tm_cursor_pool_acquire(tm_cursor_pool_t *pool,
                                       uint32_t cursor_id,
                                       uint32_t epoch)
{
    if (!pool || pool->active_count >= TM_MAX_CURSORS) return NULL;

    tm_cursor_t *c = tm_cursor_create(cursor_id, epoch);
    if (!c) return NULL;

    for (size_t i = 0; i < TM_MAX_CURSORS; i++) {
        if (!pool->cursors[i]) {
            pool->cursors[i] = c;
            pool->active_count++;
            pool->total_created++;
            if (pool->active_count > pool->peak_active)
                pool->peak_active = pool->active_count;
            return c;
        }
    }

    tm_cursor_destroy(c);
    return NULL;
}

tm_status_t tm_cursor_pool_release(tm_cursor_pool_t *pool,
                                      uint32_t cursor_id)
{
    if (!pool) return TM_ERR_BAD_STATE;

    for (size_t i = 0; i < TM_MAX_CURSORS; i++) {
        if (pool->cursors[i] && pool->cursors[i]->cursor_id == cursor_id) {
            tm_cursor_destroy(pool->cursors[i]);
            pool->cursors[i] = NULL;
            pool->active_count--;
            pool->total_destroyed++;
            return TM_OK;
        }
    }

    return TM_ERR_NOT_FOUND;
}

tm_cursor_t *tm_cursor_pool_find(const tm_cursor_pool_t *pool,
                                    uint32_t cursor_id)
{
    if (!pool) return NULL;
    for (size_t i = 0; i < TM_MAX_CURSORS; i++) {
        if (pool->cursors[i] && pool->cursors[i]->cursor_id == cursor_id)
            return pool->cursors[i];
    }
    return NULL;
}

tm_status_t tm_cursor_pool_invalidate_segment(tm_cursor_pool_t *pool,
                                                 uint32_t segment_id)
{
    if (!pool) return TM_ERR_BAD_STATE;
    for (size_t i = 0; i < TM_MAX_CURSORS; i++) {
        tm_cursor_t *c = pool->cursors[i];
        if (c && c->bound && c->bound_segment_id == segment_id) {
            c->bound = false;
            c->bound_segment = NULL;
        }
    }
    return TM_OK;
}

typedef struct {
    uint32_t  active;
    uint32_t  bound;
    uint32_t  total_created;
    uint32_t  total_destroyed;
    uint32_t  peak;
} tm_cursor_pool_stats_t;

tm_status_t tm_cursor_pool_stats(const tm_cursor_pool_t *pool,
                                    tm_cursor_pool_stats_t *stats)
{
    if (!pool || !stats) return TM_ERR_BAD_STATE;
    memset(stats, 0, sizeof(*stats));
    stats->active = pool->active_count;
    stats->total_created = pool->total_created;
    stats->total_destroyed = pool->total_destroyed;
    stats->peak = pool->peak_active;

    for (size_t i = 0; i < TM_MAX_CURSORS; i++) {
        if (pool->cursors[i] && pool->cursors[i]->bound)
            stats->bound++;
    }

    return TM_OK;
}
