/*
 * Hash DAG bundle - a file in the making
 */

#ifndef _HDAG_BUNDLE_H
#define _HDAG_BUNDLE_H

#include <hdag/edge.h>
#include <hdag/node.h>
#include <hdag/node_seq.h>

/** A bundle */
struct hdag_bundle {
    /** The length of the node hash */
    uint16_t            hash_len;

    /** Number of collected nodes */
    size_t              nodes_num;
    /** Number of allocated node slots */
    size_t              nodes_allocated;
    /**
     * Nodes.
     * Before compacting, their targets are either unknown, or are indirect
     * indices pointing into the target_hashes array. After compacting their
     * targets are either unknown, direct indices pointing into the same
     * array, or indirect indices pointing into the extra_edges array.
     */
    struct hdag_node   *nodes;

    /** Number of collected target hashes */
    size_t              target_hashes_num;
    /** Number of allocated target hash slots */
    size_t              target_hashes_allocated;
    /** Target hashes */
    uint8_t            *target_hashes;

    /** Number of collected extra edges */
    size_t              extra_edges_num;
    /** Number of allocated extra edge slots */
    size_t              extra_edges_allocated;
    /** The array of extra edges, which didn't fit into nodes themselves */
    struct hdag_edge   *extra_edges;
};

/** An empty bundle */
#define HDAG_BUNDLE_EMPTY (struct hdag_bundle){0, }

/**
 * Cleanup a bundle, freeing associated resources (but not the bundle
 * structure itself).
 *
 * @param bundle    The bundle to cleanup.
 */
extern void hdag_bundle_cleanup(struct hdag_bundle *bundle);

/**
 * Load a node sequence (adjacency list) into a bundle.
 *
 * @param pbundle   Location for the bundle.
 *                  Not modified in case of error.
 *                  Can be NULL to have the bundle discarded.
 * @param hash_len  The length of the node hashes.
 * @param node_seq  The sequence of nodes (and optionally their
 *                  targets) to load.
 *
 * @return True if the data was loaded and processed successfully,
 *         false if not. The errno is set in case of failure.
 */
extern bool hdag_bundle_create(struct hdag_bundle *pbundle,
                               uint16_t hash_len,
                               struct hdag_node_seq node_seq);

#endif /* _HDAG_BUNDLE_H */
