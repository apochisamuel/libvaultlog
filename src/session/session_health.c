#include "tm/session.h"
#include <string.h>
#include <stdlib.h>

typedef enum {
    TM_HEALTH_OK       = 0,
    TM_HEALTH_WARNING  = 1,
    TM_HEALTH_CRITICAL = 2,
    TM_HEALTH_FAILED   = 3
} tm_health_level_t;

typedef struct {
    tm_health_level_t  overall;
    tm_health_level_t  segment_health;
    tm_health_level_t  index_health;
    tm_health_level_t  wal_health;
    tm_health_level_t  lease_health;
    tm_health_level_t  catalog_health;
    tm_health_level_t  slab_health;
    uint32_t           warning_count;
    uint32_t           critical_count;
    char               messages[8][128];
    uint32_t           message_count;
} tm_health_report_t;

static void add_message(tm_health_report_t *r, const char *msg)
{
    if (r->message_count < 8) {
        size_t len = strlen(msg);
        if (len > 127) len = 127;
        memcpy(r->messages[r->message_count], msg, len);
        r->messages[r->message_count][len] = 0;
        r->message_count++;
    }
}

tm_status_t tm_session_health_check(const tm_session_t *sess,
                                       tm_health_report_t *report)
{
    if (!sess || !report) return TM_ERR_BAD_STATE;
    memset(report, 0, sizeof(*report));

    /* Segment health */
    uint32_t seg_count = 0, sealed = 0;
    for (size_t i = 0; i < TM_MAX_SEGMENTS; i++) {
        if (sess->segments.segments[i] && sess->segments.segments[i]->active) {
            seg_count++;
            if (sess->segments.segments[i]->sealed) sealed++;
        }
    }
    if (seg_count >= TM_MAX_SEGMENTS - 2) {
        report->segment_health = TM_HEALTH_CRITICAL;
        add_message(report, "Segment table near capacity");
        report->critical_count++;
    } else if (sealed > seg_count / 2 + 1) {
        report->segment_health = TM_HEALTH_WARNING;
        add_message(report, "Many sealed segments pending compaction");
        report->warning_count++;
    }

    /* Index health */
    if (sess->index.count >= sess->index.capacity - 2) {
        report->index_health = TM_HEALTH_CRITICAL;
        add_message(report, "Index near capacity");
        report->critical_count++;
    } else if (sess->index.count > sess->index.capacity * 3 / 4) {
        report->index_health = TM_HEALTH_WARNING;
        add_message(report, "Index fill factor high");
        report->warning_count++;
    }

    /* WAL health */
    if (sess->wal.has_gap) {
        report->wal_health = TM_HEALTH_CRITICAL;
        add_message(report, "WAL has replay gap");
        report->critical_count++;
    } else if (sess->wal.count >= TM_MAX_WAL_ENTRIES - 4) {
        report->wal_health = TM_HEALTH_WARNING;
        add_message(report, "WAL near capacity");
        report->warning_count++;
    }

    /* Lease health */
    if (sess->leases.used_quota >= sess->leases.total_quota) {
        report->lease_health = TM_HEALTH_CRITICAL;
        add_message(report, "Lease quota exhausted");
        report->critical_count++;
    }

    /* Catalog health */
    if (sess->catalog.count >= sess->catalog.capacity) {
        report->catalog_health = TM_HEALTH_CRITICAL;
        add_message(report, "Catalog full");
        report->critical_count++;
    } else if (sess->catalog.count >= sess->catalog.eviction_threshold) {
        report->catalog_health = TM_HEALTH_WARNING;
        add_message(report, "Catalog under eviction pressure");
        report->warning_count++;
    }

    /* Slab health */
    if (sess->slab.free_top == 0) {
        report->slab_health = TM_HEALTH_CRITICAL;
        add_message(report, "Slab pool exhausted");
        report->critical_count++;
    } else if (sess->slab.free_top < sess->slab.capacity / 4) {
        report->slab_health = TM_HEALTH_WARNING;
        add_message(report, "Slab pool low");
        report->warning_count++;
    }

    /* Overall */
    if (report->critical_count > 0)
        report->overall = TM_HEALTH_CRITICAL;
    else if (report->warning_count > 0)
        report->overall = TM_HEALTH_WARNING;
    else
        report->overall = TM_HEALTH_OK;

    return TM_OK;
}
