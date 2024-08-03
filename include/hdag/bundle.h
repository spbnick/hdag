/*
 * Hash DAG bundle - a file in the making
 */

#ifndef _HDAG_BUNDLE_H
#define _HDAG_BUNDLE_H

#include <hdag/edge.h>
#include <hdag/node.h>
#include <hdag/node_seq.h>
#include <hdag/darr.h>
#include <stdio.h>

/** A bundle */
struct hdag_bundle {
    /** The length of the node hash */
    uint16_t            hash_len;

    /**
     * True, if the indirect target indices in nodes point to the
     * "extra_edges" array, false if they point to "target_hashes" instead.
     */
    bool ind_extra_edges;

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
 * Check if a bundle's nodes and targets are sorted.
 *
 * @param bundle    The bundle to check. Must be valid.
 *
 * @return True if the bundle is sorted, false if not.
 */
extern bool hdag_bundle_is_sorted(const struct hdag_bundle *bundle);

/**
 * Check if a bundle's nodes and targets are sorted and deduplicated.
 *
 * @param bundle    The bundle to check. Must be valid.
 *
 * @return True if the bundle is sorted and deduplicated, false if not.
 */
extern bool hdag_bundle_is_sorted_and_deduped(
                            const struct hdag_bundle *bundle);

/**
 * Check if a bundle's nodes are referencing their target nodes via their
 * indices in the nodes array (directly or via "extra_edges", as opposed to
 * referencing them by their hashes (via "target_hashes").
 *
 * @param bundle    The bundle to check. Must be valid.
 *
 * @return True if nodes are referencing their target nodes by their
 *         indices, false if they are referencing them by hashes.
 */
static inline bool
hdag_bundle_is_indexed(const struct hdag_bundle *bundle)
{
    ssize_t idx;
    const struct hdag_node *node;
    assert(hdag_bundle_is_valid(bundle));
    HDAG_DARR_ITER_FORWARD(&bundle->nodes, idx, node, (void)0, (void)0) {
        if (hdag_targets_are_direct(&node->targets) ||
            (bundle->ind_extra_edges &&
             hdag_targets_are_indirect(&node->targets))) {
            return true;
        }
    }
    return false;
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
 * Deflate a bundle, releasing any extra allocated memory
 *
 * @param bundle    The bundle to deflate.
 *
 * @return True if deflating succeeded, false if memory reallocation failed,
 *         and errno was set.
 */
extern bool hdag_bundle_deflate(struct hdag_bundle *bundle);

/**
 * Empty a bundle, removing all data, but not releasing any memory.
 *
 * @param bundle    The bundle to empty.
 */
extern void hdag_bundle_empty(struct hdag_bundle *bundle);

/**
 * Cleanup a bundle, freeing associated memory (but not the bundle
 * structure itself).
 *
 * @param bundle    The bundle to cleanup.
 */
extern void hdag_bundle_cleanup(struct hdag_bundle *bundle);

/**
 * Load a node sequence (adjacency list) into a bundle, but don't do any
 * optimization.
 *
 * @param bundle    The bundle to load the node sequence into.
 *                  Must be valid and empty. Can be left unclean on failure.
 * @param node_seq  The sequence of nodes (and optionally their targets)
 *                  to load. Node hash length is assumed to be the bundle's
 *                  hash length.
 *
 * @return True if the data was loaded successfully,
 *         false if not. The errno is set in case of failure.
 */
extern bool hdag_bundle_load_node_seq(struct hdag_bundle *bundle,
                                      struct hdag_node_seq node_seq);

/**
 * Load a node sequence (adjacency list) into a bundle, and optimize.
 *
 * @param bundle    The bundle to load the node sequence into.
 *                  Must be valid and empty. Can be left unclean on failure.
 * @param node_seq  The sequence of nodes (and optionally their targets)
 *                  to load. Node hash length is assumed to be the bundle's
 *                  hash length.
 *
 * @return True if the data was loaded and optimized successfully,
 *         false if not. The errno is set in case of failure.
 */
extern bool hdag_bundle_ingest_node_seq(struct hdag_bundle *bundle,
                                        struct hdag_node_seq node_seq);

/**
 * Sort the bundle's nodes and their target nodes by hash, lexicographically,
 * assuming target nodes are not referenced by their indices.
 *
 * @param bundle    The bundle to sort the nodes and targets in.
 */
extern void hdag_bundle_sort(struct hdag_bundle *bundle);

/**
 * Remove duplicate node entries from a bundle, preferring known ones, as well
 * as duplicate edges. Assume nodes are sorted by hash, and are not using
 * direct-index targets.
 *
 * @param bundle    The bundle to deduplicate nodes in.
 */
extern void hdag_bundle_dedup(struct hdag_bundle *bundle);

/**
 * Compact a bundle's edge targets into nodes, putting the rest into
 * "extra_edges", assuming the nodes are sorted, de-duplicated, reference
 * target hashes as their indirect targets and don't have direct targets.
 *
 * @param bundle    The bundle to compact edges in.
 *
 * @return True if compaction succeeded, false if it failed.
 *         Errno is set in case of failure.
 */
extern bool hdag_bundle_compact(struct hdag_bundle *bundle);

/**
 * Write a Graphviz DOT representation of the graph in the bundle to a file.
 *
 * @param bundle    The bundle to write the representation of.
 * @param name      The name to give the output digraph.
 * @param stream    The FILE stream to write the representation to.
 *
 * @return True if writing succeeded, false if not, and errno was set.
 */
extern bool hdag_bundle_write_dot(const struct hdag_bundle *bundle,
                                  const char *name, FILE *stream);

#endif /* _HDAG_BUNDLE_H */
