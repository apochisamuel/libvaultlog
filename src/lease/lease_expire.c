#include "tm/lease.h"
#include <string.h>

typedef struct {
    uint32_t  expired_ids[TM_MAX_LEASES];
    uint32_t  expired_count;
    uint32_t  renewed_count;
    uint32_t  active_count;
} tm_lease_expiry_result_t;

tm_status_t tm_lease_check_expiry(tm_lease_table_t *lt,
                                     uint32_t current_epoch,
                                     tm_lease_expiry_result_t *result)
{
    if (!lt || !result) return TM_ERR_BAD_STATE;
    memset(result, 0, sizeof(*result));

    for (size_t i = 0; i < TM_MAX_LEASES; i++) {
        tm_lease_t *l = lt->leases[i];
        if (!l || !l->active) continue;

        if (l->epoch_expires <= current_epoch) {
            l->expired = true;
            if (result->expired_count < TM_MAX_LEASES) {
                result->expired_ids[result->expired_count++] = l->lease_id;
            }
        } else {
            result->active_count++;
        }

        if (l->renewal_count > 0) result->renewed_count++;
    }

    return TM_OK;
}

tm_status_t tm_lease_expire_by_policy(tm_lease_table_t *lt,
                                         uint32_t current_epoch,
                                         uint32_t max_age)
{
    if (!lt) return TM_ERR_BAD_STATE;

    for (size_t i = 0; i < TM_MAX_LEASES; i++) {
        tm_lease_t *l = lt->leases[i];
        if (!l || !l->active) continue;

        uint32_t age = current_epoch - l->epoch_acquired;
        if (age >= max_age && l->epoch_expires <= current_epoch) {
            l->expired = true;
            l->active = false;
        }
    }

    return TM_OK;
}

tm_status_t tm_lease_gc_expired(tm_lease_table_t *lt)
{
    if (!lt) return TM_ERR_BAD_STATE;

    for (size_t i = 0; i < TM_MAX_LEASES; i++) {
        tm_lease_t *l = lt->leases[i];
        if (!l) continue;

        if (l->expired && !l->active) {
            /* Unlink from any chain */
            if (l->chain_prev) l->chain_prev->chain_next = l->chain_next;
            if (l->chain_next) l->chain_next->chain_prev = l->chain_prev;
            lt->leases[i] = NULL;
            lt->count--;
            lt->used_quota--;
            free(l);
        }
    }

    return TM_OK;
}

tm_status_t tm_lease_extend_all(tm_lease_table_t *lt,
                                   uint32_t additional_epochs)
{
    if (!lt) return TM_ERR_BAD_STATE;

    for (size_t i = 0; i < TM_MAX_LEASES; i++) {
        tm_lease_t *l = lt->leases[i];
        if (l && l->active && !l->expired) {
            l->epoch_expires += additional_epochs;
            l->renewal_count++;
        }
    }

    return TM_OK;
}
