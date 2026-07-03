#include "tm/session.h"
#include "tm/wire.h"
#include <string.h>
#include <stdlib.h>

/*
 * Administrative session handlers: lease management, retention
 * policy, catalog management, compaction scheduling.
 * These are invoked through tm_admin_dispatch.
 */

typedef struct {
    uint32_t  admin_ops_processed;
    uint32_t  leases_acquired;
    uint32_t  leases_released;
    uint32_t  leases_renewed;
    uint32_t  retention_sweeps;
    uint32_t  compact_triggers;
    uint32_t  catalog_accesses;
    uint32_t  catalog_evictions;
} tm_admin_stats_t;

static tm_admin_stats_t g_admin_stats;

tm_status_t tm_admin_reset_stats(void)
{
    memset(&g_admin_stats, 0, sizeof(g_admin_stats));
    return TM_OK;
}

tm_status_t tm_admin_get_stats(tm_admin_stats_t *stats)
{
    if (!stats) return TM_ERR_BAD_STATE;
    memcpy(stats, &g_admin_stats, sizeof(*stats));
    return TM_OK;
}

tm_status_t tm_admin_process_lease_batch(tm_session_t *sess,
                                            const uint32_t *segment_ids,
                                            uint32_t count,
                                            uint32_t duration)
{
    if (!sess || !segment_ids) return TM_ERR_BAD_STATE;

    for (uint32_t i = 0; i < count; i++) {
        tm_segment_t *seg = tm_segment_lookup(&sess->segments, segment_ids[i]);
        if (!seg) continue;

        tm_lease_t *l = tm_lease_acquire(&sess->leases, segment_ids[i],
                                           sess->clock.current_epoch, duration);
        if (l) g_admin_stats.leases_acquired++;
    }

    return TM_OK;
}

tm_status_t tm_admin_expire_leases(tm_session_t *sess)
{
    if (!sess) return TM_ERR_BAD_STATE;

    for (size_t i = 0; i < TM_MAX_LEASES; i++) {
        tm_lease_t *l = sess->leases.leases[i];
        if (!l || !l->active) continue;

        if (l->epoch_expires <= sess->clock.current_epoch) {
            l->expired = true;
            l->active = false;
        }
    }

    return TM_OK;
}

tm_status_t tm_admin_run_retention(tm_session_t *sess)
{
    if (!sess) return TM_ERR_BAD_STATE;

    /* First expire leases */
    tm_admin_expire_leases(sess);

    /* Then run retention sweep */
    tm_status_t rc = tm_retention_sweep(&sess->retention,
                                           &sess->segments,
                                           &sess->catalog,
                                           &sess->slab,
                                           sess->clock.current_epoch);
    if (rc == TM_OK) g_admin_stats.retention_sweeps++;
    return rc;
}

tm_status_t tm_admin_compact(tm_session_t *sess)
{
    if (!sess) return TM_ERR_BAD_STATE;

    tm_status_t rc = tm_compact_trigger(&sess->compact,
                                           &sess->segments,
                                           &sess->slab,
                                           &sess->index,
                                           sess->clock.current_epoch);
    if (rc == TM_OK) g_admin_stats.compact_triggers++;
    return rc;
}

tm_status_t tm_admin_catalog_maintenance(tm_session_t *sess)
{
    if (!sess) return TM_ERR_BAD_STATE;

    /* Check catalog pressure and evict if needed */
    if (sess->catalog.count >= sess->catalog.eviction_threshold) {
        tm_catalog_evict_lru(&sess->catalog);
        g_admin_stats.catalog_evictions++;
    }

    return TM_OK;
}

tm_status_t tm_admin_session_health(const tm_session_t *sess,
                                       bool *out_healthy)
{
    if (!sess || !out_healthy) return TM_ERR_BAD_STATE;
    *out_healthy = true;

    /* Check for WAL gaps */
    if (sess->wal.has_gap) *out_healthy = false;

    /* Check for excessive lease expiry */
    uint32_t expired = 0;
    for (size_t i = 0; i < TM_MAX_LEASES; i++) {
        if (sess->leases.leases[i] && sess->leases.leases[i]->expired)
            expired++;
    }
    if (expired > TM_MAX_LEASES / 2) *out_healthy = false;

    /* Check catalog pressure */
    if (sess->catalog.count >= sess->catalog.capacity)
        *out_healthy = false;

    return TM_OK;
}
