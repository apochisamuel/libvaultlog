#include "tm/lease.h"
#include <string.h>

/*
 * Lease chain management: chain construction, validation,
 * and traversal operations.
 */

uint32_t tm_lease_chain_length(const tm_lease_t *start)
{
    if (!start) return 0;
    uint32_t count = 0;
    const tm_lease_t *current = start;
    while (current) {
        count++;
        current = current->chain_next;
        if (count > TM_MAX_LEASES) break; /* cycle protection */
    }
    return count;
}

tm_lease_t *tm_lease_chain_tail(tm_lease_t *start)
{
    if (!start) return NULL;
    tm_lease_t *current = start;
    uint32_t safety = 0;
    while (current->chain_next && safety < TM_MAX_LEASES) {
        current = current->chain_next;
        safety++;
    }
    return current;
}

tm_status_t tm_lease_chain_unlink(tm_lease_t *lease)
{
    if (!lease) return TM_ERR_BAD_STATE;
    if (lease->chain_prev) lease->chain_prev->chain_next = lease->chain_next;
    if (lease->chain_next) lease->chain_next->chain_prev = lease->chain_prev;
    lease->chain_prev = NULL;
    lease->chain_next = NULL;
    return TM_OK;
}

bool tm_lease_chain_contains(const tm_lease_t *start, uint32_t lease_id)
{
    const tm_lease_t *current = start;
    uint32_t safety = 0;
    while (current && safety < TM_MAX_LEASES) {
        if (current->lease_id == lease_id) return true;
        current = current->chain_next;
        safety++;
    }
    return false;
}

typedef struct {
    uint32_t  chain_count;
    uint32_t  total_leases;
    uint32_t  max_chain_length;
    uint32_t  expired_count;
} tm_lease_stats_t;

tm_status_t tm_lease_compute_stats(const tm_lease_table_t *lt,
                                      uint32_t current_epoch,
                                      tm_lease_stats_t *stats)
{
    if (!lt || !stats) return TM_ERR_BAD_STATE;
    memset(stats, 0, sizeof(*stats));

    for (size_t i = 0; i < TM_MAX_LEASES; i++) {
        if (!lt->leases[i]) continue;
        stats->total_leases++;

        if (lt->leases[i]->epoch_expires <= current_epoch)
            stats->expired_count++;

        if (!lt->leases[i]->chain_prev) {
            /* This is a chain head */
            stats->chain_count++;
            uint32_t len = tm_lease_chain_length(lt->leases[i]);
            if (len > stats->max_chain_length)
                stats->max_chain_length = len;
        }
    }

    return TM_OK;
}
