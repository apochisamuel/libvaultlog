#ifndef TM_CLOCK_H
#define TM_CLOCK_H
#include "tm/types.h"
#include "tm/error.h"

typedef struct {
    uint32_t  current_epoch;
    uint32_t  epoch_start_sequence;
    uint32_t  advances;
    uint64_t  logical_time;
} tm_clock_t;

tm_status_t  tm_clock_init(tm_clock_t *clk);
tm_status_t  tm_clock_advance_epoch(tm_clock_t *clk);
uint32_t     tm_clock_epoch(const tm_clock_t *clk);
uint64_t     tm_clock_time(const tm_clock_t *clk);
tm_status_t  tm_clock_tick(tm_clock_t *clk);
#endif
