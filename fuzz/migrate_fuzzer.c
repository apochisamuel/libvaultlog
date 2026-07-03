#include "tm/session.h"
int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    if (size < (TM_SESSION_HEADER_WIRE_SIZE + TM_FRAME_HEADER_WIRE_SIZE) * 2)
        return 0;
    size_t split = size / 2;
    tm_session_t *src = tm_session_create(1);
    tm_session_t *dst = tm_session_create(2);
    if (!src || !dst) { tm_session_destroy(src); tm_session_destroy(dst); return 0; }
    tm_session_ingest(src, data, split);
    tm_session_ingest(dst, data + split, size - split);
    tm_session_destroy(dst);
    tm_session_destroy(src);
    return 0;
}
