#include "tm/clock.h"
#include <string.h>
#include <stdlib.h>

#define TM_CLOCK_HISTORY_SIZE 64

typedef struct {
    uint32_t  epoch;
    uint64_t  logical_time;
} tm_clock_event_t;

typedef struct {
    tm_clock_event_t events[TM_CLOCK_HISTORY_SIZE];
    size_t           count;
    size_t           head;
} tm_clock_history_t;

static tm_clock_history_t g_clock_history;

tm_status_t tm_clock_record_event(uint32_t epoch, uint64_t logical_time)
{
    size_t idx = g_clock_history.head % TM_CLOCK_HISTORY_SIZE;
    g_clock_history.events[idx].epoch = epoch;
    g_clock_history.events[idx].logical_time = logical_time;
    g_clock_history.head++;
    if (g_clock_history.count < TM_CLOCK_HISTORY_SIZE)
        g_clock_history.count++;
    return TM_OK;
}

tm_status_t tm_clock_get_history(tm_clock_event_t *events, size_t *count,
                                    size_t max_events)
{
    if (!events || !count) return TM_ERR_BAD_STATE;
    *count = 0;
    size_t total = g_clock_history.count;
    if (total > max_events) total = max_events;
    
    size_t start = (g_clock_history.head >= total) ?
                    g_clock_history.head - total : 0;
    for (size_t i = 0; i < total; i++) {
        size_t idx = (start + i) % TM_CLOCK_HISTORY_SIZE;
        events[i] = g_clock_history.events[idx];
        (*count)++;
    }

    return TM_OK;
}

void tm_clock_clear_history(void)
{
    memset(&g_clock_history, 0, sizeof(g_clock_history));
}

uint32_t tm_clock_epoch_at_time(uint64_t logical_time)
{
    /* Find the epoch that was active at the given logical time */
    uint32_t best_epoch = 0;
    for (size_t i = 0; i < g_clock_history.count; i++) {
        size_t idx = i % TM_CLOCK_HISTORY_SIZE;
        if (g_clock_history.events[idx].logical_time <= logical_time) {
            if (g_clock_history.events[idx].epoch > best_epoch)
                best_epoch = g_clock_history.events[idx].epoch;
        }
    }
    return best_epoch;
}

bool tm_clock_epoch_was_active(uint32_t epoch, uint64_t start, uint64_t end)
{
    for (size_t i = 0; i < g_clock_history.count; i++) {
        size_t idx = i % TM_CLOCK_HISTORY_SIZE;
        if (g_clock_history.events[idx].epoch == epoch &&
            g_clock_history.events[idx].logical_time >= start &&
            g_clock_history.events[idx].logical_time <= end)
            return true;
    }
    return false;
}
