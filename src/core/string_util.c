#include "tm/core.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

size_t tm_format_size(uint64_t bytes, char *dst, size_t cap)
{
    const char *units[] = {"B", "KB", "MB", "GB", "TB"};
    int unit = 0;
    double size = (double)bytes;
    while (size >= 1024.0 && unit < 4) { size /= 1024.0; unit++; }
    return (size_t)snprintf(dst, cap, "%.2f %s", size, units[unit]);
}

size_t tm_format_duration(uint64_t seconds, char *dst, size_t cap)
{
    uint64_t days = seconds / 86400;
    uint64_t hours = (seconds % 86400) / 3600;
    uint64_t mins = (seconds % 3600) / 60;
    uint64_t secs = seconds % 60;
    if (days > 0)
        return (size_t)snprintf(dst, cap, "%lud %luh %lum %lus",
                                (unsigned long)days, (unsigned long)hours,
                                (unsigned long)mins, (unsigned long)secs);
    if (hours > 0)
        return (size_t)snprintf(dst, cap, "%luh %lum %lus",
                                (unsigned long)hours, (unsigned long)mins,
                                (unsigned long)secs);
    if (mins > 0)
        return (size_t)snprintf(dst, cap, "%lum %lus",
                                (unsigned long)mins, (unsigned long)secs);
    return (size_t)snprintf(dst, cap, "%lus", (unsigned long)secs);
}

size_t tm_format_count(uint64_t count, char *dst, size_t cap)
{
    if (count >= 1000000000ULL)
        return (size_t)snprintf(dst, cap, "%.2fB", (double)count / 1e9);
    if (count >= 1000000ULL)
        return (size_t)snprintf(dst, cap, "%.2fM", (double)count / 1e6);
    if (count >= 1000ULL)
        return (size_t)snprintf(dst, cap, "%.2fK", (double)count / 1e3);
    return (size_t)snprintf(dst, cap, "%lu", (unsigned long)count);
}

int tm_strcmp_nocase(const char *a, const char *b)
{
    while (*a && *b) {
        char ca = (*a >= 'A' && *a <= 'Z') ? (*a + 32) : *a;
        char cb = (*b >= 'A' && *b <= 'Z') ? (*b + 32) : *b;
        if (ca != cb) return ca - cb;
        a++; b++;
    }
    return *a - *b;
}

size_t tm_strlcpy(char *dst, const char *src, size_t cap)
{
    size_t len = strlen(src);
    if (cap > 0) {
        size_t copy = (len < cap - 1) ? len : cap - 1;
        memcpy(dst, src, copy);
        dst[copy] = 0;
    }
    return len;
}

size_t tm_strlcat(char *dst, const char *src, size_t cap)
{
    size_t dlen = strlen(dst);
    size_t slen = strlen(src);
    if (dlen >= cap) return dlen + slen;
    size_t remaining = cap - dlen - 1;
    size_t copy = (slen < remaining) ? slen : remaining;
    memcpy(dst + dlen, src, copy);
    dst[dlen + copy] = 0;
    return dlen + slen;
}
