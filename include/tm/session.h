#ifndef TM_SESSION_H
#define TM_SESSION_H

#include "tm/types.h"
#include "tm/error.h"
#include "tm/segment.h"
#include "tm/index.h"
#include "tm/cursor.h"
#include "tm/snapshot.h"
#include "tm/compact.h"
#include "tm/wal.h"
#include "tm/lease.h"
#include "tm/retention.h"
#include "tm/tidemark.h"
#include "tm/catalog.h"
#include "tm/clock.h"
#include "tm/slab.h"
#include "tm/core.h"

typedef struct {
    uint32_t            session_id;
    bool                active;
    uint32_t            sequence;
    uint32_t            owner_token;

    /* Subsystem state */
    tm_segment_table_t  segments;
    tm_index_t          index;
    tm_cursor_t        *cursors[TM_MAX_CURSORS];
    tm_snapshot_t      *snapshots[TM_MAX_SNAPSHOTS];
    tm_compact_ctx_t    compact;
    tm_wal_t            wal;
    tm_lease_table_t    leases;
    tm_retention_ctx_t  retention;
    tm_tidemark_t       tidemark;
    tm_catalog_t        catalog;
    tm_clock_t          clock;
    tm_slab_t           slab;
    tm_arena_t          arena;
} tm_session_t;

tm_session_t *tm_session_create(uint32_t session_id);
void          tm_session_destroy(tm_session_t *sess);

/* Entry points for different harnesses */
tm_status_t   tm_session_ingest(tm_session_t *sess,
                                  const uint8_t *data, size_t len);
tm_status_t   tm_query_execute(tm_session_t *sess,
                                 const uint8_t *data, size_t len);
tm_status_t   tm_admin_dispatch(tm_session_t *sess,
                                  const uint8_t *data, size_t len);

#endif
