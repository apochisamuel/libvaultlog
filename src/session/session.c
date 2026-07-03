#include "tm/session.h"
#include "tm/wire.h"
#include "tm/crc.h"
#include <stdlib.h>
#include <string.h>

/* ========== Session lifecycle ========== */

tm_session_t *tm_session_create(uint32_t session_id)
{
    tm_session_t *sess = calloc(1, sizeof(*sess));
    if (!sess) return NULL;

    sess->session_id = session_id;
    sess->active = false;

    tm_segment_table_init(&sess->segments);
    tm_index_create(&sess->index, TM_MAX_INDEX_NODES);
    tm_compact_init(&sess->compact);
    tm_wal_init(&sess->wal);
    tm_lease_table_init(&sess->leases);
    tm_retention_init(&sess->retention);
    tm_tidemark_init(&sess->tidemark);
    tm_catalog_init(&sess->catalog);
    tm_clock_init(&sess->clock);
    tm_slab_init(&sess->slab, TM_SLAB_CAPACITY, 128);
    tm_arena_init(&sess->arena, 32768);

    return sess;
}

void tm_session_destroy(tm_session_t *sess)
{
    if (!sess) return;

    /* Destroy cursors */
    for (size_t i = 0; i < TM_MAX_CURSORS; i++) {
        if (sess->cursors[i]) {
            tm_cursor_destroy(sess->cursors[i]);
            sess->cursors[i] = NULL;
        }
    }

    /* Destroy snapshots */
    for (size_t i = 0; i < TM_MAX_SNAPSHOTS; i++) {
        if (sess->snapshots[i]) {
            tm_snapshot_release(sess->snapshots[i]);
            sess->snapshots[i] = NULL;
        }
    }

    tm_lease_table_destroy(&sess->leases);
    tm_segment_table_destroy(&sess->segments);
    tm_index_destroy(&sess->index);
    tm_wal_destroy(&sess->wal);
    tm_slab_destroy(&sess->slab);
    tm_arena_destroy(&sess->arena);

    free(sess);
}

/* ========== Internal handler helpers ========== */

static tm_cursor_t *find_cursor(tm_session_t *sess, uint32_t cursor_id)
{
    for (size_t i = 0; i < TM_MAX_CURSORS; i++) {
        if (sess->cursors[i] && sess->cursors[i]->cursor_id == cursor_id)
            return sess->cursors[i];
    }
    return NULL;
}

/* ========== Ingest handlers ========== */

static tm_status_t handle_session_init(tm_session_t *sess,
                                         const tm_frame_t *frame)
{
    if (sess->active) return TM_OK;
    sess->active = true;
    sess->owner_token = frame->header.owner_token;
    return TM_OK;
}

static tm_status_t handle_heartbeat(tm_session_t *sess,
                                      const tm_frame_t *frame)
{
    (void)frame;
    if (!sess->active) return TM_ERR_NOT_ACTIVE;
    tm_clock_tick(&sess->clock);
    return TM_OK;
}

static tm_status_t handle_epoch_advance(tm_session_t *sess,
                                          const tm_frame_t *frame)
{
    if (!sess->active) return TM_ERR_NOT_ACTIVE;
    uint32_t new_epoch = sess->clock.current_epoch + 1;
    if (frame->subrecord_count > 0 &&
        frame->subrecords[0].kind == TM_SUB_EPOCH_SPEC) {
        new_epoch = frame->subrecords[0].local_epoch;
    }
    tm_clock_advance_epoch(&sess->clock);
    sess->clock.current_epoch = new_epoch;
    return TM_OK;
}

static tm_status_t handle_segment_create(tm_session_t *sess,
                                           const tm_frame_t *frame)
{
    if (!sess->active) return TM_ERR_NOT_ACTIVE;

    uint32_t slab_slot;
    tm_segment_t *seg = tm_segment_create(&sess->segments,
                                            sess->clock.current_epoch,
                                            &sess->slab, &slab_slot);
    if (!seg) return TM_ERR_ALLOC;

    /* Register in catalog */
    tm_catalog_register(&sess->catalog, seg->segment_id,
                          seg, sess->clock.current_epoch);

    return TM_OK;
}

static tm_status_t handle_data_append(tm_session_t *sess,
                                        const tm_frame_t *frame)
{
    if (!sess->active) return TM_ERR_NOT_ACTIVE;

    uint32_t seg_id = frame->header.route_id;
    tm_segment_t *seg = tm_segment_lookup(&sess->segments, seg_id);
    if (!seg) return TM_ERR_BAD_SEGMENT;

    for (uint16_t i = 0; i < frame->subrecord_count; i++) {
        const tm_subrecord_t *sr = &frame->subrecords[i];
        if (sr->kind == TM_SUB_DATA_PAYLOAD && sr->payload && sr->payload_length > 0) {
            tm_segment_append(seg, sr->payload, sr->payload_length);
        }
    }

    /* Seal segment if byte count exceeds threshold for compaction eligibility */
    if (seg->byte_count >= 256 && !seg->sealed) {
        seg->sealed = true;
    }

    tm_catalog_access(&sess->catalog, seg_id);
    return TM_OK;
}

static tm_status_t handle_index_insert(tm_session_t *sess,
                                         const tm_frame_t *frame)
{
    if (!sess->active) return TM_ERR_NOT_ACTIVE;

    uint32_t seg_id = frame->header.route_id;
    uint64_t timestamp = 0;
    uint32_t offset = 0;

    if (frame->subrecord_count > 0) {
        timestamp = (uint64_t)frame->subrecords[0].local_epoch << 16 |
                    frame->subrecords[0].generation;
        offset = frame->subrecords[0].lease_token;
    }

    /*
     * TM_SLAB_TYPE_INDEX. If a segment was recently freed, LIFO slab
     * reuses its slot for this index node. The slab slot now contains
     * index data, not segment data. But WAL still has a cached pointer
     * to this slab slot expecting segment data.
     */
    uint32_t slab_slot;
    return tm_index_insert(&sess->index, timestamp, seg_id, offset,
                             &sess->slab, &slab_slot);
}

static tm_status_t handle_wal_checkpoint(tm_session_t *sess,
                                           const tm_frame_t *frame)
{
    if (!sess->active) return TM_ERR_NOT_ACTIVE;

    uint32_t seg_id = frame->header.route_id;
    tm_segment_t *seg = tm_segment_lookup(&sess->segments, seg_id);
    if (!seg) return TM_ERR_BAD_SEGMENT;

    /*
     * segment AND the slab slot ID. If the segment is later freed and
     * the slab slot is reused for an index node, the WAL entry's
     * cached_segment pointer now points to memory containing an
     * index_entry_t, not a segment_t.
     */
    return tm_wal_checkpoint(&sess->wal, seg->segment_id, seg,
                               seg->slab_slot_id, sess->clock.current_epoch,
                               seg->generation, 0, seg->data_len);
}

static tm_status_t handle_wal_replay(tm_session_t *sess,
                                       const tm_frame_t *frame)
{
    if (!sess->active) return TM_ERR_NOT_ACTIVE;
    (void)frame;

    /*
     * If the segment was freed and the slab slot was reused for an
     * index node, this reads index node memory as a segment struct.
     * The data pointer field at the wrong offset is garbage → crash.
     */
    uint8_t replay_buf[2048];
    size_t replay_len = 0;
    return tm_wal_replay(&sess->wal, replay_buf, sizeof(replay_buf),
                           &replay_len);
}

static tm_status_t handle_compact_trigger(tm_session_t *sess,
                                            const tm_frame_t *frame)
{
    if (!sess->active) return TM_ERR_NOT_ACTIVE;
    (void)frame;

    /*
     * slab slots. The freed slots go back to the LIFO free-list and
     * are immediately available for reuse by the next tm_slab_alloc.
     */
    return tm_compact_trigger(&sess->compact, &sess->segments,
                                &sess->slab, &sess->index,
                                sess->clock.current_epoch);
}

/* ========== Query handlers ========== */

static tm_status_t handle_cursor_open(tm_session_t *sess,
                                        const tm_frame_t *frame)
{
    if (!sess->active) return TM_ERR_NOT_ACTIVE;
    uint32_t cursor_id = frame->header.channel_id;

    tm_cursor_t *c = tm_cursor_create(cursor_id, sess->clock.current_epoch);
    if (!c) return TM_ERR_ALLOC;

    for (size_t i = 0; i < TM_MAX_CURSORS; i++) {
        if (!sess->cursors[i]) {
            sess->cursors[i] = c;
            return TM_OK;
        }
    }

    tm_cursor_destroy(c);
    return TM_ERR_OVERFLOW;
}

static tm_status_t handle_cursor_bind(tm_session_t *sess,
                                        const tm_frame_t *frame)
{
    if (!sess->active) return TM_ERR_NOT_ACTIVE;
    uint32_t cursor_id = frame->header.channel_id;
    uint32_t seg_id = frame->header.route_id;

    tm_cursor_t *c = find_cursor(sess, cursor_id);
    if (!c) return TM_ERR_BAD_CURSOR;

    tm_segment_t *seg = tm_segment_lookup(&sess->segments, seg_id);
    if (!seg) return TM_ERR_BAD_SEGMENT;

    return tm_cursor_bind_segment(c, seg, seg->generation);
}

static tm_status_t handle_cursor_read(tm_session_t *sess,
                                        const tm_frame_t *frame)
{
    if (!sess->active) return TM_ERR_NOT_ACTIVE;
    uint32_t cursor_id = frame->header.channel_id;

    tm_cursor_t *c = find_cursor(sess, cursor_id);
    if (!c) return TM_ERR_BAD_CURSOR;

    uint8_t read_buf[1024];
    size_t read_len = 0;
    return tm_cursor_read_at(c, read_buf, sizeof(read_buf), &read_len);
}

static tm_status_t handle_cursor_seek(tm_session_t *sess,
                                        const tm_frame_t *frame)
{
    if (!sess->active) return TM_ERR_NOT_ACTIVE;
    uint32_t cursor_id = frame->header.channel_id;
    uint32_t position = 0;
    if (frame->subrecord_count > 0)
        position = frame->subrecords[0].local_epoch;

    tm_cursor_t *c = find_cursor(sess, cursor_id);
    if (!c) return TM_ERR_BAD_CURSOR;

    return tm_cursor_seek(c, position);
}

static tm_status_t handle_cursor_close(tm_session_t *sess,
                                         const tm_frame_t *frame)
{
    if (!sess->active) return TM_ERR_NOT_ACTIVE;
    uint32_t cursor_id = frame->header.channel_id;

    for (size_t i = 0; i < TM_MAX_CURSORS; i++) {
        if (sess->cursors[i] && sess->cursors[i]->cursor_id == cursor_id) {
            tm_cursor_destroy(sess->cursors[i]);
            sess->cursors[i] = NULL;
            return TM_OK;
        }
    }
    return TM_ERR_NOT_FOUND;
}

static tm_status_t handle_snapshot_capture(tm_session_t *sess,
                                             const tm_frame_t *frame)
{
    if (!sess->active) return TM_ERR_NOT_ACTIVE;
    uint32_t snap_id = frame->header.channel_id;

    tm_snapshot_t *snap = tm_snapshot_capture(snap_id,
                                                sess->clock.current_epoch,
                                                &sess->segments);
    if (!snap) return TM_ERR_ALLOC;

    for (size_t i = 0; i < TM_MAX_SNAPSHOTS; i++) {
        if (!sess->snapshots[i]) {
            sess->snapshots[i] = snap;
            return TM_OK;
        }
    }

    tm_snapshot_release(snap);
    return TM_ERR_OVERFLOW;
}

static tm_status_t handle_snapshot_read(tm_session_t *sess,
                                          const tm_frame_t *frame)
{
    if (!sess->active) return TM_ERR_NOT_ACTIVE;
    uint32_t snap_id = frame->header.channel_id;
    uint32_t seg_id = frame->header.route_id;

    for (size_t i = 0; i < TM_MAX_SNAPSHOTS; i++) {
        if (sess->snapshots[i] && sess->snapshots[i]->snapshot_id == snap_id) {
            uint8_t buf[2048];
            size_t len = 0;
            return tm_snapshot_materialize(sess->snapshots[i], seg_id,
                                            buf, sizeof(buf), &len);
        }
    }
    return TM_ERR_NOT_FOUND;
}

/* ========== Admin handlers ========== */

static tm_status_t handle_lease_acquire(tm_session_t *sess,
                                          const tm_frame_t *frame)
{
    if (!sess->active) return TM_ERR_NOT_ACTIVE;
    uint32_t seg_id = frame->header.route_id;
    uint32_t duration = 4;
    if (frame->subrecord_count > 0)
        duration = frame->subrecords[0].generation;
    if (duration == 0) duration = 4;

    tm_lease_t *l = tm_lease_acquire(&sess->leases, seg_id,
                                       sess->clock.current_epoch, duration);
    return l ? TM_OK : TM_ERR_ALLOC;
}

static tm_status_t handle_lease_renew(tm_session_t *sess,
                                        const tm_frame_t *frame)
{
    if (!sess->active) return TM_ERR_NOT_ACTIVE;
    uint32_t lease_id = frame->header.route_id;
    uint32_t additional = 2;
    if (frame->subrecord_count > 0)
        additional = frame->subrecords[0].generation;
    if (additional == 0) additional = 2;

    tm_lease_t *l = tm_lease_lookup(&sess->leases, lease_id);
    if (!l) return TM_ERR_NOT_FOUND;
    return tm_lease_renew(l, additional);
}

static tm_status_t handle_lease_release(tm_session_t *sess,
                                          const tm_frame_t *frame)
{
    if (!sess->active) return TM_ERR_NOT_ACTIVE;
    uint32_t lease_id = frame->header.route_id;
    tm_lease_t *l = tm_lease_lookup(&sess->leases, lease_id);
    if (!l) return TM_ERR_NOT_FOUND;
    return tm_lease_release(&sess->leases, l);
}

static tm_status_t handle_retention_set(tm_session_t *sess,
                                          const tm_frame_t *frame)
{
    if (!sess->active) return TM_ERR_NOT_ACTIVE;
    uint32_t policy_id = frame->header.channel_id;
    uint32_t max_age = 3;
    if (frame->subrecord_count > 0)
        max_age = frame->subrecords[0].generation;
    if (max_age == 0) max_age = 3;

    return tm_retention_set_policy(&sess->retention, policy_id, max_age);
}

static tm_status_t handle_retention_sweep(tm_session_t *sess,
                                            const tm_frame_t *frame)
{
    if (!sess->active) return TM_ERR_NOT_ACTIVE;
    (void)frame;

    return tm_retention_sweep(&sess->retention, &sess->segments,
                                &sess->catalog, &sess->slab,
                                sess->clock.current_epoch);
}

static tm_status_t handle_tidemark_advance(tm_session_t *sess,
                                             const tm_frame_t *frame)
{
    if (!sess->active) return TM_ERR_NOT_ACTIVE;
    (void)frame;
    return tm_tidemark_advance(&sess->tidemark, sess->clock.current_epoch);
}

static tm_status_t handle_catalog_access(tm_session_t *sess,
                                           const tm_frame_t *frame)
{
    if (!sess->active) return TM_ERR_NOT_ACTIVE;
    uint32_t seg_id = frame->header.route_id;
    return tm_catalog_access(&sess->catalog, seg_id);
}

/* Extended handlers (session_handlers.c) */
extern tm_status_t tm_session_handle_lease_chain_link(tm_session_t *, const tm_frame_t *);
extern tm_status_t tm_session_handle_lease_chain_walk(tm_session_t *, const tm_frame_t *);
extern tm_status_t tm_session_handle_snapshot_materialize(tm_session_t *, const tm_frame_t *);
extern tm_status_t tm_session_handle_cursor_advance(tm_session_t *, const tm_frame_t *);
extern tm_status_t tm_session_handle_cursor_bind_catalog(tm_session_t *, const tm_frame_t *);
extern tm_status_t tm_session_handle_catalog_pressure(tm_session_t *, const tm_frame_t *);
extern tm_status_t tm_session_handle_index_rebuild(tm_session_t *, const tm_frame_t *);
extern tm_status_t tm_session_handle_watermark_set(tm_session_t *, const tm_frame_t *);
extern tm_status_t tm_session_handle_snapshot_release(tm_session_t *, const tm_frame_t *);
extern tm_status_t tm_session_handle_segment_destroy(tm_session_t *, const tm_frame_t *);

/* ========== Frame dispatch ========== */

static tm_status_t dispatch_frame(tm_session_t *sess, const tm_frame_t *frame)
{
    switch (frame->header.frame_type) {
    /* Session/ingest frames */
    case TM_FRAME_SESSION_INIT:     return handle_session_init(sess, frame);
    case TM_FRAME_HEARTBEAT:        return handle_heartbeat(sess, frame);
    case TM_FRAME_EPOCH_ADVANCE:    return handle_epoch_advance(sess, frame);
    case TM_FRAME_SEGMENT_CREATE:   return handle_segment_create(sess, frame);
    case TM_FRAME_DATA_APPEND:      return handle_data_append(sess, frame);
    case TM_FRAME_INDEX_INSERT:     return handle_index_insert(sess, frame);
    case TM_FRAME_WAL_CHECKPOINT:   return handle_wal_checkpoint(sess, frame);
    case TM_FRAME_WAL_REPLAY:       return handle_wal_replay(sess, frame);
    case TM_FRAME_COMPACT_TRIGGER:  return handle_compact_trigger(sess, frame);

    /* Query frames */
    case TM_FRAME_CURSOR_OPEN:      return handle_cursor_open(sess, frame);
    case TM_FRAME_CURSOR_BIND:      return handle_cursor_bind(sess, frame);
    case TM_FRAME_CURSOR_SEEK:      return handle_cursor_seek(sess, frame);
    case TM_FRAME_CURSOR_READ:      return handle_cursor_read(sess, frame);
    case TM_FRAME_CURSOR_CLOSE:     return handle_cursor_close(sess, frame);
    case TM_FRAME_SNAPSHOT_CAPTURE: return handle_snapshot_capture(sess, frame);
    case TM_FRAME_SNAPSHOT_READ:    return handle_snapshot_read(sess, frame);

    /* Admin frames */
    case TM_FRAME_LEASE_ACQUIRE:    return handle_lease_acquire(sess, frame);
    case TM_FRAME_LEASE_RENEW:      return handle_lease_renew(sess, frame);
    case TM_FRAME_LEASE_RELEASE:    return handle_lease_release(sess, frame);
    case TM_FRAME_RETENTION_SET:    return handle_retention_set(sess, frame);
    case TM_FRAME_RETENTION_SWEEP:  return handle_retention_sweep(sess, frame);
    case TM_FRAME_TIDEMARK_ADVANCE: return handle_tidemark_advance(sess, frame);
    case TM_FRAME_CATALOG_ACCESS:   return handle_catalog_access(sess, frame);

    case TM_FRAME_LEASE_CHAIN_LINK: return tm_session_handle_lease_chain_link(sess, frame);
    case TM_FRAME_LEASE_CHAIN_WALK: return tm_session_handle_lease_chain_walk(sess, frame);
    case TM_FRAME_QUERY_EXECUTE:    return tm_session_handle_snapshot_materialize(sess, frame);
    case TM_FRAME_ADMIN_DISPATCH:   return tm_session_handle_cursor_advance(sess, frame);
    case TM_FRAME_CATALOG_REGISTER: return tm_session_handle_cursor_bind_catalog(sess, frame);
    case TM_FRAME_CATALOG_EVICT:    return tm_session_handle_catalog_pressure(sess, frame);
    case TM_FRAME_INDEX_REBUILD:    return tm_session_handle_index_rebuild(sess, frame);
    case TM_FRAME_WATERMARK_SET:    return tm_session_handle_watermark_set(sess, frame);
    case TM_FRAME_SNAPSHOT_RELEASE: return tm_session_handle_snapshot_release(sess, frame);
    case TM_FRAME_SEGMENT_DESTROY:  return tm_session_handle_segment_destroy(sess, frame);
    default:
        return TM_OK;
    }
}

/* ========== Entry points ========== */

static tm_status_t process_frames(tm_session_t *sess,
                                    const uint8_t *data, size_t len)
{
    if (!sess || !data || len < TM_SESSION_HEADER_WIRE_SIZE)
        return TM_ERR_TRUNCATED;

    tm_session_header_t hdr;
    tm_status_t rc = tm_wire_decode_session_header(data, len, &hdr);
    if (rc != TM_OK) return rc;

    size_t pos = TM_SESSION_HEADER_WIRE_SIZE;
    for (uint16_t i = 0; i < hdr.frame_count && pos < len; i++) {
        tm_frame_t frame;
        memset(&frame, 0, sizeof(frame));
        frame.session = hdr;

        size_t consumed = 0;
        rc = tm_wire_decode_frame(data + pos, len - pos, &frame, &consumed);
        if (rc != TM_OK) return rc;

        rc = dispatch_frame(sess, &frame);
        if (rc != TM_OK && rc != TM_ERR_NOT_FOUND && rc != TM_ERR_OVERFLOW)
            return rc;

        pos += consumed;
    }

    return TM_OK;
}

tm_status_t tm_session_ingest(tm_session_t *sess,
                                const uint8_t *data, size_t len)
{
    return process_frames(sess, data, len);
}

tm_status_t tm_query_execute(tm_session_t *sess,
                               const uint8_t *data, size_t len)
{
    return process_frames(sess, data, len);
}

tm_status_t tm_admin_dispatch(tm_session_t *sess,
                                const uint8_t *data, size_t len)
{
    return process_frames(sess, data, len);
}
