/*
 * Hash DAG bundle - a file in the making
 */

#ifndef _HDAG_BUNDLE_H
#define _HDAG_BUNDLE_H

#include <hdag/edge.h>
#include <hdag/node.h>
#include <hdag/node_seq.h>
#include <hdag/darr.h>

/** A bundle */
struct hdag_bundle {
    /** The length of the node hash */
    uint16_t            hash_len;

    /**
     * Nodes.
     * Before compacting, their targets are either unknown, or are indirect
     * indices pointing into the target_hashes array. After compacting their
     * targets are either unknown, direct indices pointing into the same
     * array, or indirect indices pointing into the extra_edges array.
     */
    struct hdag_darr    nodes;

    /** Target hashes */
    struct hdag_darr    target_hashes;

    /** The array of extra edges, which didn't fit into nodes themselves */
    struct hdag_darr    extra_edges;
};

/**
 * An initializer for an empty bundle.
 *
 * @param _hash_len The length of node hashes.
 */
#define HDAG_BUNDLE_EMPTY(_hash_len) (struct hdag_bundle){ \
    .hash_len = hdag_hash_len_validate(_hash_len),                  \
    .nodes = HDAG_DARR_EMPTY(hdag_node_size(_hash_len), 64),        \
    .target_hashes = HDAG_DARR_EMPTY(_hash_len, 64),                \
    .extra_edges = HDAG_DARR_EMPTY(sizeof(struct hdag_edge), 64),   \
}

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
        hdag_darr_is_valid(&bundle->nodes) &&
        bundle->nodes.slot_size == hdag_node_size(bundle->hash_len) &&
        hdag_darr_is_valid(&bundle->target_hashes) &&
        bundle->target_hashes.slot_size == bundle->hash_len &&
        hdag_darr_is_valid(&bundle->extra_edges) &&
        bundle->extra_edges.slot_size == sizeof(struct hdag_edge);
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
    return
        hdag_darr_is_empty(&bundle->nodes) &&
        hdag_darr_is_empty(&bundle->target_hashes) &&
        hdag_darr_is_empty(&bundle->extra_edges);
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
    return
        hdag_darr_is_clean(&bundle->nodes) &&
        hdag_darr_is_clean(&bundle->target_hashes) &&
        hdag_darr_is_clean(&bundle->extra_edges);
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
