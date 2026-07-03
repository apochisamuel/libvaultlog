#include "tm/lease.h"
#include <string.h>

tm_status_t tm_lease_validate_chain(const tm_lease_t *start,
                                       uint32_t *chain_length,
                                       bool *has_cycle)
{
    if (!start || !chain_length || !has_cycle) return TM_ERR_BAD_STATE;
    *chain_length = 0;
    *has_cycle = false;

    const tm_lease_t *slow = start;
    const tm_lease_t *fast = start;

    while (fast && fast->chain_next) {
        slow = slow->chain_next;
        fast = fast->chain_next->chain_next;
        (*chain_length)++;

        if (slow == fast) {
            *has_cycle = true;
            return TM_OK;
        }
        if (*chain_length > TM_MAX_LEASES) break;
    }

    /* Count actual length */
    *chain_length = 0;
    const tm_lease_t *cur = start;
    while (cur && *chain_length < TM_MAX_LEASES) {
        (*chain_length)++;
        cur = cur->chain_next;
    }

    return TM_OK;
}

tm_status_t tm_lease_validate_table(const tm_lease_table_t *lt,
                                       uint32_t *active_count,
                                       uint32_t *expired_count,
                                       uint32_t *chained_count)
{
    if (!lt || !active_count || !expired_count || !chained_count)
        return TM_ERR_BAD_STATE;
    *active_count = 0;
    *expired_count = 0;
    *chained_count = 0;

    for (size_t i = 0; i < TM_MAX_LEASES; i++) {
        const tm_lease_t *l = lt->leases[i];
        if (!l) continue;
        if (l->active) (*active_count)++;
        if (l->expired) (*expired_count)++;
        if (l->chain_next || l->chain_prev) (*chained_count)++;
    }

    return TM_OK;
}

bool tm_lease_table_quota_available(const tm_lease_table_t *lt)
{
    return lt ? (lt->used_quota < lt->total_quota) : false;
}

uint32_t tm_lease_table_quota_used(const tm_lease_table_t *lt)
{
    return lt ? lt->used_quota : 0;
}
