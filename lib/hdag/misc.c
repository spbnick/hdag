/*
 * Hash DAG miscellaneous definitions.
 */

#include <hdag/misc.h>
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
