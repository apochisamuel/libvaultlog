#include "tm/lease.h"
#include <string.h>

typedef struct {
    uint32_t  max_leases_per_segment;
    uint32_t  max_lease_duration;
    uint32_t  max_renewals;
    uint32_t  min_remaining_for_renew;
    bool      allow_chains;
} tm_lease_policy_t;

static tm_lease_policy_t g_lease_policy = {
    .max_leases_per_segment = 4,
    .max_lease_duration = 100,
    .max_renewals = 10,
    .min_remaining_for_renew = 1,
    .allow_chains = true
};

tm_status_t tm_lease_set_policy(const tm_lease_policy_t *policy)
{
    if (!policy) return TM_ERR_BAD_STATE;
    g_lease_policy = *policy;
    return TM_OK;
}

tm_status_t tm_lease_get_policy(tm_lease_policy_t *policy)
{
    if (!policy) return TM_ERR_BAD_STATE;
    *policy = g_lease_policy;
    return TM_OK;
}

bool tm_lease_policy_allows_acquire(const tm_lease_table_t *lt,
                                       uint32_t segment_id)
{
    if (!lt) return false;
    uint32_t count = 0;
    for (size_t i = 0; i < TM_MAX_LEASES; i++) {
        const tm_lease_t *l = lt->leases[i];
        if (l && l->active && l->segment_id == segment_id)
            count++;
    }
    return count < g_lease_policy.max_leases_per_segment;
}

bool tm_lease_policy_allows_renew(const tm_lease_t *lease,
                                     uint32_t current_epoch)
{
    if (!lease || !lease->active) return false;
    if (lease->renewal_count >= g_lease_policy.max_renewals) return false;
    if (lease->epoch_expires > current_epoch + g_lease_policy.min_remaining_for_renew)
        return true;
    return false;
}

bool tm_lease_policy_allows_chain(void)
{
    return g_lease_policy.allow_chains;
}

uint32_t tm_lease_policy_clamp_duration(uint32_t requested)
{
    if (requested > g_lease_policy.max_lease_duration)
        return g_lease_policy.max_lease_duration;
    return requested;
}
