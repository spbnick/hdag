/**
 * Hash DAG bundle - a set of (organized) DAGs
 */

#ifndef _HDAG_BUNDLE_H
#define _HDAG_BUNDLE_H

#include <hdag/file.h>
#include <hdag/edge.h>
#include <hdag/node.h>
#include <hdag/nodes.h>
#include <hdag/node_iter.h>
#include <hdag/arr.h>
#include <hdag/fanout.h>
#include <hdag/res.h>
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
    struct hdag_arr     nodes;

    /**
     * An array of node numbers, where each element stores the number of nodes
     * with the first byte of their hash equal to or less than the index of
     * the element. So each element is either equal or greater than the
     * previous one, and the last element represents the total number of nodes
     * in the bundle. No elements can be greater than INT32_MAX.
     * Otherwise the array is considered invalid.
     */
    struct hdag_arr     nodes_fanout;

    /**
     * Target hashes.
     * Must be empty if extra_edges is not.
     * If non-empty, the indirect indices in node's targets are pointing here.
     */
    struct hdag_arr     target_hashes;

    /**
     * The array of extra edges, which didn't fit into nodes themselves.
     * Must be empty if target_hashes is not. Filled in when "compacting".
     * If non-empty, the indirect indices in node's targets are pointing here.
     */
    struct hdag_arr     extra_edges;

    /**
     * Indexes (uint32_t) of "unknown" nodes. Filled in when "compacting".
     * Duplicates "nodes" somewhat for faster reference checks.
     */
    struct hdag_arr     unknown_indexes;

    /**
     * The file containing the bundle (closed, if none).
     * The bundle can be "filed" - put into/linked to a file, in which case
     * this file will be "open", and "unfiled" - detached from a file, when
     * this file is "closed". Only an organized bundle can be "filed".
     */
    struct hdag_file    file;
};

/**
 * An initializer for an empty bundle.
 *
 * @param _hash_len The length of node hashes.
 */
#define HDAG_BUNDLE_EMPTY(_hash_len) (struct hdag_bundle){ \
    .hash_len = (_hash_len) ? hdag_hash_len_validate(_hash_len) : 0,    \
    .nodes = HDAG_ARR_EMPTY(hdag_node_size(_hash_len), 64),             \
    .nodes_fanout = HDAG_ARR_EMPTY(sizeof(uint32_t), 256),              \
    .target_hashes = HDAG_ARR_EMPTY(_hash_len, 64),                     \
    .extra_edges = HDAG_ARR_EMPTY(sizeof(struct hdag_edge), 64),        \
    .unknown_indexes = HDAG_ARR_EMPTY(sizeof(uint32_t), 16),            \
    .file = HDAG_FILE_CLOSED,                                           \
}

/**
 * Check if a bundle is valid.
 *
 * @param bundle    The bundle to check.
 *
 * @return True if the bundle is valid, false otherwise.
 */
extern bool hdag_bundle_is_valid(const struct hdag_bundle *bundle);

/**
 * Validate a bundle.
 *
 * @param bundle    The bundle to validate.
 *
 * @return The validated bundle.
 */
static inline struct hdag_bundle*
hdag_bundle_validate(struct hdag_bundle *bundle)
{
    assert(hdag_bundle_is_valid(bundle));
    return bundle;
}

/**
 * Check if a bundle is mutable.
 *
 * @param bundle    The bundle to check.
 *
 * @return True if the bundle is mutable, false otherwise.
 */
extern bool hdag_bundle_is_mutable(const struct hdag_bundle *bundle);

/**
 * Check if a bundle is immutable.
 *
 * @param bundle    The bundle to check.
 *
 * @return True if the bundle is immutable, false otherwise.
 */
extern bool hdag_bundle_is_immutable(const struct hdag_bundle *bundle);

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
extern bool hdag_bundle_has_index_targets(const struct hdag_bundle *bundle);

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
    return hdag_arr_slots_occupied(&bundle->target_hashes) != 0;
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
extern bool hdag_bundle_is_compacted(const struct hdag_bundle *bundle);

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
        hdag_arr_is_empty(&bundle->nodes) &&
        hdag_arr_is_empty(&bundle->nodes_fanout) &&
        hdag_arr_is_empty(&bundle->target_hashes) &&
        hdag_arr_is_empty(&bundle->extra_edges) &&
        hdag_arr_is_empty(&bundle->unknown_indexes);
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
        hdag_arr_is_clean(&bundle->nodes) &&
        hdag_arr_is_clean(&bundle->nodes_fanout) &&
        hdag_arr_is_clean(&bundle->target_hashes) &&
        hdag_arr_is_clean(&bundle->extra_edges) &&
        hdag_arr_is_clean(&bundle->unknown_indexes);
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
 * Create a bundle from a node iterator (adjacency list), but don't do any
 * optimization or validation.
 *
 * @param pbundle   The location for the bundle created from the node
 *                  iterator. Can be NULL to have the bundle discarded after
 *                  creating. Will not be modified on failure.
 * @param node_iter The iterator of nodes (and optionally their targets)
 *                  to create the bundle from.
 *
 * @return A void universal result.
 */
[[nodiscard]]
extern hdag_res hdag_bundle_from_node_iter(struct hdag_bundle *pbundle,
                                           const struct hdag_iter *iter);

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
 * Sort the bundle's nodes (and their targets), lexicographically, then
 * deduplicate both, assuming target nodes are not referenced by their
 * indices. 
 *
 * @param bundle        The bundle to sort and deduplicate
 *                      the nodes and targets in. Must be valid.
 * @param merge_targets If true, targets of same-hash nodes are merged
 *                      together when deduping. If false, same-hash nodes will
 *                      be verified to have the same set of targets before
 *                      deduping.
 *
 * @return A void universal result.
 */
[[nodiscard]]
extern hdag_res hdag_bundle_sort_and_dedup(struct hdag_bundle *bundle,
                                           bool merge_targets);

/**
 * Fill in the nodes fanout array for a bundle.
 *
 * @param bundle    The bundle to fill in the nodes fanout array for.
 *                  Must be valid, have hashes, and be sorted.
 *
 * @return A void universal result.
 */
[[nodiscard]]
extern hdag_res hdag_bundle_fanout_fill(struct hdag_bundle *bundle);

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
    return hdag_fanout_arr_is_empty(&bundle->nodes_fanout);
}

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
extern bool hdag_bundle_is_unenumerated(const struct hdag_bundle *bundle);

/**
 * Enumerate components and generations in a bundle: assign component and
 * generation numbers to every node.
 *
 * @param bundle    The bundle to enumerate. Must be unenumerated.
 *
 * @return A void universal result.
 */
[[nodiscard]]
extern hdag_res hdag_bundle_enumerate(struct hdag_bundle *bundle);

/**
 * Check if all bundle nodes are enumerated. That is have both components and
 * generations assigned (non-zero).
 *
 * @param bundle    The bundle to check.
 *
 * @return True if all bundle's nodes are enumerated.
 */
extern bool hdag_bundle_is_enumerated(const struct hdag_bundle *bundle);

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
    return &HDAG_ARR_ELEMENT_UNSIZED(
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
    return &HDAG_ARR_ELEMENT_UNSIZED_CONST(
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
    _Generic(                                                               \
        _bundle,                                                            \
        struct hdag_bundle *:                                               \
            hdag_bundle_targets((struct hdag_bundle *)_bundle, _node_idx),  \
        const struct hdag_bundle *:                                         \
            hdag_bundle_targets_const(_bundle, _node_idx)                   \
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
extern uint32_t hdag_bundle_targets_node_idx(const struct hdag_bundle *bundle,
                                             uint32_t node_idx,
                                             uint32_t target_idx);

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
    return hdag_arr_element(&bundle->nodes, node_idx);
}

/**
 * Return a const bundle's const node at specified index.
 *
 * @param bundle    The bundle to get the node from.
 * @param node_idx  The index of the node to get.
 *
 * @return The specified node.
 */
static inline const struct hdag_node *
hdag_bundle_node_const(const struct hdag_bundle *bundle, uint32_t node_idx)
{
    assert(hdag_bundle_is_valid(bundle));
    return hdag_arr_element_const(&bundle->nodes, node_idx);
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
            hdag_bundle_node(                               \
                (struct hdag_bundle *)_bundle, _node_idx    \
            ),                                              \
        const struct hdag_bundle *:                         \
            hdag_bundle_node_const(_bundle, _node_idx)      \
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
extern const uint8_t *hdag_bundle_targets_node_hash(
                                const struct hdag_bundle *bundle,
                                uint32_t node_idx, uint32_t target_idx);

/**
 * Lookup the index of a node within a bundle, using its hash.
 *
 * @param bundle    The bundle to look up the node in.
 * @param hash      The hash the node must have.
 *                  The hash length must match the bundle hash length.
 *
 * @return The index of the found node (< INT32_MAX),
 *         or INT32_MAX, if not found.
 */
static inline uint32_t
hdag_bundle_find_node_idx(const struct hdag_bundle *bundle,
                          const uint8_t *hash)
{
    assert(hdag_bundle_is_valid(bundle));
    return hdag_nodes_slice_find(
        bundle->nodes.slots,
        ((*hash == 0 || hdag_bundle_fanout_is_empty(bundle)) ? 0 :
         hdag_fanout_arr_get(&bundle->nodes_fanout, *hash - 1)),
        (hdag_bundle_fanout_is_empty(bundle) ?
            hdag_arr_slots_occupied(&bundle->nodes) :
            hdag_fanout_arr_get(&bundle->nodes_fanout, *hash)),
        bundle->hash_len,
        hash
    );
}

/**
 * Lookup a node within a bundle, using its hash.
 *
 * @param bundle    The bundle to look up the node in.
 *                  Must have the nodes fanout filled in.
 * @param hash      The hash the node must have.
 *                  The hash length must match the bundle hash length.
 *
 * @return The pointer to the found node, or NULL, if not found.
 */
static inline struct hdag_node *
hdag_bundle_find_node(struct hdag_bundle *bundle,
                      const uint8_t *hash)
{
    assert(hdag_bundle_is_valid(bundle));
    assert(hdag_bundle_is_mutable(bundle));
    uint32_t node_idx = hdag_bundle_find_node_idx(bundle, hash);
    return node_idx >= INT32_MAX ? NULL
        : hdag_arr_element(&bundle->nodes, node_idx);
}

/**
 * Lookup a constant node within a constant bundle, using its hash.
 *
 * @param bundle    The bundle to look up the node in.
 *                  Must have the nodes fanout filled in.
 * @param hash      The hash the node must have.
 *                  The hash length must match the bundle hash length.
 *
 * @return The pointer to the found node, or NULL, if not found.
 */
static inline const struct hdag_node *
hdag_bundle_find_node_const(const struct hdag_bundle *bundle,
                            const uint8_t *hash)
{
    assert(hdag_bundle_is_valid(bundle));
    uint32_t node_idx = hdag_bundle_find_node_idx(bundle, hash);
    return node_idx >= INT32_MAX ? NULL
        : hdag_arr_element_const(&bundle->nodes, node_idx);
}

/**
 * Find a (possibly const) node in a (possibly const) bundle, using its hash.
 *
 * @param _bundle   The bundle to look up the node in.
 *                  Must have the nodes fanout filled in.
 * @param _hash     The hash the node must have.
 *                  The hash length must match the bundle hash length.
 *
 * @return The node pointer (const, if the bundle was const).
 */
#define HDAG_BUNDLE_FIND_NODE(_bundle, _hash) \
    _Generic(                                           \
        _bundle,                                        \
        struct hdag_bundle *:                           \
            hdag_bundle_find_node(                      \
                (struct hdag_bundle *)_bundle, _hash    \
            ),                                          \
        const struct hdag_bundle *:                     \
            hdag_bundle_find_node_const(_bundle, _hash) \
    )


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
 * @param bundle        The bundle to organize. Must be completely unorganized.
 * @param merge_targets If true, targets of same-hash nodes are merged
 *                      together when deduping. If false, same-hash nodes will
 *                      be verified to have the same set of targets before
 *                      deduping.
 *
 * @return A void universal result.
 */
[[nodiscard]]
extern hdag_res hdag_bundle_organize(struct hdag_bundle *bundle,
                                     bool merge_targets);

/**
 * Check if a bundle is fully organized, that is ready to become a file.
 *
 * @param bundle    The bundle to check. Must be valid and not "hashless".
 *
 * @return True if the bundle is fully organized, false if not.
 */
extern bool hdag_bundle_is_organized(const struct hdag_bundle *bundle);

/**
 * Create a bundle from a node iterator (adjacency list), and organize it.
 *
 * @param pbundle       The location for the bundle created from the node
 *                      iterator. Can be NULL to have the bundle discarded
 *                      after creation. Will not be modified on failure.
 * @param merge_targets If true, targets of same-hash nodes are merged
 *                      together when deduping. If false, same-hash nodes will
 *                      be verified to have the same set of targets before
 *                      deduping.
 * @param iter          The iterator of nodes (and optionally their targets)
 *                      to create the bundle from.
 *
 * @return A void universal result.
 */
[[nodiscard]]
extern hdag_res hdag_bundle_organized_from_node_iter(
                        struct hdag_bundle *pbundle,
                        bool merge_targets,
                        const struct hdag_iter *iter);

/**
 * Create a bundle from an adjacency list text file, optimize and validate.
 *
 * @param pbundle       The location for the bundle created from the adjacency
 *                      list text file. Can be NULL to have the bundle
 *                      discarded after creating. Will not be modified on
 *                      failure.
 * @param merge_targets If true, targets of same-hash nodes are merged
 *                      together when deduping. If false, same-hash nodes will
 *                      be verified to have the same set of targets before
 *                      deduping.
 * @param stream        The FILE stream containing the text to parse and load.
 *                      Each line of the stream is expected to contain a
 *                      node's hash followed by hashes of its targets, if any.
 *                      Each hash is represented by a hexadecimal number,
 *                      separated by (non-linebreak) whitespace. Hashes are
 *                      assumed to be right-aligned.
 * @param hash_len      The length of hashes expected to be contained in the
 *                      stream. Must be a valid hash length.
 *
 * @return A void universal result, including HDAG_RES_INVALID_FORMAT,
 *         if the file format is invalid, HDAG_RES_ERRNO's in case of
 *         libc errors, and HDAG_RES_GRAPH_CYCLE in case of cycles.
 */
[[nodiscard]]
extern hdag_res hdag_bundle_organized_from_txt(
                        struct hdag_bundle *pbundle,
                        bool merge_targets,
                        FILE *stream, uint16_t hash_len);

/** Bundle's universal iterator data */
struct hdag_bundle_iter_data {
    /** The bundle being iterated over, NULL for a void iterator */
    const struct hdag_bundle   *bundle;
    /** If true, return unknown nodes too */
    bool                        with_unknown;
    /** The index of the last returned node + 1, zero for none */
    size_t                      node_idx;
    /** The index of the target to return next */
    size_t                      target_idx;
    /** The returned node item */
    struct hdag_node_iter_item  item;
};

/**
 * Check if a bundle's universal iterator data is valid.
 *
 * @param data   The iterator data to check.
 *
 * @return True if the iterator data is valid, false otherwise.
 */
static inline bool
hdag_bundle_iter_data_is_valid(const struct hdag_bundle_iter_data *data)
{
    return data != NULL && (
        data->bundle == NULL || (
            hdag_bundle_is_valid(data->bundle) && (
                /* We haven't returned any node yet */
                data->node_idx == 0 || (
                    /* We have returned a node at a valid index */
                    data->node_idx <=
                        hdag_arr_slots_occupied(&data->bundle->nodes) &&
                    /* And the current target index for that node is valid */
                    data->target_idx <=
                        hdag_bundle_targets_count(data->bundle,
                                                  data->node_idx - 1)
                )
            ) &&
            hdag_node_iter_item_is_valid(&data->item)
        )
    );
}

/**
 * Validate a bundle's universal iterator data.
 *
 * @param data  The iterator data to validate.
 *
 * @return The validated iterator data.
 */
static inline struct hdag_bundle_iter_data*
hdag_bundle_iter_data_validate(struct hdag_bundle_iter_data *data)
{
    assert(hdag_bundle_iter_data_is_valid(data));
    return data;
}

/** A next-hash retrieval function for target hash iterator */
[[nodiscard]]
extern hdag_res hdag_bundle_targets_hash_iter_next(
                    const struct hdag_iter *iter,
                    void **pitem);

/**
 * Create a node's target hash iterator.
 *
 * @param data      The location for the private iterator data.
 * @param bundle    The bundle containing the node (NULL for a void iterator).
 * @param node_idx  The index of the node to return the target hashes for.
 */
static inline struct hdag_iter
hdag_bundle_targets_hash_iter(struct hdag_bundle_iter_data *data,
                              const struct hdag_bundle *bundle,
                              uint32_t node_idx)
{
    assert(data != NULL);
    assert(bundle == NULL || hdag_bundle_is_valid(bundle));
    assert(bundle == NULL ||
           node_idx <= hdag_arr_slots_occupied(&bundle->nodes));
    *data = (struct hdag_bundle_iter_data){
        .bundle = bundle,
        .with_unknown = true,
        .node_idx = node_idx,
    };
    return hdag_iter(
        (bundle == NULL
            ? hdag_iter_empty_next
            : hdag_bundle_targets_hash_iter_next),
        NULL,
        (bundle == NULL
            ? HDAG_TYPE_VOID
            : HDAG_TYPE_ARR(HDAG_TYPE_ID_UINT8, bundle->hash_len)),
        false,
        hdag_bundle_iter_data_validate(data)
    );
}

/** A next-node retrieval function for bundle's node iterator */
[[nodiscard]]
extern hdag_res hdag_bundle_node_iter_next(const struct hdag_iter *iter,
                                           void **pitem);
/** A next-target hash retrieval function for bundle's node iterator */
[[nodiscard]]
extern hdag_res hdag_bundle_node_iter_targets_hash_iter_next(
                                            const struct hdag_iter *iter,
                                            void **pitem);

/**
 * Create a bundle's node iterator
 *
 * @param data          The location for the private iterator data.
 * @param bundle        The bundle to initialize the iterator for.
 *                      NULL to create a void iterator.
 * @param with_unknown  If true, return unknown nodes too.
 *
 * @return The created iterator.
 */
static inline struct hdag_iter
hdag_bundle_node_iter(struct hdag_bundle_iter_data *data,
                      const struct hdag_bundle *bundle,
                      bool with_unknown)
{
    assert(data != NULL);
    assert(bundle == NULL || hdag_bundle_is_valid(bundle));
    *data = (struct hdag_bundle_iter_data){
        .bundle = bundle,
        .with_unknown = with_unknown,
        .item = {
            .target_hash_iter = hdag_iter(
                (bundle == NULL
                    ? hdag_iter_empty_next
                    : hdag_bundle_node_iter_targets_hash_iter_next),
                NULL,
                (bundle == NULL
                    ? HDAG_TYPE_VOID
                    : HDAG_TYPE_ARR(HDAG_TYPE_ID_UINT8, bundle->hash_len)),
                false,
                NULL
            )
        }
    };
    return hdag_iter(
        (bundle == NULL
            ? hdag_iter_empty_next
            : hdag_bundle_node_iter_next),
        NULL,
        (bundle == NULL
            ? HDAG_TYPE_VOID
            : HDAG_NODE_ITER_ITEM_TYPE(bundle->hash_len)
        ),
        false,
        hdag_bundle_iter_data_validate(data)
    );
}

/** A next-hash retrieval function for node hash iterator */
[[nodiscard]]
extern hdag_res hdag_bundle_node_hash_iter_next(const struct hdag_iter *iter,
                                                void **pitem);

/**
 * Create a bundle's node hash iterator
 *
 * @param bundle        The bundle to initialize the iterator for.
 * @param with_unknown  If true, return hashes of unknown nodes too.
 */
static inline struct hdag_iter
hdag_bundle_node_hash_iter(struct hdag_bundle_iter_data *data,
                           const struct hdag_bundle *bundle,
                           bool with_unknown)
{
    assert(data != NULL);
    assert(bundle == NULL || hdag_bundle_is_valid(bundle));
    *data = (struct hdag_bundle_iter_data){
        .bundle = bundle,
        .with_unknown = with_unknown,
    };
    return hdag_iter(
        (bundle == NULL
            ? hdag_iter_empty_next
            : hdag_bundle_node_hash_iter_next),
        NULL,
        (bundle == NULL
            ? HDAG_TYPE_VOID
            : HDAG_TYPE_ARR(HDAG_TYPE_ID_UINT8, bundle->hash_len)),
        false,
        hdag_bundle_iter_data_validate(data)
    );
}

/**
 * Check if a bundle is "filed" - having its contents located in a
 * memory-mapped region, possibly backed by a file, instead of the heap.
 *
 * @param bundle    The bundle to check.
 *
 * @return True if the bunde is "filed", false otherwise.
 */
static inline bool
hdag_bundle_is_filed(const struct hdag_bundle *bundle)
{
    assert(hdag_bundle_is_valid(bundle));
    return hdag_file_is_open(&bundle->file);
}

/**
 * Check if a filed bundle is "backed" - that is mapped to a file.
 *
 * @param bundle    The bundle to check. Must be "filed".
 *
 * @return True if the bunde is "backed", false otherwise.
 */
static inline bool
hdag_bundle_is_backed(const struct hdag_bundle *bundle)
{
    assert(hdag_bundle_is_valid(bundle));
    assert(hdag_bundle_is_filed(bundle));
    return hdag_file_is_backed(&bundle->file);
}

/**
 * Create a bundle from an opened HDAG file, taking ownership over it and
 * linking its contents in.
 *
 * @param pbundle   The location for the created bundle with the file
 *                  embedded. Can be NULL to not have the bundle output, and
 *                  to have it destroyed after creation instead.
 * @param file      The file to embed and link into the bundle.
 *                  Must be open. Will be closed.
 */
extern void hdag_bundle_from_file(struct hdag_bundle *pbundle,
                                  struct hdag_file *file);

/**
 * Detach and close the ("filed") bundle's embedded and linked file,
 * copying its contents to the bundle.
 *
 * @param bundle    The bundle to "unfile". Not modified in case of failure.
 *                  Must be "filed".
 *
 * @return A void universal result.
 */
[[nodiscard]]
extern hdag_res hdag_bundle_unfile(struct hdag_bundle *bundle);

/**
 * Create a hash DAG file from a bundle.
 *
 * @param pfile             Location for the created and opened file.
 *                          Not modified in case of failure.
 *                          Can be NULL to have the file closed after
 *                          creation.
 * @param pathname          The file's pathname (template), or NULL to
 *                          open an in-memory file.
 * @param template_sfxlen   The (non-negative) number of suffix characters
 *                          following the "XXXXXX" at the end of "pathname",
 *                          if it contains the template for a temporary file
 *                          to be created. Or a negative number to treat
 *                          "pathname" literally. Ignored, if "pathname" is
 *                          NULL.
 * @param open_mode         The mode bitmap to supply to open(2).
 *                          Ignored, if pathname is NULL.
 * @param bundle            The bundle to create the file from.
 *                          Must be organized.
 *
 * @return A void universal result.
 */
[[nodiscard]]
extern hdag_res hdag_bundle_to_file(struct hdag_file *pfile,
                                    const char *pathname,
                                    int template_sfxlen,
                                    mode_t open_mode,
                                    const struct hdag_bundle *bundle);

/**
 * Move bundle contents to a new hash DAG file, embed it and link its contents
 * into the bundle.
 *
 * @param bundle            The bundle to "file". Must be organized.
 * @param pathname          The file's pathname (template), or NULL to
 *                          open an in-memory file.
 * @param template_sfxlen   The (non-negative) number of suffix characters
 *                          following the "XXXXXX" at the end of "pathname",
 *                          if it contains the template for a temporary file
 *                          to be created. Or a negative number to treat
 *                          "pathname" literally. Ignored, if "pathname" is
 *                          NULL.
 * @param open_mode         The mode bitmap to supply to open(2).
 *                          Ignored, if pathname is NULL.
 *
 * @return A void universal result.
 */
[[nodiscard]]
extern hdag_res hdag_bundle_file(struct hdag_bundle *bundle,
                                 const char *pathname,
                                 int template_sfxlen,
                                 mode_t open_mode);

/**
 * Change the name of the ("backed") bundle's embedded and linked file.
 *
 * @param bundle    The bundle to rename the file of.
 *                  Not modified in case of failure. Must be "backed".
 * @param pathname  The pathname to rename the file to.
 *                  If NULL, the bundle's file is removed instead.
 *
 * @return A void universal result.
 */
[[nodiscard]]
static inline hdag_res
hdag_bundle_rename(struct hdag_bundle *bundle, const char *pathname)
{
    assert(hdag_bundle_is_valid(bundle));
    assert(hdag_bundle_is_filed(bundle));
    assert(hdag_bundle_is_backed(bundle));
    return hdag_file_rename(&bundle->file, pathname);
}

/**
 * Unlink (remove) the ("backed") bundle's embedded and linked file.
 * The bundle stops being "backed" as a result.
 *
 * @param bundle    The bundle to unlink the file of.
 *                  Not modified in case of failure. Must be "backed".
 *
 * @return A void universal result.
 */
[[nodiscard]]
static inline hdag_res
hdag_bundle_unlink(struct hdag_bundle *bundle)
{
    assert(hdag_bundle_is_valid(bundle));
    assert(hdag_bundle_is_filed(bundle));
    assert(hdag_bundle_is_backed(bundle));
    return hdag_file_unlink(&bundle->file);
}

#endif /* _HDAG_BUNDLE_H */
