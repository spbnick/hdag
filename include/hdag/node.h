/*
 * Hash DAG node
 */

#ifndef _HDAG_NODE_H
#define _HDAG_NODE_H

#include <hdag/targets.h>
#include <hdag/hash.h>
#include <stdint.h>


/** A node */
struct hdag_node {
    /** The ID of the component the node belongs to */
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
 * @param hash_len  The length of the node ID hash, bytes.
 *                  Must be divisible by 4.
 */
static inline size_t
hdag_node_size(uint16_t hash_len)
{
    size_t size;
    assert(hdag_hash_len_is_valid(hash_len));
    size = sizeof(struct hdag_node) + hash_len;
    assert(size != 0 && (size & 3) == 0);
    return size;
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
    assert(hdag_hash_len_is_valid(hash_len));
    return (struct hdag_node *)((uint8_t *)node + off * hdag_node_size(hash_len));
}

/**
 * Compare two nodes by hash lexicographically.
 *
 * @param a         The first node to compare.
 * @param b         The second node to compare.
 * @param phash_len Location containing the node hash length (uint16_t).
 *
 * @return -1, if a < b
 *          0, if a == b
 *          1, if a > b
 */
extern int hdag_node_cmp(const void *a, const void *b, void *phash_len);

#endif /* _HDAG_NODE_H */
