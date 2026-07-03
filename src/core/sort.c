#include "tm/core.h"
#include <string.h>
#include <stdlib.h>

void tm_sort_u32(uint32_t *arr, size_t count)
{
    if (!arr || count < 2) return;
    for (size_t i = 1; i < count; i++) {
        uint32_t key = arr[i];
        int j = (int)i - 1;
        while (j >= 0 && arr[j] > key) {
            arr[j + 1] = arr[j];
            j--;
        }
        arr[j + 1] = key;
    }
}

void tm_sort_u64(uint64_t *arr, size_t count)
{
    if (!arr || count < 2) return;
    for (size_t i = 1; i < count; i++) {
        uint64_t key = arr[i];
        int j = (int)i - 1;
        while (j >= 0 && arr[j] > key) {
            arr[j + 1] = arr[j];
            j--;
        }
        arr[j + 1] = key;
    }
}

size_t tm_unique_u32(uint32_t *arr, size_t count)
{
    if (!arr || count < 2) return count;
    tm_sort_u32(arr, count);
    size_t write = 1;
    for (size_t i = 1; i < count; i++) {
        if (arr[i] != arr[i - 1])
            arr[write++] = arr[i];
    }
    return write;
}

int tm_binary_search_u32(const uint32_t *arr, size_t count, uint32_t target)
{
    if (!arr || count == 0) return -1;
    int lo = 0, hi = (int)count - 1;
    while (lo <= hi) {
        int mid = (lo + hi) / 2;
        if (arr[mid] == target) return mid;
        if (arr[mid] < target) lo = mid + 1;
        else hi = mid - 1;
    }
    return -1;
}

int tm_binary_search_u64(const uint64_t *arr, size_t count, uint64_t target)
{
    if (!arr || count == 0) return -1;
    int lo = 0, hi = (int)count - 1;
    while (lo <= hi) {
        int mid = (lo + hi) / 2;
        if (arr[mid] == target) return mid;
        if (arr[mid] < target) lo = mid + 1;
        else hi = mid - 1;
    }
    return -1;
}

size_t tm_merge_sorted_u32(const uint32_t *a, size_t alen,
                              const uint32_t *b, size_t blen,
                              uint32_t *dst, size_t dst_cap)
{
    size_t ai = 0, bi = 0, di = 0;
    while (ai < alen && bi < blen && di < dst_cap) {
        if (a[ai] <= b[bi]) dst[di++] = a[ai++];
        else dst[di++] = b[bi++];
    }
    while (ai < alen && di < dst_cap) dst[di++] = a[ai++];
    while (bi < blen && di < dst_cap) dst[di++] = b[bi++];
    return di;
}

void tm_reverse_u32(uint32_t *arr, size_t count)
{
    if (!arr) return;
    for (size_t i = 0; i < count / 2; i++) {
        uint32_t tmp = arr[i];
        arr[i] = arr[count - 1 - i];
        arr[count - 1 - i] = tmp;
    }
}
