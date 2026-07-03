#ifndef TM_ERROR_H
#define TM_ERROR_H

typedef int tm_status_t;

#define TM_OK                0
#define TM_ERR_ALLOC        -1
#define TM_ERR_BAD_MAGIC    -2
#define TM_ERR_BAD_VERSION  -3
#define TM_ERR_BAD_CRC      -4
#define TM_ERR_TRUNCATED    -5
#define TM_ERR_BAD_STATE    -6
#define TM_ERR_NOT_FOUND    -7
#define TM_ERR_OVERFLOW     -8
#define TM_ERR_BAD_FRAME    -9
#define TM_ERR_BAD_SEGMENT  -10
#define TM_ERR_NOT_ACTIVE   -11
#define TM_ERR_BAD_SEQUENCE -12
#define TM_ERR_BAD_LEASE    -13
#define TM_ERR_BAD_FRAME_TYPE -14
#define TM_ERR_BAD_CURSOR   -15
#define TM_ERR_QUOTA        -16
#define TM_ERR_CORRUPT      -17
#define TM_ERR_WAL_GAP      -18
#define TM_ERR_BAD_EPOCH    -19
#define TM_ERR_BAD_CHANNEL  -20

#endif /* TM_ERROR_H */
