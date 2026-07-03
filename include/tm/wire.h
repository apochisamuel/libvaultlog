#ifndef TM_WIRE_H
#define TM_WIRE_H
#include "tm/types.h"
#include "tm/error.h"

void        tm_wire_write_u16(uint8_t *dst, uint16_t val);
void        tm_wire_write_u32(uint8_t *dst, uint32_t val);
uint16_t    tm_wire_read_u16(const uint8_t *src);
uint32_t    tm_wire_read_u32(const uint8_t *src);

tm_status_t tm_wire_decode_session_header(const uint8_t *data, size_t len,
                                            tm_session_header_t *out);
tm_status_t tm_wire_decode_frame(const uint8_t *data, size_t len,
                                   tm_frame_t *out, size_t *consumed);
#endif
