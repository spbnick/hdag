/**
 * Hash DAG node ID hash
 */

#include <hdag/hash.h>
#include <string.h>

int
hdag_hash_cmp(const void *a, const void *b, void *plen)
{
    return memcmp(a, b, *(uint16_t *)plen);
}
