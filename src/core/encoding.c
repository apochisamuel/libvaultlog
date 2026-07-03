#include "tm/core.h"
#include <string.h>

typedef struct {
    uint8_t  *buffer;
    size_t    capacity;
    size_t    position;
} tm_encoder_t;

tm_status_t tm_encoder_init(tm_encoder_t *enc, uint8_t *buffer, size_t capacity)
{
    if (!enc || !buffer) return TM_ERR_BAD_STATE;
    enc->buffer = buffer;
    enc->capacity = capacity;
    enc->position = 0;
    return TM_OK;
}

tm_status_t tm_encoder_write_u8(tm_encoder_t *enc, uint8_t value)
{
    if (!enc || enc->position >= enc->capacity) return TM_ERR_OVERFLOW;
    enc->buffer[enc->position++] = value;
    return TM_OK;
}

tm_status_t tm_encoder_write_u16(tm_encoder_t *enc, uint16_t value)
{
    if (!enc || enc->position + 2 > enc->capacity) return TM_ERR_OVERFLOW;
    enc->buffer[enc->position++] = (uint8_t)(value & 0xFF);
    enc->buffer[enc->position++] = (uint8_t)((value >> 8) & 0xFF);
    return TM_OK;
}

tm_status_t tm_encoder_write_u32(tm_encoder_t *enc, uint32_t value)
{
    if (!enc || enc->position + 4 > enc->capacity) return TM_ERR_OVERFLOW;
    enc->buffer[enc->position++] = (uint8_t)(value & 0xFF);
    enc->buffer[enc->position++] = (uint8_t)((value >> 8) & 0xFF);
    enc->buffer[enc->position++] = (uint8_t)((value >> 16) & 0xFF);
    enc->buffer[enc->position++] = (uint8_t)((value >> 24) & 0xFF);
    return TM_OK;
}

tm_status_t tm_encoder_write_u64(tm_encoder_t *enc, uint64_t value)
{
    tm_status_t rc = tm_encoder_write_u32(enc, (uint32_t)(value & 0xFFFFFFFF));
    if (rc != TM_OK) return rc;
    return tm_encoder_write_u32(enc, (uint32_t)((value >> 32) & 0xFFFFFFFF));
}

tm_status_t tm_encoder_write_bytes(tm_encoder_t *enc, const uint8_t *data, size_t len)
{
    if (!enc || !data || enc->position + len > enc->capacity) return TM_ERR_OVERFLOW;
    memcpy(enc->buffer + enc->position, data, len);
    enc->position += len;
    return TM_OK;
}

size_t tm_encoder_position(const tm_encoder_t *enc)
{
    return enc ? enc->position : 0;
}

typedef struct {
    const uint8_t *buffer;
    size_t         length;
    size_t         position;
} tm_decoder_t;

tm_status_t tm_decoder_init(tm_decoder_t *dec, const uint8_t *buffer, size_t length)
{
    if (!dec || !buffer) return TM_ERR_BAD_STATE;
    dec->buffer = buffer;
    dec->length = length;
    dec->position = 0;
    return TM_OK;
}

tm_status_t tm_decoder_read_u8(tm_decoder_t *dec, uint8_t *value)
{
    if (!dec || !value || dec->position >= dec->length) return TM_ERR_TRUNCATED;
    *value = dec->buffer[dec->position++];
    return TM_OK;
}

tm_status_t tm_decoder_read_u16(tm_decoder_t *dec, uint16_t *value)
{
    if (!dec || !value || dec->position + 2 > dec->length) return TM_ERR_TRUNCATED;
    *value = (uint16_t)dec->buffer[dec->position] |
             ((uint16_t)dec->buffer[dec->position + 1] << 8);
    dec->position += 2;
    return TM_OK;
}

tm_status_t tm_decoder_read_u32(tm_decoder_t *dec, uint32_t *value)
{
    if (!dec || !value || dec->position + 4 > dec->length) return TM_ERR_TRUNCATED;
    *value = (uint32_t)dec->buffer[dec->position] |
             ((uint32_t)dec->buffer[dec->position + 1] << 8) |
             ((uint32_t)dec->buffer[dec->position + 2] << 16) |
             ((uint32_t)dec->buffer[dec->position + 3] << 24);
    dec->position += 4;
    return TM_OK;
}

tm_status_t tm_decoder_read_bytes(tm_decoder_t *dec, uint8_t *dst, size_t len)
{
    if (!dec || !dst || dec->position + len > dec->length) return TM_ERR_TRUNCATED;
    memcpy(dst, dec->buffer + dec->position, len);
    dec->position += len;
    return TM_OK;
}

size_t tm_decoder_remaining(const tm_decoder_t *dec)
{
    if (!dec || dec->position >= dec->length) return 0;
    return dec->length - dec->position;
}
