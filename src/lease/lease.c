#include "tm/lease.h"
#include <stdlib.h>
#include <string.h>

tm_status_t tm_lease_table_init(tm_lease_table_t *lt)
{
    if (!lt) return TM_ERR_BAD_STATE;
    memset(lt, 0, sizeof(*lt));
    lt->next_id = 1;
    lt->total_quota = TM_MAX_LEASES;
    return TM_OK;
}

void tm_lease_table_destroy(tm_lease_table_t *lt)
{
    if (!lt) return;
    for (size_t i = 0; i < TM_MAX_LEASES; i++) {
        free(lt->leases[i]);
        lt->leases[i] = NULL;
    }
    lt->count = 0;
}

tm_lease_t *tm_lease_acquire(tm_lease_table_t *lt, uint32_t segment_id,
                               uint32_t epoch, uint32_t duration)
{
    if (!lt || lt->used_quota >= lt->total_quota) return NULL;
    if (lt->count >= TM_MAX_LEASES) return NULL;

    tm_lease_t *l = calloc(1, sizeof(*l));
    if (!l) return NULL;
    l->lease_id       = lt->next_id++;
    l->segment_id     = segment_id;
    l->epoch_acquired = epoch;
    l->epoch_expires  = epoch + duration;
    l->active         = true;

    for (size_t i = 0; i < TM_MAX_LEASES; i++) {
        if (!lt->leases[i]) {
            lt->leases[i] = l;
            lt->count++;
            lt->used_quota++;
            return l;
        }
    }

    free(l);
    return NULL;
}

tm_lease_t *tm_lease_lookup(tm_lease_table_t *lt, uint32_t lease_id)
{
    if (!lt) return NULL;
    for (size_t i = 0; i < TM_MAX_LEASES; i++) {
        if (lt->leases[i] && lt->leases[i]->active &&
            lt->leases[i]->lease_id == lease_id)
            return lt->leases[i];
    }
    return NULL;
}

tm_status_t tm_lease_renew(tm_lease_t *lease, uint32_t additional_epochs)
{
    if (!lease || !lease->active) return TM_ERR_BAD_STATE;
    lease->epoch_expires += additional_epochs;
    lease->renewal_count++;
    return TM_OK;
}

tm_status_t tm_lease_release(tm_lease_table_t *lt, tm_lease_t *lease)
{
    if (!lt || !lease) return TM_ERR_BAD_STATE;
    for (size_t i = 0; i < TM_MAX_LEASES; i++) {
        if (lt->leases[i] == lease) {
            lt->leases[i] = NULL;
            lt->count--;
            lt->used_quota--;
            break;
        }
    }
    lease->active = false;
    free(lease);
    return TM_OK;
}

tm_status_t tm_lease_chain_link(tm_lease_t *a, tm_lease_t *b)
{
    if (!a || !b || !a->active || !b->active) return TM_ERR_BAD_STATE;
    a->chain_next = b;
    b->chain_prev = a;
    return TM_OK;
}

tm_status_t tm_lease_chain_walk(tm_lease_t *start, uint8_t *dst,
                                  size_t dst_cap, size_t *out_len)
{
    if (!start || !dst || !out_len) return TM_ERR_BAD_STATE;

    size_t written = 0;
    tm_lease_t *current = start;
    while (current && written + 8 <= dst_cap) {
        /* Write lease_id and segment_id for each chain node */
        memcpy(dst + written, &current->lease_id, 4);
        written += 4;
        memcpy(dst + written, &current->segment_id, 4);
        written += 4;
        current = current->chain_next;
    }

    *out_len = written;
    return TM_OK;
}
