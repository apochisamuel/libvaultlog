#include "tm/core.h"
#include "tm/error.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

typedef enum {
    TM_LOG_DEBUG   = 0,
    TM_LOG_INFO    = 1,
    TM_LOG_WARN    = 2,
    TM_LOG_ERROR   = 3,
    TM_LOG_FATAL   = 4
} tm_log_level_t;

static tm_log_level_t g_log_level = TM_LOG_WARN;
static uint32_t g_log_count = 0;
static char g_log_buffer[4096];
static size_t g_log_pos = 0;

void tm_log_set_level(tm_log_level_t level) { g_log_level = level; }
tm_log_level_t tm_log_get_level(void) { return g_log_level; }

void tm_log_write(tm_log_level_t level, const char *file, int line,
                     const char *fmt, ...)
{
    if (level < g_log_level) return;
    const char *level_names[] = {"DEBUG", "INFO", "WARN", "ERROR", "FATAL"};
    const char *ln = (level < 5) ? level_names[level] : "UNKNOWN";
    
    size_t remaining = sizeof(g_log_buffer) - g_log_pos;
    if (remaining < 128) { g_log_pos = 0; remaining = sizeof(g_log_buffer); }
    
    int written = snprintf(g_log_buffer + g_log_pos, remaining,
                           "[%s] %s:%d: ", ln, file, line);
    if (written > 0) g_log_pos += (size_t)written;
    
    va_list args;
    va_start(args, fmt);
    remaining = sizeof(g_log_buffer) - g_log_pos;
    written = vsnprintf(g_log_buffer + g_log_pos, remaining, fmt, args);
    va_end(args);
    if (written > 0) g_log_pos += (size_t)written;
    
    if (g_log_pos < sizeof(g_log_buffer) - 1)
        g_log_buffer[g_log_pos++] = '\n';
    
    g_log_count++;
}

uint32_t tm_log_get_count(void) { return g_log_count; }

void tm_log_reset(void)
{
    g_log_pos = 0;
    g_log_count = 0;
    memset(g_log_buffer, 0, sizeof(g_log_buffer));
}

const char *tm_log_get_buffer(void) { return g_log_buffer; }

size_t tm_log_get_size(void) { return g_log_pos; }

void tm_assert_fail(const char *expr, const char *file, int line)
{
    fprintf(stderr, "ASSERTION FAILED: %s at %s:%d\n", expr, file, line);
}

const char *tm_error_name(tm_status_t status)
{
    switch (status) {
    case TM_OK: return "TM_OK";
    case TM_ERR_ALLOC: return "TM_ERR_ALLOC";
    case TM_ERR_BAD_MAGIC: return "TM_ERR_BAD_MAGIC";
    case TM_ERR_BAD_CRC: return "TM_ERR_BAD_CRC";
    case TM_ERR_TRUNCATED: return "TM_ERR_TRUNCATED";
    case TM_ERR_BAD_STATE: return "TM_ERR_BAD_STATE";
    case TM_ERR_NOT_FOUND: return "TM_ERR_NOT_FOUND";
    case TM_ERR_OVERFLOW: return "TM_ERR_OVERFLOW";
    case TM_ERR_CORRUPT: return "TM_ERR_CORRUPT";
    case TM_ERR_WAL_GAP: return "TM_ERR_WAL_GAP";
    default: return "TM_ERR_UNKNOWN";
    }
}
