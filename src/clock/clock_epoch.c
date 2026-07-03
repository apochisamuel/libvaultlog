#include "tm/clock.h"
#include <string.h>

/*
 * Epoch management: epoch comparison, duration calculation,
 * and epoch window operations.
 */

bool tm_clock_epoch_in_window(const tm_clock_t *clk,
                                uint32_t epoch, uint32_t window_size)
{
    if (!clk) return false;
    if (epoch > clk->current_epoch) return false;
    return (clk->current_epoch - epoch) <= window_size;
}

uint32_t tm_clock_epoch_distance(const tm_clock_t *clk, uint32_t epoch)
{
    if (!clk || epoch > clk->current_epoch) return 0;
    return clk->current_epoch - epoch;
}

bool tm_clock_epoch_expired(const tm_clock_t *clk,
                              uint32_t epoch, uint32_t max_age)
{
    if (!clk) return false;
    if (epoch > clk->current_epoch) return false;
    return (clk->current_epoch - epoch) >= max_age;
}

typedef struct {
    uint32_t  current_epoch;
    uint32_t  total_advances;
    uint64_t  logical_time;
    uint32_t  ticks_since_advance;
} tm_clock_stats_t;

tm_status_t tm_clock_compute_stats(const tm_clock_t *clk,
                                      tm_clock_stats_t *stats)
{
    if (!clk || !stats) return TM_ERR_BAD_STATE;
    memset(stats, 0, sizeof(*stats));
    stats->current_epoch = clk->current_epoch;
    stats->total_advances = clk->advances;
    stats->logical_time = clk->logical_time;
    return TM_OK;
}
