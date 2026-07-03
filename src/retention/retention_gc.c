#include "tm/retention.h"
#include "tm/lease.h"
#include "tm/segment.h"
#include <string.h>

typedef struct {
    uint32_t  gc_runs;
    uint32_t  segments_freed;
    uint32_t  leases_expired;
    uint32_t  bytes_reclaimed;
} tm_gc_stats_t;

static tm_gc_stats_t g_gc_stats;

tm_status_t tm_gc_reset_stats(void)
{
    memset(&g_gc_stats, 0, sizeof(g_gc_stats));
    return TM_OK;
}

tm_status_t tm_gc_get_stats(tm_gc_stats_t *stats)
{
    if (!stats) return TM_ERR_BAD_STATE;
    *stats = g_gc_stats;
    return TM_OK;
}

tm_status_t tm_gc_full_sweep(void *seg_table_ptr, void *lease_table_ptr,
                                void *catalog_ptr, void *slab_ptr,
                                void *retention_ptr, uint32_t current_epoch)
{
    if (!seg_table_ptr || !retention_ptr) return TM_ERR_BAD_STATE;

    tm_retention_ctx_t *ret = (tm_retention_ctx_t *)retention_ptr;
    tm_segment_table_t *segs = (tm_segment_table_t *)seg_table_ptr;
    tm_lease_table_t *leases = (tm_lease_table_t *)lease_table_ptr;

    g_gc_stats.gc_runs++;

    /* Phase 1: Expire old leases */
    if (leases) {
        for (size_t i = 0; i < TM_MAX_LEASES; i++) {
            tm_lease_t *l = leases->leases[i];
            if (!l || !l->active) continue;
            if (l->epoch_expires <= current_epoch) {
                l->expired = true;
                l->active = false;
                g_gc_stats.leases_expired++;
            }
        }
    }

    /* Phase 2: Sweep eligible segments */
    for (size_t i = 0; i < TM_MAX_SEGMENTS; i++) {
        tm_segment_t *seg = segs->segments[i];
        if (!seg || !seg->active) continue;

        if (!tm_retention_eligible(ret, seg->epoch_created, current_epoch))
            continue;

        /* Check lease protection */
        bool protected = false;
        if (leases) {
            for (size_t j = 0; j < TM_MAX_LEASES; j++) {
                tm_lease_t *l = leases->leases[j];
                if (l && l->active && l->segment_id == seg->segment_id) {
                    protected = true;
                    break;
                }
            }
        }

        if (!protected) {
            g_gc_stats.bytes_reclaimed += (uint32_t)seg->data_len;
            tm_segment_destroy(segs, seg, slab_ptr);
            g_gc_stats.segments_freed++;
        }
    }

    return TM_OK;
}

tm_status_t tm_gc_incremental(void *seg_table_ptr, void *slab_ptr,
                                 void *retention_ptr, uint32_t current_epoch,
                                 uint32_t max_segments)
{
    if (!seg_table_ptr || !retention_ptr) return TM_ERR_BAD_STATE;

    tm_retention_ctx_t *ret = (tm_retention_ctx_t *)retention_ptr;
    tm_segment_table_t *segs = (tm_segment_table_t *)seg_table_ptr;
    uint32_t freed = 0;

    for (size_t i = 0; i < TM_MAX_SEGMENTS && freed < max_segments; i++) {
        tm_segment_t *seg = segs->segments[i];
        if (!seg || !seg->active) continue;

        if (tm_retention_eligible(ret, seg->epoch_created, current_epoch)) {
            g_gc_stats.bytes_reclaimed += (uint32_t)seg->data_len;
            tm_segment_destroy(segs, seg, slab_ptr);
            g_gc_stats.segments_freed++;
            freed++;
        }
    }

    g_gc_stats.gc_runs++;
    return TM_OK;
}
