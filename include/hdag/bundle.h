/*
 * Hash DAG bundle - a file in the making
 */

#ifndef _HDAG_BUNDLE_H
#define _HDAG_BUNDLE_H

#include <hdag/edge.h>
#include <hdag/node.h>
#include <hdag/nodes.h>
#include <hdag/node_seq.h>
#include <hdag/darr.h>
#include <hdag/fanout.h>
#include <hdag/res.h>
#include <hdag/misc.h>
#include <stdio.h>

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

    /**
     * An array of node numbers, where each element stores the number of nodes
     * with the first byte of their hash equal to or less than the index of
     * the element. So each element is either equal or greater than the
     * previous one, and the last element represents the total number of nodes
     * in the bundle. No elements can be greater than INT32_MAX.
     * Otherwise the array is considered invalid.
     */
    uint32_t            nodes_fanout[256];

    /**
     * Target hashes.
     * Must be empty if extra_edges is not.
     * If non-empty, the indirect indices in node's targets are pointing here.
     */
    struct hdag_darr    target_hashes;

    /*
     * The array of extra edges, which didn't fit into nodes themselves.
     * Must be empty if target_hashes is not.
     * If non-empty, the indirect indices in node's targets are pointing here.
     */
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
    .nodes_fanout = HDAG_FANOUT_EMPTY,                              \
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
        hdag_darr_occupied_slots(&bundle->nodes) < INT32_MAX &&
        hdag_fanout_is_valid(bundle->nodes_fanout,
                             HDAG_ARR_LEN(bundle->nodes_fanout)) &&
        (hdag_fanout_is_empty(bundle->nodes_fanout,
                              HDAG_ARR_LEN(bundle->nodes_fanout)) ||
         bundle->nodes_fanout[255] ==
            hdag_darr_occupied_slots(&bundle->nodes)) &&
        hdag_darr_is_valid(&bundle->target_hashes) &&
        bundle->target_hashes.slot_size == bundle->hash_len &&
        hdag_darr_occupied_slots(&bundle->target_hashes) < INT32_MAX &&
        hdag_darr_is_valid(&bundle->extra_edges) &&
        bundle->extra_edges.slot_size == sizeof(struct hdag_edge) &&
        hdag_darr_occupied_slots(&bundle->extra_edges) < INT32_MAX &&
        (hdag_darr_occupied_slots(&bundle->target_hashes) == 0 ||
         hdag_darr_occupied_slots(&bundle->extra_edges) == 0);
}

/**
 * Check if a bundle's nodes and targets are all sorted.
 *
 * @param bundle    The bundle to check. Must be valid.
 *
 * @return True if the bundle is sorted, false if not.
 */
extern bool hdag_bundle_is_sorted(const struct hdag_bundle *bundle);

/**
 * Check if a bundle's nodes and targets are all sorted and deduplicated.
 *
 * @param bundle    The bundle to check. Must be valid.
 *
 * @return True if the bundle is sorted and deduplicated, false if not.
 */
extern bool hdag_bundle_is_sorted_and_deduped(
                            const struct hdag_bundle *bundle);

/**
 * Check if a bundle contains nodes that are referencing their target nodes
 * via their indices in the nodes array, directly or via "extra_edges", as
 * opposed to referencing them by their hashes (via "target_hashes").
 *
 * @param bundle    The bundle to check. Must be valid.
 *
 * @return True if the bundle has nodes that are referencing their target
 *         nodes by their indices, false if all of them are referencing them
 *         by hashes.
 */
static inline bool
hdag_bundle_has_index_targets(const struct hdag_bundle *bundle)
{
    ssize_t idx;
    const struct hdag_node *node;
    assert(hdag_bundle_is_valid(bundle));
    HDAG_DARR_ITER_FORWARD(&bundle->nodes, idx, node, (void)0, (void)0) {
        if (hdag_targets_are_direct(&node->targets) ||
            (hdag_targets_are_indirect(&node->targets) &&
             hdag_darr_occupied_slots(&bundle->extra_edges) != 0)) {
            return true;
        }
    }
    return false;
}

/**
 * Check if a bundle is using hashes to refer to targets.
 *
 * @param bundle    The bundle to check. Must be valid.
 *
 * @return True if the bundle is using target hashes, false otherwise.
 */
static inline bool
hdag_bundle_has_hash_targets(const struct hdag_bundle *bundle)
{
    assert(hdag_bundle_is_valid(bundle));
    return hdag_darr_occupied_slots(&bundle->target_hashes) != 0;
}

/**
 * Check if a bundle is completely indexed, and nodes with two or less edges
 * store their targets directly, and not in "extra_edges" array, that is if
 * the bundle is "compacted".
 *
 * @param bundle    The bundle to check. Must be valid.
 *
 * @return True if the bundle is fully-indexed and compacted, false otherwise.
 */
static inline bool
hdag_bundle_is_compacted(const struct hdag_bundle *bundle)
{
    ssize_t idx;
    const struct hdag_node *node;
    assert(hdag_bundle_is_valid(bundle));
    if (hdag_darr_occupied_slots(&bundle->target_hashes) != 0) {
        return false;
    }
    HDAG_DARR_ITER_FORWARD(&bundle->nodes, idx, node, (void)0, (void)0) {
        if (hdag_targets_are_indirect(&node->targets)) {
            if (hdag_node_get_last_ind_idx(node) -
                hdag_node_get_first_ind_idx(node) <= 1) {
                return false;
            }
        }
    }
    return true;
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
 * @return A void universal result.
 */
extern hdag_res hdag_bundle_deflate(struct hdag_bundle *bundle);

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
 * optimization or validation.
 *
 * @param pbundle   The location for the bundle with loaded node sequence.
 *                  Can be NULL to have the bundle discarded after loading.
 *                  Will not be modified on failure.
 * @param node_seq  The sequence of nodes (and optionally their targets)
 *                  to load.
 *
 * @return A void universal result.
 */
extern hdag_res hdag_bundle_node_seq_load(struct hdag_bundle *pbundle,
                                          struct hdag_node_seq node_seq);

/**
 * Load an adjacency list text file into a bundle, but don't do any
 * optimization or validation.
 *
 * @param pbundle   The location for the bundle with loaded node sequence.
 *                  Can be NULL to have the bundle discarded after loading.
 *                  Will not be modified on failure.
 * @param stream    The FILE stream containing the text to parse and load.
 *                  Each line of the stream is expected to contain a node's
 *                  hash followed by hashes of its targets, if any. Each hash is
 *                  represented by a hexadecimal number, separated by
 *                  (non-linebreak) whitespace. Hashes are assumed to be
 *                  right-aligned.
 * @param hash_len  The length of hashes expected to be contained in the
 *                  stream. Must be a valid hash length.
 *
 * @return A void universal result, including HDAG_RES_INVALID_FORMAT,
 *         if the file format is invalid, and HDAG_RES_ERRNO's in case of
 *         libc errors.
 */
extern hdag_res hdag_bundle_txt_load(struct hdag_bundle *pbundle,
                                     FILE *stream, uint16_t hash_len);

/**
 * Output the bundle hash DAG as an adjacency list text file.
 *
 * @param stream    The stream to output the text to.
 * @param bundle    The bundle to output.
 *
 * @return A void universal result.
 */
extern hdag_res hdag_bundle_txt_save(FILE *stream,
                                     const struct hdag_bundle *bundle);

/**
 * Load a node sequence (adjacency list) into a bundle, optimize, and
 * validate.
 *
 * @param pbundle   The location for the bundle with ingested node sequence.
 *                  Can be NULL to have the bundle discarded after ingesting.
 *                  Will not be modified on failure.
 * @param node_seq  The sequence of nodes (and optionally their targets)
 *                  to load.
 *
 * @return A void universal result.
 */
extern hdag_res hdag_bundle_node_seq_ingest(struct hdag_bundle *pbundle,
                                            struct hdag_node_seq node_seq);

/**
 * Load an adjacency list text file into a bundle, optimize and validate.
 *
 * @param pbundle   The location for the bundle with loaded node sequence.
 *                  Can be NULL to have the bundle discarded after loading.
 *                  Will not be modified on failure.
 * @param stream    The FILE stream containing the text to parse and load.
 *                  Each line of the stream is expected to contain a node's
 *                  hash followed by hashes of its targets, if any. Each hash is
 *                  represented by a hexadecimal number, separated by
 *                  (non-linebreak) whitespace. Hashes are assumed to be
 *                  right-aligned.
 * @param hash_len  The length of hashes expected to be contained in the
 *                  stream. Must be a valid hash length.
 *
 * @return A void universal result, including HDAG_RES_INVALID_FORMAT,
 *         if the file format is invalid, HDAG_RES_ERRNO's in case of
 *         libc errors, and HDAG_RES_GRAPH_CYCLE in case of cycles.
 */
extern hdag_res hdag_bundle_txt_ingest(struct hdag_bundle *pbundle,
                                       FILE *stream, uint16_t hash_len);

/**
 * Sort the bundle's nodes and their target nodes by hash, lexicographically,
 * assuming target nodes are not referenced by their indices.
 *
 * @param bundle    The bundle to sort the nodes and targets in.
 */
extern void hdag_bundle_sort(struct hdag_bundle *bundle);

/**
 * Fill in the nodes fanout array for a bundle.
 *
 * @param bundle    The bundle to fill in the nodes fanout array for.
 *                  Must be sorted. And have nodes fanout empty.
 */
extern void hdag_bundle_fanout_fill(struct hdag_bundle *bundle);

/**
 * Check if the nodes_fanout array is empty in a bundle.
 *
 * @param bundle    The bundle to check.
 *
 * @return True if the bundle's nodes_fanout array is empty, false otherwise.
 */
static inline bool
hdag_bundle_fanout_is_empty(const struct hdag_bundle *bundle)
{
    assert(hdag_bundle_is_valid(bundle));
    return hdag_fanout_is_empty(bundle->nodes_fanout,
                                HDAG_ARR_LEN(bundle->nodes_fanout));
}

/**
 * Remove duplicate node entries from a bundle, preferring known ones, as well
 * as duplicate edges. Assume nodes are sorted by hash, and are not using
 * direct-index targets.
 *
 * @param bundle    The bundle to deduplicate nodes in.
 *
 * @return A void universal result.
 */
extern hdag_res hdag_bundle_dedup(struct hdag_bundle *bundle);

/**
 * Convert a bundle's target references from hashes to indexes, and compact
 * edge targets into nodes, putting the rest into "extra_edges", assuming the
 * nodes and edges are sorted and de-duplicated. On success the bundle becomes
 * "indexed" and "compacted".
 *
 * @param bundle    The bundle to compact edges in.
 */
extern void hdag_bundle_compact(struct hdag_bundle *bundle);

/**
 * Invert the graph in a bundle: create a new graph with edge directions
 * changed to the opposite, and generations reset.
 *
 * @param pinverted Location for the inverted bundle.
 *                  Will not be modified on failure.
 *                  Can be NULL to have the result discarded.
 * @param original  The bundle containing the graph to be inverted.
 *                  Must be sorted, deduped, and indexed.
 *
 * @return A void universal result.
 */
extern hdag_res hdag_bundle_invert(struct hdag_bundle *pinverted,
                                   const struct hdag_bundle *original);

/**
 * Enumerate generations in a bundle: assign generation numbers to every node.
 * Resets component IDs
 *
 * @param bundle    The bundle to enumerate.
 *
 * @return A void universal result.
 */
extern hdag_res hdag_bundle_generations_enumerate(struct hdag_bundle *bundle);

/**
 * Check if all bundle nodes have generations assigned (non-zero).
 *
 * @param bundle    The bundle to check.
 *
 * @return True if generations are assigned.
 */
static inline bool
hdag_bundle_all_nodes_have_generations(const struct hdag_bundle *bundle)
{
    ssize_t idx;
    const struct hdag_node *node;

    assert(hdag_bundle_is_valid(bundle));

    HDAG_DARR_ITER_FORWARD(&bundle->nodes, idx, node, (void)0, (void)0) {
        if (!node->generation) {
            return false;
        }
    }

    return true;
}

/**
 * Check if at least one bundle's node has generation assigned (non-zero).
 *
 * @param bundle    The bundle to check.
 *
 * @return True if at least one node has generation assigned.
 */
static inline bool
hdag_bundle_some_nodes_have_generations(const struct hdag_bundle *bundle)
{
    ssize_t idx;
    const struct hdag_node *node;

    assert(hdag_bundle_is_valid(bundle));

    HDAG_DARR_ITER_FORWARD(&bundle->nodes, idx, node, (void)0, (void)0) {
        if (node->generation) {
            return true;
        }
    }

    return false;
}

/**
 * Enumerate components in a bundle: assign component numbers to every node.
 *
 * @param bundle    The bundle to enumerate.
 *
 * @return A void universal result.
 */
extern hdag_res hdag_bundle_components_enumerate(struct hdag_bundle *bundle);

/**
 * Check if all bundle nodes have components assigned (non-zero).
 *
 * @param bundle    The bundle to check.
 *
 * @return True if components are assigned.
 */
static inline bool
hdag_bundle_all_nodes_have_components(const struct hdag_bundle *bundle)
{
    ssize_t idx;
    const struct hdag_node *node;

    assert(hdag_bundle_is_valid(bundle));

    HDAG_DARR_ITER_FORWARD(&bundle->nodes, idx, node, (void)0, (void)0) {
        if (!node->component) {
            return false;
        }
    }

    return true;
}

/**
 * Check if at least one bundle's node has component assigned (non-zero).
 *
 * @param bundle    The bundle to check.
 *
 * @return True if at least one node has component assigned.
 */
static inline bool
hdag_bundle_some_nodes_have_components(const struct hdag_bundle *bundle)
{
    ssize_t idx;
    const struct hdag_node *node;

    assert(hdag_bundle_is_valid(bundle));

    HDAG_DARR_ITER_FORWARD(&bundle->nodes, idx, node, (void)0, (void)0) {
        if (node->component) {
            return true;
        }
    }

    return false;
}

/**
 * Given a bundle and a node index return the node's targets structure.
 *
 * @param bundle        The bundle to get the node target count from.
 * @param node_idx      The index of the node to get the targets for.
 *
 * @return The specified node's targets
 */
static inline struct hdag_targets *
hdag_bundle_targets(struct hdag_bundle *bundle, uint32_t node_idx)
{
    assert(hdag_bundle_is_valid(bundle));
    return &HDAG_DARR_ELEMENT_UNSIZED(
        &bundle->nodes, struct hdag_node, node_idx
    )->targets;
}

/**
 * Given a const bundle and a node index return the node's targets structure.
 *
 * @param bundle        The bundle to get the node target count from.
 * @param node_idx      The index of the node to get the targets for.
 *
 * @return The specified node's targets
 */
static inline const struct hdag_targets *
hdag_bundle_targets_const(const struct hdag_bundle *bundle, uint32_t node_idx)
{
    assert(hdag_bundle_is_valid(bundle));
    return &HDAG_DARR_ELEMENT_UNSIZED(
        &bundle->nodes, struct hdag_node, node_idx
    )->targets;
}

/**
 * Return a (possibly const) targets of a bundle's node at specified index.
 *
 * @param _bundle   The bundle to get the node from.
 * @param _node_idx The index of the node to get targets from.
 *
 * @return The targets pointer (const, if the bundle was const).
 */
#define HDAG_BUNDLE_TARGETS(_bundle, _node_idx) \
    _Generic(                                                   \
        _bundle,                                                \
        struct hdag_bundle *:                                   \
            (struct hdag_targets *)hdag_bundle_targets(         \
                (struct hdag_bundle *)_bundle, _node_idx        \
            ),                                                  \
        const struct hdag_bundle *:                             \
            (const struct hdag_targets *)hdag_bundle_targets(   \
                (struct hdag_bundle *)_bundle, _node_idx        \
            )                                                   \
    )

/**
 * Given a bundle and a node index return the number of node's targets
 * (outgoing edges), that is the node's outdegree.
 *
 * @param bundle        The bundle to get the node target count from.
 * @param node_idx      The index of the node to get the target for.
 *
 * @return The specified node's target count.
 */
static inline uint32_t
hdag_bundle_targets_count(const struct hdag_bundle *bundle, uint32_t node_idx)
{
    assert(hdag_bundle_is_valid(bundle));
    return hdag_targets_count(HDAG_BUNDLE_TARGETS(bundle, node_idx));
}

/**
 * Given a bundle, a node index, and a node's target index, return target
 * node's index.
 *
 * @param bundle        The bundle to get the node's target from.
 * @param node_idx      The index of the node to get the target for.
 * @param target_idx    The index of the target to get node index for.
 *
 * @return The specified target's node index.
 */
static inline uint32_t
hdag_bundle_targets_node_idx(const struct hdag_bundle *bundle,
                             uint32_t node_idx, uint32_t target_idx)
{
    const struct hdag_targets *targets;
    assert(hdag_bundle_is_valid(bundle));
    targets = HDAG_BUNDLE_TARGETS(bundle, node_idx);
    assert(target_idx < hdag_targets_count(targets));

    if (hdag_target_is_ind_idx(targets->first)) {
        if (hdag_darr_is_empty(&bundle->extra_edges)) {
            uint32_t target_node_idx = hdag_nodes_find(
                bundle->nodes.slots,
                bundle->nodes.slots_occupied,
                bundle->hash_len,
                HDAG_DARR_ELEMENT_UNSIZED(
                    &bundle->target_hashes, uint8_t,
                    hdag_target_to_ind_idx(targets->first) +
                    target_idx
                )
            );
            assert(target_node_idx < INT32_MAX);
            return target_node_idx;
        } else {
            return HDAG_DARR_ELEMENT(
                &bundle->extra_edges, struct hdag_edge,
                hdag_target_to_ind_idx(targets->first) +
                target_idx
            )->node_idx;
        }
    }

    if (target_idx == 0 && hdag_target_is_dir_idx(targets->first)) {
        return hdag_target_to_dir_idx(targets->first);
    }

    return hdag_target_to_dir_idx(targets->last);
}

/**
 * Return a bundle's node at specified index.
 *
 * @param bundle    The bundle to get the node from.
 * @param node_idx  The index of the node to get.
 *
 * @return The specified node.
 */
static inline struct hdag_node *
hdag_bundle_node(struct hdag_bundle *bundle, uint32_t node_idx)
{
    assert(hdag_bundle_is_valid(bundle));
    return hdag_darr_element(&bundle->nodes, node_idx);
}

/**
 * Return a (possibly const) bundle's node at specified index.
 *
 * @param _bundle   The bundle to get the node from.
 * @param _node_idx The index of the node to get.
 *
 * @return The node pointer (const, if the bundle was const).
 */
#define HDAG_BUNDLE_NODE(_bundle, _node_idx) \
    _Generic(                                               \
        _bundle,                                            \
        struct hdag_bundle *:                               \
            (struct hdag_node *)hdag_bundle_node(           \
                (struct hdag_bundle *)_bundle, _node_idx    \
            ),                                              \
        const struct hdag_bundle *:                         \
            (const struct hdag_node *)hdag_bundle_node(     \
                (struct hdag_bundle *)_bundle, _node_idx    \
            )                                               \
    )

/**
 * Given a bundle, a node index, and a node's target index, return the target
 * node.
 *
 * @param bundle        The bundle to get the node's target from.
 * @param node_idx      The index of the node to get the target for.
 * @param target_idx    The index of the target to get node for.
 *
 * @return The specified target node.
 */
static inline struct hdag_node *
hdag_bundle_targets_node(struct hdag_bundle *bundle,
                         uint32_t node_idx, uint32_t target_idx)
{
    assert(hdag_bundle_is_valid(bundle));
    return hdag_bundle_node(
        bundle,
        hdag_bundle_targets_node_idx(bundle, node_idx, target_idx)
    );
}

/**
 * Given a (possibly const) bundle, a node index, and a node's target index,
 * return the target node.
 *
 * @param _bundle       The bundle to get the node's target from.
 * @param _node_idx     The index of the node to get the target for.
 * @param _target_idx   The index of the target to get node for.
 *
 * @return The node pointer (const, if the bundle was const).
 */
#define HDAG_BUNDLE_TARGETS_NODE(_bundle, _node_idx, _target_idx) \
    _Generic(                                                           \
        _bundle,                                                        \
        struct hdag_bundle *:                                           \
            (struct hdag_node *)hdag_bundle_targets_node(               \
                (struct hdag_bundle *)_bundle, _node_idx, _target_idx   \
            ),                                                          \
        const struct hdag_bundle *:                                     \
            (const struct hdag_node *)hdag_bundle_targets_node(         \
                (struct hdag_bundle *)_bundle, _node_idx, _target_idx   \
            )                                                           \
    )

/**
 * Given a bundle, a node index, and a node's target index, return target
 * node's hash.
 *
 * @param bundle        The bundle to get the node's target from.
 * @param node_idx      The index of the node to get the target for.
 * @param target_idx    The index of the target to get node index for.
 *
 * @return The specified target's node index.
 */
static inline const uint8_t *
hdag_bundle_targets_node_hash(const struct hdag_bundle *bundle,
                              uint32_t node_idx, uint32_t target_idx)
{
    const struct hdag_targets *targets;
    uint32_t target_node_idx;
    assert(hdag_bundle_is_valid(bundle));
    targets = HDAG_BUNDLE_TARGETS(bundle, node_idx);
    assert(target_idx < hdag_targets_count(targets));

    if (hdag_target_is_ind_idx(targets->first)) {
        if (hdag_darr_is_empty(&bundle->extra_edges)) {
            return HDAG_DARR_ELEMENT_UNSIZED(
                &bundle->target_hashes, uint8_t,
                hdag_target_to_ind_idx(targets->first) +
                target_idx
            );
        } else {
            target_node_idx = HDAG_DARR_ELEMENT(
                &bundle->extra_edges, struct hdag_edge,
                hdag_target_to_ind_idx(targets->first) +
                target_idx
            )->node_idx;
        }
    } else if (target_idx == 0 && hdag_target_is_dir_idx(targets->first)) {
        target_node_idx = hdag_target_to_dir_idx(targets->first);
    } else {
        target_node_idx = hdag_target_to_dir_idx(targets->last);
    }
    return HDAG_BUNDLE_NODE(bundle, target_node_idx)->hash;
}

#endif /* _HDAG_BUNDLE_H */
