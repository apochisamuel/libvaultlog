#include "tm/session.h"
#include <string.h>

typedef enum {
    TM_SESS_STATE_INIT       = 0,
    TM_SESS_STATE_ACTIVE     = 1,
    TM_SESS_STATE_MIGRATING  = 2,
    TM_SESS_STATE_RECOVERING = 3,
    TM_SESS_STATE_DRAINING   = 4,
    TM_SESS_STATE_CLOSED     = 5
} tm_session_state_t;

typedef struct {
    tm_session_state_t state;
    uint32_t           transition_count;
    uint32_t           last_transition_epoch;
    uint32_t           error_count;
    uint32_t           recovery_attempts;
} tm_session_lifecycle_t;

static tm_session_lifecycle_t g_lifecycle[8]; /* max 8 concurrent sessions */

tm_status_t tm_lifecycle_init(uint32_t session_id)
{
    uint32_t idx = session_id % 8;
    memset(&g_lifecycle[idx], 0, sizeof(tm_session_lifecycle_t));
    g_lifecycle[idx].state = TM_SESS_STATE_INIT;
    return TM_OK;
}

tm_status_t tm_lifecycle_transition(uint32_t session_id,
                                       tm_session_state_t new_state,
                                       uint32_t epoch)
{
    uint32_t idx = session_id % 8;
    tm_session_lifecycle_t *lc = &g_lifecycle[idx];

    /* Validate transition */
    switch (lc->state) {
    case TM_SESS_STATE_INIT:
        if (new_state != TM_SESS_STATE_ACTIVE) return TM_ERR_BAD_STATE;
        break;
    case TM_SESS_STATE_ACTIVE:
        if (new_state != TM_SESS_STATE_MIGRATING &&
            new_state != TM_SESS_STATE_DRAINING &&
            new_state != TM_SESS_STATE_CLOSED)
            return TM_ERR_BAD_STATE;
        break;
    case TM_SESS_STATE_MIGRATING:
        if (new_state != TM_SESS_STATE_ACTIVE &&
            new_state != TM_SESS_STATE_RECOVERING)
            return TM_ERR_BAD_STATE;
        break;
    case TM_SESS_STATE_RECOVERING:
        if (new_state != TM_SESS_STATE_ACTIVE &&
            new_state != TM_SESS_STATE_CLOSED)
            return TM_ERR_BAD_STATE;
        break;
    case TM_SESS_STATE_DRAINING:
        if (new_state != TM_SESS_STATE_CLOSED)
            return TM_ERR_BAD_STATE;
        break;
    case TM_SESS_STATE_CLOSED:
        return TM_ERR_BAD_STATE;
    }

    lc->state = new_state;
    lc->transition_count++;
    lc->last_transition_epoch = epoch;
    return TM_OK;
}

tm_session_state_t tm_lifecycle_get_state(uint32_t session_id)
{
    return g_lifecycle[session_id % 8].state;
}

tm_status_t tm_lifecycle_record_error(uint32_t session_id)
{
    g_lifecycle[session_id % 8].error_count++;
    return TM_OK;
}

tm_status_t tm_lifecycle_get_stats(uint32_t session_id,
                                      uint32_t *transitions,
                                      uint32_t *errors)
{
    uint32_t idx = session_id % 8;
    if (transitions) *transitions = g_lifecycle[idx].transition_count;
    if (errors) *errors = g_lifecycle[idx].error_count;
    return TM_OK;
}
