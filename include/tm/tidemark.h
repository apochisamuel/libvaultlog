#ifndef TM_TIDEMARK_H
#define TM_TIDEMARK_H
#include "tm/types.h"
#include "tm/error.h"

typedef struct {
    uint32_t  watermark_epoch;
    uint32_t  zone_count;
    uint32_t  advance_count;
    uint32_t  compaction_debt;    /* segments needing compaction */
} tm_tidemark_t;

tm_status_t  tm_tidemark_init(tm_tidemark_t *tm);
tm_status_t  tm_tidemark_advance(tm_tidemark_t *tm, uint32_t new_epoch);
bool         tm_tidemark_zone_eligible(tm_tidemark_t *tm, uint32_t epoch);
uint32_t     tm_tidemark_watermark(const tm_tidemark_t *tm);
bool         tm_tidemark_epoch_compactable(const tm_tidemark_t *tm, uint32_t epoch);
uint32_t     tm_tidemark_zone_for_epoch(const tm_tidemark_t *tm, uint32_t epoch);
#endif
