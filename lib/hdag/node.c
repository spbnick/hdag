/**
 * Hash DAG node
 */

#include <hdag/node.h>
#include <string.h>

int
hdag_node_cmp(const void *a, const void *b, void *phash_len)
{
    const struct hdag_node *node_a = a;
    const struct hdag_node *node_b = b;
    return hdag_hash_cmp(node_a->hash, node_b->hash, phash_len);
}
