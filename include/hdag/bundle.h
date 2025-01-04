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
#include <hdag/ctx.h>
#include <hdag/misc.h>
#include <stdio.h>

/** A bundle */
struct hdag_bundle {
    /**
     * The length of the node hash.
     * Has to be valid according to hdag_hash_len_is_valid(), or be zero.
     * If zero, then nodes in "nodes" don't have hashes, and both
     * "nodes_fanout" and "target_hashes" must be "empty".
     */
    uint16_t            hash_len;

    /**
     * Nodes.
     * Before compacting, their targets are either unknown, or are indirect
     * indices pointing into the target_hashes array. After sorting, they're
     * ordered lexicographically by their hashes. Same goes for their targets.
     * After compacting their targets are either unknown, direct indices
     * pointing into the same array, or indirect indices pointing into the
     * extra_edges array.
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
    .hash_len = (_hash_len) ? hdag_hash_len_validate(_hash_len) : 0,    \
    .nodes = HDAG_DARR_EMPTY(hdag_node_size(_hash_len), 64),            \
    .nodes_fanout = HDAG_FANOUT_EMPTY,                                  \
    .target_hashes = HDAG_DARR_EMPTY(_hash_len, 64),                    \
    .extra_edges = HDAG_DARR_EMPTY(sizeof(struct hdag_edge), 64),       \
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
        (bundle->hash_len == 0 || hdag_hash_len_is_valid(bundle->hash_len)) &&
        hdag_darr_is_valid(&bundle->nodes) &&
        bundle->nodes.slot_size == hdag_node_size(bundle->hash_len) &&
        hdag_darr_occupied_slots(&bundle->nodes) < INT32_MAX &&
        hdag_fanout_is_valid(bundle->nodes_fanout,
                             HDAG_ARR_LEN(bundle->nodes_fanout)) &&
        (hdag_fanout_is_empty(bundle->nodes_fanout,
                              HDAG_ARR_LEN(bundle->nodes_fanout)) ||
         bundle->nodes_fanout[255] ==
            hdag_darr_occupied_slots(&bundle->nodes)) &&
        (bundle->hash_len != 0 ||
         hdag_fanout_is_empty(bundle->nodes_fanout,
                              HDAG_ARR_LEN(bundle->nodes_fanout))) &&
        hdag_darr_is_valid(&bundle->target_hashes) &&
        bundle->target_hashes.slot_size == bundle->hash_len &&
        hdag_darr_occupied_slots(&bundle->target_hashes) < INT32_MAX &&
        hdag_darr_is_valid(&bundle->extra_edges) &&
        bundle->extra_edges.slot_size == sizeof(struct hdag_edge) &&
        hdag_darr_occupied_slots(&bundle->extra_edges) < INT32_MAX &&
        (bundle->hash_len != 0 || hdag_darr_is_empty(&bundle->target_hashes)) &&
        (hdag_darr_is_empty(&bundle->target_hashes) ||
         hdag_darr_is_empty(&bundle->extra_edges));
}

/**
 * Check if a bundle is "hashless" (has zero-length hashes).
 *
 * @param bundle    The bundle to check.
 *
 * @return True if the bundle is hashless, false otherwise.
 */
static inline bool
hdag_bundle_is_hashless(const struct hdag_bundle *bundle)
{
    assert(hdag_bundle_is_valid(bundle));
    return bundle->hash_len == 0;
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
[[nodiscard]]
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
 * Create a bundle from a node sequence (adjacency list), but don't do any
 * optimization or validation.
 *
 * @param pbundle   The location for the bundle created from the node
 *                  sequence. Can be NULL to have the bundle discarded after
 *                  creating. Will not be modified on failure.
 * @param node_seq  The sequence of nodes (and optionally their targets)
 *                  to create the bundle from.
 *
 * @return A void universal result.
 */
[[nodiscard]]
extern hdag_res hdag_bundle_from_node_seq(struct hdag_bundle *pbundle,
                                          struct hdag_node_seq *node_seq);

/**
 * Create a bundle from an adjacency list text file, but don't do any
 * optimization or validation.
 *
 * @param pbundle   The location for the bundle created from the text file.
 *                  Can be NULL to have the bundle discarded after creating.
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
[[nodiscard]]
extern hdag_res hdag_bundle_from_txt(struct hdag_bundle *pbundle,
                                     FILE *stream, uint16_t hash_len);

/**
 * Output the hash DAG of a bundle into an adjacency list text file.
 *
 * @param stream    The stream to output the text to.
 * @param bundle    The bundle to output. Must be valid and have hashes.
 *
 * @return A void universal result.
 */
[[nodiscard]]
extern hdag_res hdag_bundle_to_txt(FILE *stream,
                                   const struct hdag_bundle *bundle);

/**
 * Sort the bundle's nodes and their target nodes by hash, lexicographically,
 * assuming target nodes are not referenced by their indices.
 *
 * @param bundle    The bundle to sort the nodes and targets in.
 *                  Must be valid.
 */
extern void hdag_bundle_sort(struct hdag_bundle *bundle);

/**
 * Fill in the nodes fanout array for a bundle.
 *
 * @param bundle    The bundle to fill in the nodes fanout array for.
 *                  Must be valid, have hashes, be sorted, and have nodes
 *                  fanout empty.
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
 *                  Must be valid, and have hashes.
 * @param ctx       The context of this bundle (the abstract supergraph) to
 *                  check if nodes are duplicates. Can be NULL, which is
 *                  interpreted as an empty context.
 *
 * @return A void universal result. Including:
 *         * HDAG_RES_NODE_CONFLICT, if nodes with matching hashes but
 *           different targets were found.
 */
[[nodiscard]]
extern hdag_res hdag_bundle_dedup(struct hdag_bundle *bundle,
                                  const struct hdag_ctx *ctx);

/**
 * Convert a bundle's target references from hashes to indexes, and compact
 * edge targets into nodes, putting the rest into "extra_edges", assuming the
 * nodes and edges are sorted and de-duplicated. On success the bundle becomes
 * "indexed" and "compacted".
 *
 * @param bundle    The bundle to compact edges in.
 *                  Must be valid, and have hashes.
 *
 * @return A void universal result.
 */
[[nodiscard]]
extern hdag_res hdag_bundle_compact(struct hdag_bundle *bundle);

/**
 * Invert the graph in a bundle: create a new graph with edge directions
 * changed to the opposite, and generations reset.
 *
 * @param pinverted Location for the inverted bundle.
 *                  Will not be modified on failure.
 *                  Can be NULL to have the result discarded.
 * @param original  The bundle containing the graph to be inverted.
 *                  Must be sorted, deduped, and indexed.
 * @param hashless  True if the inverted array should have hashes dropped.
 *                  False if the original hashes should be preserved.
 *
 * @return A void universal result.
 */
[[nodiscard]]
extern hdag_res hdag_bundle_invert(struct hdag_bundle *pinverted,
                                   const struct hdag_bundle *original,
                                   bool hashless);

/**
 * Check if all bundle's nodes are unenumerated (have no component or
 * generation assigned, that is both are set to zero).
 *
 * @param bundle    The bundle to check.
 *
 * @return True if at all bundle's node are unenumerated.
 */
static inline bool
hdag_bundle_is_unenumerated(const struct hdag_bundle *bundle)
{
    ssize_t idx;
    const struct hdag_node *node;

    assert(hdag_bundle_is_valid(bundle));

    HDAG_DARR_ITER_FORWARD(&bundle->nodes, idx, node, (void)0, (void)0) {
        if (node->component || node->generation) {
            return false;
        }
    }

    return true;
}

/**
 * Enumerate components and generations in a bundle: assign component and
 * generation numbers to every node.
 *
 * @param bundle    The bundle to enumerate. Must be unenumerated.
 * @param ctx       The context of this bundle (the abstract supergraph) to
 *                  retrieve connected component and generation numbers. Can
 *                  be NULL, which is interpreted as an empty context.
 *
 * @return A void universal result.
 */
[[nodiscard]]
extern hdag_res hdag_bundle_enumerate(struct hdag_bundle *bundle,
                                      const struct hdag_ctx *ctx);

/**
 * Check if all bundle nodes are enumerated. That is have both components and
 * generations assigned (non-zero).
 *
 * @param bundle    The bundle to check.
 *
 * @return True if all bundle's nodes are enumerated.
 */
static inline bool
hdag_bundle_is_enumerated(const struct hdag_bundle *bundle)
{
    ssize_t idx;
    const struct hdag_node *node;

    assert(hdag_bundle_is_valid(bundle));

    HDAG_DARR_ITER_FORWARD(&bundle->nodes, idx, node, (void)0, (void)0) {
        if (!(node->component && node->generation)) {
            return false;
        }
    }

    return true;
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
 * @return The specified target node's hash.
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

/** The (resettable) node target hash sequence */
struct hdag_bundle_targets_hash_seq {
    /** The base abstract hash sequence */
    struct hdag_hash_seq        base;
    /** The bundle with the node having targets to go over */
    const struct hdag_bundle   *bundle;
    /** The index of the node having targets to go over */
    uint32_t                    node_idx;
    /** The index of the target to return next */
    uint32_t                    target_idx;
};

/** A next-hash retrieval function for target hash sequence */
[[nodiscard]]
extern hdag_res hdag_bundle_targets_hash_seq_next_fn(
                    struct hdag_hash_seq *base_seq,
                    uint8_t *phash);

/** A reset function for target hash sequence */
extern void hdag_bundle_targets_hash_seq_reset_fn(
                    struct hdag_hash_seq *base_seq);

/**
 * Initialize a sequence of target hashes for a bundle node.
 *
 * @param pseq      The location of the target hash sequence to initialize.
 * @param bundle    The bundle containing the node.
 * @param node_idx  The index of the node to return the target hashes for.
 *
 * @return The base abstract sequence pointer.
 */
static inline struct hdag_hash_seq *
hdag_bundle_targets_hash_seq_init(
                    struct hdag_bundle_targets_hash_seq *pseq,
                    const struct hdag_bundle *bundle,
                    uint32_t node_idx)
{
    assert(pseq != NULL);
    assert(hdag_bundle_is_valid(bundle));
    assert(node_idx < bundle->nodes.slots_occupied);

    *pseq = (struct hdag_bundle_targets_hash_seq){
        .base = {
            .hash_len = bundle->hash_len,
            .reset_fn = hdag_bundle_targets_hash_seq_reset_fn,
            .next_fn = hdag_bundle_targets_hash_seq_next_fn,
        },
        .bundle = bundle,
        .node_idx = node_idx,
        .target_idx = 0,
    };

    assert(hdag_hash_seq_is_valid(&pseq->base));
    assert(hdag_hash_seq_is_resettable(&pseq->base));
    return &pseq->base;
}

/**
 * Lookup the index of a node within a bundle, using its hash.
 *
 * @param bundle    The bundle to look up the node in.
 *                  Must have the nodes fanout filled in.
 * @param hash_ptr  The hash the node must have.
 *                  The hash length must match the bundle hash length.
 *
 * @return The index of the found node (< INT32_MAX),
 *         or INT32_MAX, if not found.
 */
static inline uint32_t
hdag_bundle_find_node_idx(const struct hdag_bundle *bundle,
                          const uint8_t *hash_ptr)
{
    assert(hdag_bundle_is_valid(bundle));
    assert(hdag_darr_occupied_slots(&bundle->nodes) == 0 ||
           !hdag_bundle_fanout_is_empty(bundle));
    return hdag_nodes_slice_find(
        bundle->nodes.slots,
        (*hash_ptr == 0 ? 0 : bundle->nodes_fanout[*hash_ptr - 1]),
        bundle->nodes_fanout[*hash_ptr],
        bundle->hash_len,
        hash_ptr
    );
}

/**
 * Check if a bundle is completely unorganized, that is no organizing
 * has been done to it at all.
 *
 * @param bundle    The bundle to check. Must be valid and not "hashless".
 *
 * @return True if the bundle is completely unorganized, false if not.
 */
extern bool hdag_bundle_is_unorganized(const struct hdag_bundle *bundle);

/**
 * Organize a bundle - prepare it for becoming a file - sort, dedup, fill
 * the fanout, compact, enumerate generations and components, and deflate.
 *
 * @param bundle    The bundle to organize. Must be completely unorganized.
 * @param ctx       The context of this bundle (the abstract supergraph) to
 *                  check if nodes are duplicates. Can be NULL, which is
 *                  interpreted as an empty context.
 *
 * @return A void universal result.
 */
[[nodiscard]]
extern hdag_res hdag_bundle_organize(struct hdag_bundle *bundle,
                                     const struct hdag_ctx *ctx);

/**
 * Check if a bundle is fully organized, that is ready to become a file.
 *
 * @param bundle    The bundle to check. Must be valid and not "hashless".
 *
 * @return True if the bundle is fully organized, false if not.
 */
extern bool hdag_bundle_is_organized(const struct hdag_bundle *bundle);

/**
 * Create a bundle from a node sequence (adjacency list), and organize it.
 *
 * @param pbundle   The location for the bundle created from the node
 *                  sequence. Can be NULL to have the bundle discarded after
 *                  creation. Will not be modified on failure.
 * @param ctx       The context of the created bundle (the abstract
 *                  supergraph) to check if nodes are duplicates. Can be NULL,
 *                  which is interpreted as an empty context.
 * @param node_seq  The sequence of nodes (and optionally their targets)
 *                  to create the bundle from.
 *
 * @return A void universal result.
 */
[[nodiscard]]
extern hdag_res hdag_bundle_organized_from_node_seq(
                        struct hdag_bundle *pbundle,
                        const struct hdag_ctx *ctx,
                        struct hdag_node_seq *node_seq);

/**
 * Create a bundle from an adjacency list text file, optimize and validate.
 *
 * @param pbundle   The location for the bundle created from the adjacency
 *                  list text file. Can be NULL to have the bundle discarded
 *                  after creating. Will not be modified on failure.
 * @param ctx       The context of the created bundle (the abstract
 *                  supergraph) to check if nodes are duplicates. Can be NULL,
 *                  which is interpreted as an empty context.
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
[[nodiscard]]
extern hdag_res hdag_bundle_organized_from_txt(
                        struct hdag_bundle *pbundle,
                        const struct hdag_ctx *ctx,
                        FILE *stream, uint16_t hash_len);

/** A next-node retrieval function for bundle's node sequence */
extern hdag_res hdag_bundle_node_seq_next_fn(
                            struct hdag_node_seq *base_seq,
                            uint8_t *phash,
                            struct hdag_hash_seq **ptarget_hash_seq);

/** A reset function for bundle's node sequence */
extern void hdag_bundle_node_seq_reset_fn(
                            struct hdag_node_seq *base_seq);

/** Bundle's (resettable) node sequence */
struct hdag_bundle_node_seq {
    /** The base abstract node sequence */
    struct hdag_node_seq                    base;
    /** The bundle being iterated over */
    const struct hdag_bundle               *bundle;
    /** The index of the next node to return */
    size_t                                  node_idx;
    /** The returned node's target hash sequence */
    struct hdag_bundle_targets_hash_seq     targets_hash_seq;
};

/*
 * Initialize a sequence of hashes of targets of a node from a bundle.
 *
 * @param pseq      Location for the node sequence.
 * @param pseq_data Location for the node sequence data.
 * @param bundle    The bundle to setup the sequence from.
 *
 * @return The setup node sequence pointer ("pseq").
 */
static inline struct hdag_node_seq *
hdag_bundle_node_seq_init(struct hdag_bundle_node_seq *pseq,
                          const struct hdag_bundle *bundle)
{
    assert(pseq != NULL);
    assert(hdag_bundle_is_valid(bundle));

    *pseq = (struct hdag_bundle_node_seq){
        .base = {
            .hash_len = bundle->hash_len,
            .reset_fn = hdag_bundle_node_seq_reset_fn,
            .next_fn = hdag_bundle_node_seq_next_fn,
        },
        .bundle = bundle,
        .node_idx = 0,
    };

    assert(hdag_node_seq_is_valid(&pseq->base));
    assert(hdag_node_seq_is_resettable(&pseq->base));
    return &pseq->base;
}


#endif /* _HDAG_BUNDLE_H */
