#include "tm/session.h"
#include "tm/wire.h"
#include <string.h>

/*
 * Session migration protocol: transfers segments, leases,
 * and WAL state between sessions for load balancing and
 * fault tolerance.
 */

typedef struct {
    uint32_t  source_session_id;
    uint32_t  dest_session_id;
    uint32_t  segment_ids[TM_MAX_SEGMENTS];
    uint32_t  segment_count;
    uint32_t  lease_ids[TM_MAX_LEASES];
    uint32_t  lease_count;
    uint32_t  migration_epoch;
    bool      prepared;
    bool      transferred;
    bool      committed;
} tm_migration_ctx_t;

static tm_migration_ctx_t g_migration;

tm_status_t tm_migration_init(void)
{
    memset(&g_migration, 0, sizeof(g_migration));
    return TM_OK;
}

tm_status_t tm_migration_prepare(tm_session_t *src, tm_session_t *dst,
                                    uint32_t *segment_ids, uint32_t count)
{
    if (!src || !dst || !segment_ids || count == 0)
        return TM_ERR_BAD_STATE;

    g_migration.source_session_id = src->session_id;
    g_migration.dest_session_id = dst->session_id;
    g_migration.migration_epoch = src->clock.current_epoch;

    for (uint32_t i = 0; i < count && i < TM_MAX_SEGMENTS; i++) {
        g_migration.segment_ids[i] = segment_ids[i];
        g_migration.segment_count++;
    }

    /* Find leases associated with migrating segments */
    for (size_t i = 0; i < TM_MAX_LEASES; i++) {
        tm_lease_t *l = src->leases.leases[i];
        if (!l || !l->active) continue;

        for (uint32_t j = 0; j < g_migration.segment_count; j++) {
            if (l->segment_id == g_migration.segment_ids[j]) {
                if (g_migration.lease_count < TM_MAX_LEASES) {
                    g_migration.lease_ids[g_migration.lease_count++] = l->lease_id;
                }
                break;
            }
        }
    }

    g_migration.prepared = true;
    return TM_OK;
}

tm_status_t tm_migration_transfer(tm_session_t *src, tm_session_t *dst)
{
    if (!src || !dst || !g_migration.prepared) return TM_ERR_BAD_STATE;

    /* Transfer segment data to destination */
    for (uint32_t i = 0; i < g_migration.segment_count; i++) {
        tm_segment_t *seg = tm_segment_lookup(&src->segments,
                                                g_migration.segment_ids[i]);
        if (!seg) continue;

        /* Create corresponding segment in destination */
        tm_segment_t *dst_seg = tm_segment_create(&dst->segments,
                                                      dst->clock.current_epoch,
                                                      &dst->slab, NULL);
        if (!dst_seg) continue;

        /* Copy data */
        if (seg->data && seg->data_len > 0) {
            tm_segment_append(dst_seg, seg->data, seg->data_len);
        }
        dst_seg->sealed = seg->sealed;

        /* Register in destination catalog */
        tm_catalog_register(&dst->catalog, dst_seg->segment_id,
                              dst_seg, dst->clock.current_epoch);
    }

    /* Transfer leases */
    for (uint32_t i = 0; i < g_migration.lease_count; i++) {
        tm_lease_t *l = tm_lease_lookup(&src->leases,
                                           g_migration.lease_ids[i]);
        if (!l) continue;

        uint32_t remaining = 0;
        if (l->epoch_expires > dst->clock.current_epoch)
            remaining = l->epoch_expires - dst->clock.current_epoch;

        tm_lease_acquire(&dst->leases, l->segment_id,
                           dst->clock.current_epoch,
                           remaining > 0 ? remaining : 2);
    }

    g_migration.transferred = true;
    return TM_OK;
}

tm_status_t tm_migration_commit(tm_session_t *src)
{
    if (!src || !g_migration.transferred) return TM_ERR_BAD_STATE;

    /* Release source segments */
    for (uint32_t i = 0; i < g_migration.segment_count; i++) {
        tm_segment_t *seg = tm_segment_lookup(&src->segments,
                                                g_migration.segment_ids[i]);
        if (seg) {
            tm_catalog_remove(&src->catalog, seg->segment_id);
            tm_segment_destroy(&src->segments, seg, &src->slab);
        }
    }

    /* Release source leases */
    for (uint32_t i = 0; i < g_migration.lease_count; i++) {
        tm_lease_t *l = tm_lease_lookup(&src->leases,
                                           g_migration.lease_ids[i]);
        if (l) tm_lease_release(&src->leases, l);
    }

    g_migration.committed = true;
    return TM_OK;
}

tm_status_t tm_migration_rollback(void)
{
    memset(&g_migration, 0, sizeof(g_migration));
    return TM_OK;
}

typedef struct {
    bool      in_progress;
    bool      prepared;
    bool      transferred;
    bool      committed;
    uint32_t  segments_migrated;
    uint32_t  leases_migrated;
    uint32_t  source_session;
    uint32_t  dest_session;
} tm_migration_stats_t;

tm_status_t tm_migration_get_stats(tm_migration_stats_t *stats)
{
    if (!stats) return TM_ERR_BAD_STATE;
    memset(stats, 0, sizeof(*stats));
    stats->in_progress = g_migration.prepared && !g_migration.committed;
    stats->prepared = g_migration.prepared;
    stats->transferred = g_migration.transferred;
    stats->committed = g_migration.committed;
    stats->segments_migrated = g_migration.segment_count;
    stats->leases_migrated = g_migration.lease_count;
    stats->source_session = g_migration.source_session_id;
    stats->dest_session = g_migration.dest_session_id;
    return TM_OK;
}
