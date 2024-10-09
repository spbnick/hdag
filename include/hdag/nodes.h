/*
 * Hash DAG (lexicographically) sorted node array
 */

#ifndef _HDAG_NODES_H
#define _HDAG_NODES_H

#include <hdag/node.h>

/**
 * Find a node in a slice of node array, by hash.
 *
 * @param nodes     The node array to search.
 *                  Must be sorted lexicographically by node hashes.
 * @param start_idx The start index of the nodes array slice to search in.
 *                  Must be less than INT32_MAX.
 * @param end_idx   The end index of the nodes array slice to search in
 *                  (pointing right after the last node of the slice).
 *                  Must be greater than or equal to start_idx.
 *                  Must be less than INT32_MAX.
 * @param hash_len  The length of the node hash.
 * @param hash_ptr  Pointer to the hash to find in the array.
 *
 * @return The array index of the found node (< INT32_MAX),
 *         or INT32_MAX, if not found.
 */
extern uint32_t hdag_nodes_slice_find(const struct hdag_node *nodes,
                                      size_t start_idx, size_t end_idx,
                                      uint16_t hash_len,
                                      const uint8_t *hash_ptr);


/**
 * Find a node in a node array, by hash.
 *
 * @param nodes     The node array to search.
 *                  Must be sorted lexicographically by node hashes.
 * @param nodes_num Number of nodes in the array. Must be less than INT32_MAX.
 * @param hash_len  The length of the node hash.
 * @param hash_ptr  Pointer to the hash to find in the array.
 *
 * @return The array index of the found node (< INT32_MAX),
 *         or >= INT32_MAX, if not found.
 */
static inline uint32_t
hdag_nodes_find(const struct hdag_node *nodes, size_t nodes_num,
                uint16_t hash_len, const uint8_t *hash_ptr)
{
    return hdag_nodes_slice_find(nodes, 0, nodes_num, hash_len, hash_ptr);
}

/**
 * Get a const node from a const node array by its index.
 *
 * @param nodes     The node array to get the node from.
 * @param hash_len  The length of the hash of each node in the array.
 * @param idx       The index of the node to get (up to the caller to
 *                  validate). Must be less than INT32_MAX.
 *
 * @return The node at the specified index.
 */
static inline const struct hdag_node *
hdag_nodes_get_const(const struct hdag_node *nodes,
                     uint16_t hash_len,
                     uint32_t idx)
{
    assert(nodes != NULL);
    assert(hdag_hash_len_is_valid(hash_len));
    assert(idx < INT32_MAX);
    return hdag_node_off_const(nodes, hash_len, idx);
}

#endif /* _HDAG_NODES_H */
