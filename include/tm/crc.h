#ifndef TM_CRC_H
#define TM_CRC_H
#include <stdint.h>
#include <stddef.h>
uint32_t tm_crc32c(uint32_t crc, const uint8_t *data, size_t len);
#endif
