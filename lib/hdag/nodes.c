/**
 * Hash DAG (lexicographically) sorted node array
 */

#include <hdag/nodes.h>
#include <string.h>

uint32_t
hdag_nodes_slice_find(const struct hdag_node *nodes,
                      size_t start_idx, size_t end_idx,
                      uint16_t hash_len, const uint8_t *hash_ptr)
{
    assert(start_idx < INT32_MAX);
    assert(end_idx <= INT32_MAX);
    assert(start_idx <= end_idx);
    assert(nodes != NULL || end_idx == 0);
    assert(hdag_hash_len_is_valid(hash_len));
    assert(hash_ptr != NULL);

    if (start_idx >= end_idx) {
        /* Not found */
        return INT32_MAX;
    }

    int relation;
    size_t middle_idx = (start_idx + end_idx) >> 1;
    const struct hdag_node *middle_node = hdag_node_off_const(
        nodes, hash_len, middle_idx
    );
    relation = memcmp(hash_ptr, middle_node->hash, hash_len);
    if (relation == 0) {
        return middle_idx;
    } if (relation > 0) {
        return hdag_nodes_slice_find(nodes, middle_idx + 1, end_idx,
                                     hash_len, hash_ptr);
    } else {
        return hdag_nodes_slice_find(nodes, start_idx, middle_idx,
                                     hash_len, hash_ptr);
    }
}
