#include "tm/core.h"
#include <string.h>

typedef struct {
    uint32_t  seconds;
    uint32_t  nanoseconds;
} tm_timestamp_t;

tm_timestamp_t tm_timestamp_from_u64(uint64_t value)
{
    tm_timestamp_t ts;
    ts.seconds = (uint32_t)(value >> 32);
    ts.nanoseconds = (uint32_t)(value & 0xFFFFFFFF);
    return ts;
}

uint64_t tm_timestamp_to_u64(tm_timestamp_t ts)
{
    return ((uint64_t)ts.seconds << 32) | ts.nanoseconds;
}

int tm_timestamp_compare(tm_timestamp_t a, tm_timestamp_t b)
{
    if (a.seconds != b.seconds)
        return (a.seconds < b.seconds) ? -1 : 1;
    if (a.nanoseconds != b.nanoseconds)
        return (a.nanoseconds < b.nanoseconds) ? -1 : 1;
    return 0;
}

tm_timestamp_t tm_timestamp_add(tm_timestamp_t ts, uint32_t seconds)
{
    ts.seconds += seconds;
    return ts;
}

uint32_t tm_timestamp_diff_seconds(tm_timestamp_t a, tm_timestamp_t b)
{
    if (a.seconds >= b.seconds)
        return a.seconds - b.seconds;
    return b.seconds - a.seconds;
}

bool tm_timestamp_in_range(tm_timestamp_t ts, tm_timestamp_t start,
                              tm_timestamp_t end)
{
    return tm_timestamp_compare(ts, start) >= 0 &&
           tm_timestamp_compare(ts, end) <= 0;
}

typedef struct {
    tm_timestamp_t start;
    tm_timestamp_t end;
    uint32_t       duration_seconds;
} tm_time_range_t;

tm_time_range_t tm_time_range_create(uint64_t start, uint64_t end)
{
    tm_time_range_t range;
    range.start = tm_timestamp_from_u64(start);
    range.end = tm_timestamp_from_u64(end);
    range.duration_seconds = tm_timestamp_diff_seconds(range.end, range.start);
    return range;
}

bool tm_time_ranges_overlap(tm_time_range_t a, tm_time_range_t b)
{
    return tm_timestamp_compare(a.start, b.end) <= 0 &&
           tm_timestamp_compare(b.start, a.end) <= 0;
}
