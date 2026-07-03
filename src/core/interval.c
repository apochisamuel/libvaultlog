#include "tm/core.h"
#include <string.h>
#include <stdlib.h>

typedef struct {
    uint64_t  start;
    uint64_t  end;
} tm_interval_t;

bool tm_interval_overlaps(tm_interval_t a, tm_interval_t b)
{
    return a.start <= b.end && b.start <= a.end;
}

bool tm_interval_contains(tm_interval_t outer, tm_interval_t inner)
{
    return outer.start <= inner.start && inner.end <= outer.end;
}

bool tm_interval_contains_point(tm_interval_t iv, uint64_t point)
{
    return point >= iv.start && point <= iv.end;
}

tm_interval_t tm_interval_union(tm_interval_t a, tm_interval_t b)
{
    tm_interval_t r;
    r.start = (a.start < b.start) ? a.start : b.start;
    r.end = (a.end > b.end) ? a.end : b.end;
    return r;
}

tm_interval_t tm_interval_intersection(tm_interval_t a, tm_interval_t b)
{
    tm_interval_t r;
    r.start = (a.start > b.start) ? a.start : b.start;
    r.end = (a.end < b.end) ? a.end : b.end;
    if (r.start > r.end) { r.start = 0; r.end = 0; }
    return r;
}

uint64_t tm_interval_length(tm_interval_t iv)
{
    return (iv.end >= iv.start) ? iv.end - iv.start : 0;
}

typedef struct {
    tm_interval_t *intervals;
    size_t         count;
    size_t         capacity;
} tm_interval_set_t;

tm_status_t tm_interval_set_init(tm_interval_set_t *set, size_t capacity)
{
    if (!set || capacity == 0) return TM_ERR_BAD_STATE;
    set->intervals = calloc(capacity, sizeof(tm_interval_t));
    if (!set->intervals) return TM_ERR_ALLOC;
    set->capacity = capacity;
    set->count = 0;
    return TM_OK;
}

void tm_interval_set_destroy(tm_interval_set_t *set)
{
    if (set) { free(set->intervals); memset(set, 0, sizeof(*set)); }
}

tm_status_t tm_interval_set_add(tm_interval_set_t *set, uint64_t start, uint64_t end)
{
    if (!set || set->count >= set->capacity) return TM_ERR_OVERFLOW;
    set->intervals[set->count].start = start;
    set->intervals[set->count].end = end;
    set->count++;
    return TM_OK;
}

bool tm_interval_set_contains(const tm_interval_set_t *set, uint64_t point)
{
    if (!set) return false;
    for (size_t i = 0; i < set->count; i++) {
        if (tm_interval_contains_point(set->intervals[i], point)) return true;
    }
    return false;
}

size_t tm_interval_set_count_overlapping(const tm_interval_set_t *set,
                                            uint64_t start, uint64_t end)
{
    if (!set) return 0;
    tm_interval_t query = {start, end};
    size_t count = 0;
    for (size_t i = 0; i < set->count; i++) {
        if (tm_interval_overlaps(set->intervals[i], query)) count++;
    }
    return count;
}

uint64_t tm_interval_set_total_coverage(const tm_interval_set_t *set)
{
    if (!set) return 0;
    uint64_t total = 0;
    for (size_t i = 0; i < set->count; i++)
        total += tm_interval_length(set->intervals[i]);
    return total;
}
