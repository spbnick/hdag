/**
 * Hash DAG miscellaneous definitions.
 */

#include <hdag/misc.h>
#include <string.h>
#include <assert.h>

char *
hdag_bytes_to_hex(char *hex_ptr, const void *bytes_ptr, size_t bytes_num)
{
    const char digits[16] = "0123456789abcdef";
    const uint8_t *i;
    char *o;

    assert(hex_ptr != NULL);
    assert(bytes_ptr != NULL || bytes_num == 0);
    assert(bytes_num < SIZE_MAX / 2);

    for (i = bytes_ptr, o = hex_ptr; bytes_num > 0; i++, bytes_num--) {
        uint8_t b = *i;
        *o++ = digits[b >> 4];
        *o++ = digits[b & 0xf];
    }
    *o = '\0';
    return hex_ptr;
}

int
hdag_size_t_cmp(const void *a, const void *b)
{
    size_t val_a = *(const size_t *)a;
    size_t val_b = *(const size_t *)b;
    return val_a == val_b ? 0 : (val_a > val_b ? 1 : -1);
}

int
hdag_size_t_rcmp(const void *a, const void *b)
{
    size_t val_a = *(const size_t *)a;
    size_t val_b = *(const size_t *)b;
    return val_a == val_b ? 0 : (val_a < val_b ? 1 : -1);
}

int
hdag_cmp_mem(const void *first, const void *second, void *data)
{
    assert((uintptr_t)data == 0 ||
           (first != NULL && second != NULL));
    assert((uintptr_t)data <= SIZE_MAX);
    return memcmp(first, second, (uintptr_t)data);
}
