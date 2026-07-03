#include "tm/clock.h"
#include <string.h>

tm_status_t tm_clock_init(tm_clock_t *clk)
{
    if (!clk) return TM_ERR_BAD_STATE;
    memset(clk, 0, sizeof(*clk));
    clk->current_epoch = 0;
    clk->logical_time = 1;
    return TM_OK;
}

tm_status_t tm_clock_advance_epoch(tm_clock_t *clk)
{
    if (!clk) return TM_ERR_BAD_STATE;
    clk->current_epoch++;
    clk->advances++;
    clk->logical_time += 100;
    return TM_OK;
}

uint32_t tm_clock_epoch(const tm_clock_t *clk) { return clk ? clk->current_epoch : 0; }
uint64_t tm_clock_time(const tm_clock_t *clk) { return clk ? clk->logical_time : 0; }

tm_status_t tm_clock_tick(tm_clock_t *clk)
{
    if (!clk) return TM_ERR_BAD_STATE;
    clk->logical_time++;
    return TM_OK;
}
