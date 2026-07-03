#ifndef TM_LEASE_H
#define TM_LEASE_H
#include "tm/types.h"
#include "tm/error.h"

typedef struct tm_lease {
    uint32_t       lease_id;
    uint32_t       segment_id;
    uint32_t       epoch_acquired;
    uint32_t       epoch_expires;
    uint32_t       renewal_count;
    struct tm_lease *chain_next;    /* linked lease chain */
    struct tm_lease *chain_prev;
    bool           active;
    bool           expired;
} tm_lease_t;

typedef struct {
    tm_lease_t *leases[TM_MAX_LEASES];
    size_t      count;
    uint32_t    next_id;
    uint32_t    total_quota;
    uint32_t    used_quota;
} tm_lease_table_t;

tm_status_t   tm_lease_table_init(tm_lease_table_t *lt);
void          tm_lease_table_destroy(tm_lease_table_t *lt);
tm_lease_t   *tm_lease_acquire(tm_lease_table_t *lt, uint32_t segment_id,
                                 uint32_t epoch, uint32_t duration);
tm_lease_t   *tm_lease_lookup(tm_lease_table_t *lt, uint32_t lease_id);
tm_status_t   tm_lease_renew(tm_lease_t *lease, uint32_t additional_epochs);
tm_status_t   tm_lease_release(tm_lease_table_t *lt, tm_lease_t *lease);
tm_status_t   tm_lease_chain_link(tm_lease_t *a, tm_lease_t *b);
tm_status_t   tm_lease_chain_walk(tm_lease_t *start, uint8_t *dst,
                                    size_t dst_cap, size_t *out_len);
#endif
