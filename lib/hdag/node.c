/**
 * Hash DAG node
 */

#include <hdag/node.h>
#include <string.h>

int
hdag_node_cmp(const void *a, const void *b, void *hash_len)
{
    const struct hdag_node *node_a = a;
    const struct hdag_node *node_b = b;
    assert((uintptr_t)hash_len <= UINT16_MAX);
    assert(hdag_hash_len_is_valid((uint16_t)(uintptr_t)hash_len));
    return hdag_hash_cmp(node_a->hash, node_b->hash, hash_len);
}
