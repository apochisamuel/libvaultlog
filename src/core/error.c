#include <stdbool.h>
#include "tm/error.h"
#include <string.h>

const char *tm_status_string(tm_status_t status)
{
    switch (status) {
    case TM_OK:               return "ok";
    case TM_ERR_ALLOC:        return "allocation failed";
    case TM_ERR_BAD_MAGIC:    return "bad magic number";
    case TM_ERR_BAD_VERSION:  return "unsupported version";
    case TM_ERR_BAD_CRC:      return "CRC mismatch";
    case TM_ERR_TRUNCATED:    return "truncated data";
    case TM_ERR_BAD_STATE:    return "invalid state";
    case TM_ERR_NOT_FOUND:    return "not found";
    case TM_ERR_OVERFLOW:     return "overflow";
    case TM_ERR_BAD_FRAME:    return "invalid frame";
    case TM_ERR_BAD_SEGMENT:  return "invalid segment";
    case TM_ERR_NOT_ACTIVE:   return "session not active";
    case TM_ERR_BAD_SEQUENCE: return "sequence mismatch";
    case TM_ERR_BAD_LEASE:    return "invalid lease";
    case TM_ERR_BAD_FRAME_TYPE: return "unknown frame type";
    case TM_ERR_BAD_CURSOR:   return "invalid cursor";
    case TM_ERR_QUOTA:        return "quota exceeded";
    case TM_ERR_CORRUPT:      return "data corruption";
    case TM_ERR_WAL_GAP:      return "WAL replay gap";
    case TM_ERR_BAD_EPOCH:    return "invalid epoch";
    case TM_ERR_BAD_CHANNEL:  return "invalid channel";
    default:                  return "unknown error";
    }
}

bool tm_status_is_error(tm_status_t status)
{
    return status < 0;
}

bool tm_status_is_fatal(tm_status_t status)
{
    switch (status) {
    case TM_ERR_ALLOC:
    case TM_ERR_CORRUPT:
    case TM_ERR_WAL_GAP:
        return true;
    default:
        return false;
    }
}
