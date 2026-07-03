#include "tm/core.h"
#include <string.h>

typedef struct {
    uint8_t  *buffer;
    size_t    capacity;
    size_t    head;
    size_t    tail;
    size_t    count;
} tm_ring_buffer_t;

tm_status_t tm_ring_init(tm_ring_buffer_t *ring, size_t capacity)
{
    if (!ring || capacity == 0) return TM_ERR_BAD_STATE;
    ring->buffer = calloc(1, capacity);
    if (!ring->buffer) return TM_ERR_ALLOC;
    ring->capacity = capacity;
    ring->head = 0;
    ring->tail = 0;
    ring->count = 0;
    return TM_OK;
}

void tm_ring_destroy(tm_ring_buffer_t *ring)
{
    if (ring) { free(ring->buffer); memset(ring, 0, sizeof(*ring)); }
}

tm_status_t tm_ring_write(tm_ring_buffer_t *ring, const uint8_t *data, size_t len)
{
    if (!ring || !data || ring->count + len > ring->capacity)
        return TM_ERR_OVERFLOW;
    for (size_t i = 0; i < len; i++) {
        ring->buffer[ring->head] = data[i];
        ring->head = (ring->head + 1) % ring->capacity;
    }
    ring->count += len;
    return TM_OK;
}

tm_status_t tm_ring_read(tm_ring_buffer_t *ring, uint8_t *dst, size_t len)
{
    if (!ring || !dst || len > ring->count) return TM_ERR_TRUNCATED;
    for (size_t i = 0; i < len; i++) {
        dst[i] = ring->buffer[ring->tail];
        ring->tail = (ring->tail + 1) % ring->capacity;
    }
    ring->count -= len;
    return TM_OK;
}

size_t tm_ring_available(const tm_ring_buffer_t *ring)
{
    return ring ? ring->count : 0;
}

size_t tm_ring_space(const tm_ring_buffer_t *ring)
{
    return ring ? ring->capacity - ring->count : 0;
}

tm_status_t tm_ring_peek(const tm_ring_buffer_t *ring, uint8_t *dst, size_t len)
{
    if (!ring || !dst || len > ring->count) return TM_ERR_TRUNCATED;
    size_t pos = ring->tail;
    for (size_t i = 0; i < len; i++) {
        dst[i] = ring->buffer[pos];
        pos = (pos + 1) % ring->capacity;
    }
    return TM_OK;
}

void tm_ring_reset(tm_ring_buffer_t *ring)
{
    if (ring) { ring->head = 0; ring->tail = 0; ring->count = 0; }
}
