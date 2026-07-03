#include "tm/crc.h"
#include <stdbool.h>

static uint32_t crc_table[256];
static bool table_init = false;

static void init_table(void)
{
    for (uint32_t i = 0; i < 256; i++) {
        uint32_t c = i;
        for (int j = 0; j < 8; j++)
            c = (c >> 1) ^ ((c & 1) ? 0x82F63B78 : 0);
        crc_table[i] = c;
    }
    table_init = true;
}

uint32_t tm_crc32c(uint32_t crc, const uint8_t *data, size_t len)
{
    if (!table_init) init_table();
    crc = ~crc;
    for (size_t i = 0; i < len; i++)
        crc = crc_table[(crc ^ data[i]) & 0xFF] ^ (crc >> 8);
    return ~crc;
}
