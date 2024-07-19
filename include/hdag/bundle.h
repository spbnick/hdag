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

/** An empty bundle with specified hash length */
#define HDAG_BUNDLE_EMPTY(_hash_len) \
    (struct hdag_bundle){.hash_len = hdag_hash_len_validate(_hash_len), 0}

/**
 * Check if a bundle is valid.
 *
 * @param bundle    The bundle to check.
 *
 * @return True if the bundle is valid, false otherwise.
 */
static inline bool
hdag_bundle_is_valid(const struct hdag_bundle *bundle)
{
    return bundle != NULL &&
        hdag_hash_len_is_valid(bundle->hash_len) &&
        bundle->nodes_num <= bundle->nodes_allocated &&
        (bundle->nodes != NULL ||
         bundle->nodes_allocated == 0) &&
        bundle->target_hashes_num <= bundle->target_hashes_allocated &&
        (bundle->target_hashes != NULL ||
         bundle->target_hashes_allocated == 0) &&
        bundle->extra_edges_num <= bundle->extra_edges_allocated &&
        (bundle->extra_edges != NULL ||
         bundle->extra_edges_allocated == 0);
}

/**
 * Check if a bundle is empty.
 *
 * @param bundle    The bundle to check. Must be valid.
 *
 * @return True if the bundle is empty, false otherwise.
 */
static inline bool
hdag_bundle_is_empty(const struct hdag_bundle *bundle)
{
    assert(hdag_bundle_is_valid(bundle));
    return !(
        bundle->nodes_num ||
        bundle->target_hashes_num ||
        bundle->extra_edges_num
    );
}

/**
 * Check if a bundle is clean (does not reference allocated data).
 *
 * @param bundle    The bundle to check. Must be valid.
 *
 * @return True if the bundle is clean, false otherwise.
 */
static inline bool
hdag_bundle_is_clean(const struct hdag_bundle *bundle)
{
    assert(hdag_bundle_is_valid(bundle));
    return !(
        bundle->nodes_allocated ||
        bundle->target_hashes_allocated ||
        bundle->extra_edges_allocated
    );
}

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
 * @param bundle    The bundle to load the node sequence into.
 *                  Must be valid and empty. Can be left unclean on failure.
 * @param node_seq  The sequence of nodes (and optionally their targets)
 *                  to load. Node hash length is assumed to be the bundle's
 *                  hash length.
 *
 * @return True if the data was loaded and processed successfully,
 *         false if not. The errno is set in case of failure.
 */
extern bool hdag_bundle_load_node_seq(struct hdag_bundle *bundle,
                                      struct hdag_node_seq node_seq);

#endif /* _HDAG_BUNDLE_H */
