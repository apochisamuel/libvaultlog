#include "tm/session.h"
#include "tm/wire.h"
#include <string.h>
#include <stdlib.h>

/*
 * Ingest-specific session operations: data ingestion pipeline,
 * batch append, and segment lifecycle management.
 */

typedef struct {
    uint32_t  frames_ingested;
    uint32_t  bytes_ingested;
    uint32_t  segments_created;
    uint32_t  segments_sealed;
    uint32_t  wal_checkpoints;
    uint32_t  index_inserts;
    uint32_t  epochs_advanced;
} tm_ingest_stats_t;

static tm_ingest_stats_t g_ingest_stats;

tm_status_t tm_ingest_reset_stats(void)
{
    memset(&g_ingest_stats, 0, sizeof(g_ingest_stats));
    return TM_OK;
}

tm_status_t tm_ingest_get_stats(tm_ingest_stats_t *stats)
{
    if (!stats) return TM_ERR_BAD_STATE;
    memcpy(stats, &g_ingest_stats, sizeof(*stats));
    return TM_OK;
}

tm_status_t tm_ingest_create_segment(tm_session_t *sess, uint32_t *out_id)
{
    if (!sess || !sess->active) return TM_ERR_NOT_ACTIVE;

    uint32_t slab_slot;
    tm_segment_t *seg = tm_segment_create(&sess->segments,
                                             sess->clock.current_epoch,
                                             &sess->slab, &slab_slot);
    if (!seg) return TM_ERR_ALLOC;

    tm_catalog_register(&sess->catalog, seg->segment_id,
                          seg, sess->clock.current_epoch);

    if (out_id) *out_id = seg->segment_id;
    g_ingest_stats.segments_created++;
    return TM_OK;
}

tm_status_t tm_ingest_append_data(tm_session_t *sess, uint32_t segment_id,
                                     const uint8_t *data, size_t len)
{
    if (!sess || !sess->active) return TM_ERR_NOT_ACTIVE;

    tm_segment_t *seg = tm_segment_lookup(&sess->segments, segment_id);
    if (!seg) return TM_ERR_BAD_SEGMENT;

    tm_status_t rc = tm_segment_append(seg, data, len);
    if (rc != TM_OK) return rc;

    g_ingest_stats.bytes_ingested += (uint32_t)len;
    g_ingest_stats.frames_ingested++;

    /* Auto-seal if threshold reached */
    if (seg->byte_count >= 256 && !seg->sealed) {
        seg->sealed = true;
        g_ingest_stats.segments_sealed++;
    }

    tm_catalog_access(&sess->catalog, segment_id);
    return TM_OK;
}

tm_status_t tm_ingest_checkpoint_segment(tm_session_t *sess,
                                            uint32_t segment_id)
{
    if (!sess || !sess->active) return TM_ERR_NOT_ACTIVE;

    tm_segment_t *seg = tm_segment_lookup(&sess->segments, segment_id);
    if (!seg) return TM_ERR_BAD_SEGMENT;

    tm_status_t rc = tm_wal_checkpoint(&sess->wal, seg->segment_id,
                                          seg, seg->slab_slot_id,
                                          sess->clock.current_epoch,
                                          seg->generation, 0, seg->data_len);
    if (rc == TM_OK) g_ingest_stats.wal_checkpoints++;
    return rc;
}

tm_status_t tm_ingest_batch_append(tm_session_t *sess,
                                      uint32_t segment_id,
                                      const uint8_t *const *chunks,
                                      const size_t *chunk_sizes,
                                      uint32_t chunk_count)
{
    if (!sess || !sess->active || !chunks || !chunk_sizes)
        return TM_ERR_BAD_STATE;

    for (uint32_t i = 0; i < chunk_count; i++) {
        tm_status_t rc = tm_ingest_append_data(sess, segment_id,
                                                  chunks[i], chunk_sizes[i]);
        if (rc != TM_OK) return rc;
    }

    return TM_OK;
}

tm_status_t tm_ingest_advance_epoch(tm_session_t *sess)
{
    if (!sess || !sess->active) return TM_ERR_NOT_ACTIVE;
    tm_clock_advance_epoch(&sess->clock);
    g_ingest_stats.epochs_advanced++;
    return TM_OK;
}

tm_status_t tm_ingest_insert_index(tm_session_t *sess,
                                      uint64_t timestamp,
                                      uint32_t segment_id,
                                      uint32_t offset)
{
    if (!sess || !sess->active) return TM_ERR_NOT_ACTIVE;

    uint32_t slab_slot;
    tm_status_t rc = tm_index_insert(&sess->index, timestamp,
                                        segment_id, offset,
                                        &sess->slab, &slab_slot);
    if (rc == TM_OK) g_ingest_stats.index_inserts++;
    return rc;
}
