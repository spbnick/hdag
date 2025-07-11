/**
 * Hash DAG node
 */

#ifndef _HDAG_NODE_H
#define _HDAG_NODE_H

#include <hdag/targets.h>
#include <hdag/hash.h>
#include <stdint.h>


/** A node */
struct hdag_node {
    /** The ID of the graph component the node belongs to */
    uint32_t component;
    /** The node's generation number */
    uint32_t generation;
    /** The targets of outgoing edges */
    struct hdag_targets targets;
    /** The hash with length defined elsewhere */
    uint8_t hash[];
};

/**
 * Check if a node is valid.
 *
 * @param node  The node to check.
 *
 * @return True if the node is valid, false otherwise.
 */
static inline bool
hdag_node_is_valid(const struct hdag_node *node)
{
    return node != NULL && ((uintptr_t)node & 3) == 0 &&
        hdag_targets_are_valid(&node->targets);
}

/**
 * Calculate the size of a node based on the ID hash length.
 *
 * @param hash_len  The length of the node ID hash, bytes. Must be valid
 *                  according to hdag_hash_len_is_valid(), or zero.
 *
 * @return The size of the node.
 */
static inline size_t
hdag_node_size(uint16_t hash_len)
{
    size_t size;
    assert(hash_len == 0 || hdag_hash_len_is_valid(hash_len));
    size = sizeof(struct hdag_node) + hash_len;
    assert(size != 0 && (size & 3) == 0);
    return size;
}

/**
 * Calculate the node hash length based on a node size.
 *
 * @param size  The size of the node to get the hash length for.
 *
 * @return The length of the node hash, bytes, or zero, if the node is
 *         "hashless".
 */
static inline size_t
hdag_node_hash_len(size_t size)
{
    assert(size >= sizeof(struct hdag_node));
    size_t hash_len = size - sizeof(struct hdag_node);
    assert(hash_len == 0 || hdag_hash_len_is_valid(hash_len));
    return hash_len;
}

/**
 * Calculate a pointer to a node with the specified offset from the given one.
 *
 * @param node      The node to start the offset from.
 *                  Must be non-NULL and divisible by four.
 * @param hash_len  The length of the node's hash.
 * @param off       The number of nodes to offset by.
 *
 * @return The pointer to the offset node.
 */
static inline struct hdag_node *
hdag_node_off(struct hdag_node *node, uint16_t hash_len, ssize_t off)
{
    assert(((uintptr_t)node & 3) == 0);
    assert(hash_len == 0 || hdag_hash_len_is_valid(hash_len));
    return (struct hdag_node *)((uint8_t *)node + off * hdag_node_size(hash_len));
}

/**
 * Calculate a pointer to a const node with the specified offset from the
 * given one.
 *
 * @param node      The node to start the offset from.
 *                  Must be non-NULL and divisible by four.
 * @param hash_len  The length of the node's hash.
 * @param off       The number of nodes to offset by.
 *
 * @return The pointer to the offset node.
 */
static inline const struct hdag_node *
hdag_node_off_const(const struct hdag_node *node,
                    uint16_t hash_len,
                    ssize_t off)
{
    assert(((uintptr_t)node & 3) == 0);
    assert(hash_len == 0 || hdag_hash_len_is_valid(hash_len));
    return (const struct hdag_node *)(
        (const uint8_t *)node + off * hdag_node_size(hash_len)
    );
}

/**
 * Compare two nodes by hash lexicographically.
 *
 * @param a         The first node to compare.
 * @param b         The second node to compare.
 * @param hash_len  The node hash length (uintptr_t).
 *
 * @return -1, if a < b
 *          0, if a == b
 *          1, if a > b
 */
extern int hdag_node_cmp(const void *a, const void *b, void *hash_len);

/**
 * Fill a node's hash with specified 32-bit unsigned integer.
 *
 * @param node      The node to have the hash filled.
 * @param hash_len  The length of the node's hash.
 * @param fill      The value to fill the hash with.
 */
static inline void
hdag_node_hash_fill(struct hdag_node *node, uint16_t hash_len, uint32_t fill)
{
    assert(hdag_node_is_valid(node));
    assert(hdag_hash_len_is_valid(hash_len));
    hdag_hash_fill(node->hash, hash_len, fill);
}

/**
 * Check if a node's hash is filled with specified 32-bit unsigned integer.
 *
 * @param node      The node to have the hash checked.
 * @param hash_len  The length of the node's hash.
 * @param fill      The fill value to check against
 *
 * @return True if the node's hash is filled with the specified value,
 *         false otherwise.
 */
static inline bool
hdag_node_hash_is_filled(struct hdag_node *node,
                         uint16_t hash_len, uint32_t fill)
{
    assert(hdag_node_is_valid(node));
    assert(hdag_hash_len_is_valid(hash_len));
    return hdag_hash_is_filled(node->hash, hash_len, fill);
}

/**
 * Get first indirect target index from a node.
 *
 * @param node  The node to get the index from. Must have indirect targets.
 *
 * @return The first indirect index.
 *         Interpretation is up to the node's container.
 */
static inline size_t
hdag_node_get_first_ind_idx(const struct hdag_node *node)
{
    assert(hdag_node_is_valid(node));
    assert(hdag_targets_are_indirect(&node->targets));
    return hdag_target_to_ind_idx(node->targets.first);
}

/**
 * Get last indirect target index from a node.
 *
 * @param node  The node to get the index from. Must have indirect targets.
 *
 * @return The last indirect index.
 *         Interpretation is up to the node's container.
 */
static inline size_t
hdag_node_get_last_ind_idx(const struct hdag_node *node)
{
    assert(hdag_node_is_valid(node));
    assert(hdag_targets_are_indirect(&node->targets));
    return hdag_target_to_ind_idx(node->targets.last);
}

/**
 * Return the number of known targets (direct or indirect) of a node.
 *
 * @param targets   The node to have targets counted for.
 *
 * @return The number of the node's known targets.
 */
static inline uint32_t
hdag_node_targets_count(const struct hdag_node *node)
{
    assert(hdag_node_is_valid(node));
    return hdag_targets_count(&node->targets);
}

/**
 * Check if a node is known, or is simply a record of node's existence.
 *
 * @param node  The node to check.
 *
 * @return True if the complete node's information is known, false if not.
 */
static inline bool
hdag_node_is_known(const struct hdag_node *node)
{
    assert(hdag_node_is_valid(node));
    return hdag_targets_are_known(&node->targets);
}

#endif /* _HDAG_NODE_H */
