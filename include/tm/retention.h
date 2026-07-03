#ifndef TM_RETENTION_H
#define TM_RETENTION_H
#include "tm/types.h"
#include "tm/error.h"

typedef struct {
    uint32_t  policy_id;
    uint32_t  max_epoch_age;     /* segments older than this many epochs can be GC'd */
    bool      active;
} tm_retention_policy_t;

typedef struct {
    tm_retention_policy_t policies[4];
    size_t                policy_count;
    uint32_t              sweep_count;
} tm_retention_ctx_t;

tm_status_t  tm_retention_init(tm_retention_ctx_t *ctx);
tm_status_t  tm_retention_set_policy(tm_retention_ctx_t *ctx,
                uint32_t policy_id, uint32_t max_epoch_age);
bool         tm_retention_eligible(tm_retention_ctx_t *ctx,
                uint32_t segment_epoch, uint32_t current_epoch);
tm_status_t  tm_retention_sweep(tm_retention_ctx_t *ctx,
                void *segment_table, void *catalog, void *slab,
                uint32_t current_epoch);
#endif
