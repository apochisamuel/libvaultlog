#include "tm/session.h"
int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    if (size < TM_SESSION_HEADER_WIRE_SIZE + TM_FRAME_HEADER_WIRE_SIZE)
        return 0;
    uint32_t sid = 1;
    tm_session_t *sess = tm_session_create(sid);
    if (!sess) return 0;
    tm_admin_dispatch(sess, data, size);
    tm_session_destroy(sess);
    return 0;
}
