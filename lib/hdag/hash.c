/**
 * Hash DAG node ID hash
 */

#include <hdag/hash.h>
#include <string.h>

int
hdag_hash_cmp(const void *a, const void *b, void *len)
{
    assert(a != NULL);
    assert(b != NULL);
    assert((uintptr_t)len <= UINT16_MAX);
    assert(hdag_hash_len_is_valid((uint16_t)(uintptr_t)len));
    return memcmp(a, b, (uintptr_t)len);
}
