#include "tm/session.h"
#include <string.h>

typedef struct {
    uint32_t  lease_ids[TM_MAX_LEASES];
    uint32_t  segment_ids[TM_MAX_LEASES];
    uint32_t  expiry_epochs[TM_MAX_LEASES];
    uint32_t  active_count;
    uint32_t  expired_count;
    uint32_t  chained_count;
    uint32_t  quota_used;
    uint32_t  quota_total;
} tm_lease_inventory_t;

tm_status_t tm_session_lease_inventory(const tm_session_t *sess,
                                         tm_lease_inventory_t *inv)
{
    if (!sess || !inv) return TM_ERR_BAD_STATE;
    memset(inv, 0, sizeof(*inv));
    inv->quota_used = sess->leases.used_quota;
    inv->quota_total = sess->leases.total_quota;
    for (size_t i = 0; i < TM_MAX_LEASES; i++) {
        const tm_lease_t *l = sess->leases.leases[i];
        if (!l) continue;
        uint32_t idx = inv->active_count + inv->expired_count;
        if (idx >= TM_MAX_LEASES) break;
        inv->lease_ids[idx] = l->lease_id;
        inv->segment_ids[idx] = l->segment_id;
        inv->expiry_epochs[idx] = l->epoch_expires;
        if (l->active && !l->expired) inv->active_count++;
        else inv->expired_count++;
        if (l->chain_next || l->chain_prev) inv->chained_count++;
    }
    return TM_OK;
}

tm_status_t tm_session_expire_all_leases(tm_session_t *sess)
{
    if (!sess) return TM_ERR_BAD_STATE;
    for (size_t i = 0; i < TM_MAX_LEASES; i++) {
        tm_lease_t *l = sess->leases.leases[i];
        if (l && l->active) {
            l->active = false;
            l->expired = true;
        }
    }
    return TM_OK;
}

tm_status_t tm_session_renew_expiring_leases(tm_session_t *sess,
                                                uint32_t window,
                                                uint32_t extension)
{
    if (!sess) return TM_ERR_BAD_STATE;
    uint32_t epoch = sess->clock.current_epoch;
    for (size_t i = 0; i < TM_MAX_LEASES; i++) {
        tm_lease_t *l = sess->leases.leases[i];
        if (!l || !l->active || l->expired) continue;
        if (l->epoch_expires <= epoch + window) {
            l->epoch_expires += extension;
            l->renewal_count++;
        }
    }
    return TM_OK;
}

tm_status_t tm_session_gc_expired_leases(tm_session_t *sess)
{
    if (!sess) return TM_ERR_BAD_STATE;
    for (size_t i = 0; i < TM_MAX_LEASES; i++) {
        tm_lease_t *l = sess->leases.leases[i];
        if (!l || l->active) continue;
        if (l->expired) {
            if (l->chain_prev) l->chain_prev->chain_next = l->chain_next;
            if (l->chain_next) l->chain_next->chain_prev = l->chain_prev;
            sess->leases.leases[i] = NULL;
            sess->leases.count--;
            sess->leases.used_quota--;
            free(l);
        }
    }
    return TM_OK;
}
