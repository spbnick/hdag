/*
 * Hash DAG bundle - a file in the making
 */

#include <hdag/bundle.h>
#include <hdag/nodes.h>
#include <hdag/hash.h>
#include <hdag/misc.h>
#include <hdag/res.h>
#include <cgraph.h>
#include <string.h>
#include <ctype.h>

void
hdag_bundle_cleanup(struct hdag_bundle *bundle)
{
    assert(hdag_bundle_is_valid(bundle));
    hdag_darr_cleanup(&bundle->nodes);
    hdag_fanout_empty(bundle->nodes_fanout,
                      HDAG_ARR_LEN(bundle->nodes_fanout));
    hdag_darr_cleanup(&bundle->target_hashes);
    hdag_darr_cleanup(&bundle->extra_edges);
    assert(hdag_bundle_is_valid(bundle));
    assert(hdag_bundle_is_clean(bundle));
}

void
hdag_bundle_empty(struct hdag_bundle *bundle)
{
    assert(hdag_bundle_is_valid(bundle));
    hdag_darr_empty(&bundle->nodes);
    hdag_fanout_empty(bundle->nodes_fanout,
                      HDAG_ARR_LEN(bundle->nodes_fanout));
    hdag_darr_empty(&bundle->target_hashes);
    hdag_darr_empty(&bundle->extra_edges);
    assert(hdag_bundle_is_valid(bundle));
    assert(hdag_bundle_is_empty(bundle));
}

hdag_res
hdag_bundle_deflate(struct hdag_bundle *bundle)
{
    assert(hdag_bundle_is_valid(bundle));
    if (hdag_darr_deflate(&bundle->nodes) &&
        hdag_darr_deflate(&bundle->target_hashes) &&
        hdag_darr_deflate(&bundle->extra_edges)) {
        return HDAG_RES_OK;
    }
    return HDAG_RES_ERRNO;
}

void
hdag_bundle_sort(struct hdag_bundle *bundle)
{
    /* Currently traversed node */
    struct hdag_node *node;
    /* The index of the currently-traversed node */
    ssize_t idx;

    assert(hdag_bundle_is_valid(bundle));
    assert(!hdag_bundle_has_index_targets(bundle));
    /* Sort the nodes by hash lexicographically */
    hdag_darr_qsort_all(&bundle->nodes, hdag_node_cmp, &bundle->hash_len);
    /* Sort the target hashes for each node lexicographically */
    HDAG_DARR_ITER_FORWARD(&bundle->nodes, idx, node, (void)0, (void)0) {
        if (hdag_targets_are_indirect(&node->targets)) {
            hdag_darr_qsort(&bundle->target_hashes,
                            hdag_node_get_first_ind_idx(node),
                            hdag_node_get_last_ind_idx(node) + 1,
                            hdag_hash_cmp, &bundle->hash_len);
        }
    }

    assert(hdag_bundle_is_valid(bundle));
    assert(hdag_bundle_is_sorted(bundle));
}

void
hdag_bundle_fanout_fill(struct hdag_bundle *bundle)
{
    /* Position in the fanout array == first byte of node hash */
    size_t pos;
    /* Currently traversed node */
    struct hdag_node *node;
    /* The index of the currently-traversed node */
    ssize_t idx;

    assert(hdag_bundle_is_valid(bundle));
    assert(hdag_bundle_is_sorted(bundle));
    assert(hdag_bundle_fanout_is_empty(bundle));

    HDAG_DARR_ITER_FORWARD(&bundle->nodes, idx, node, pos = 0, (void)0) {
        for (; pos < node->hash[0]; pos++) {
            bundle->nodes_fanout[pos] = idx;
        }
    }
    for (; pos < HDAG_ARR_LEN(bundle->nodes_fanout); pos++) {
        bundle->nodes_fanout[pos] = idx;
    }

    assert(hdag_fanout_is_valid(bundle->nodes_fanout,
                                HDAG_ARR_LEN(bundle->nodes_fanout)));
    assert(idx == 0 || !hdag_bundle_fanout_is_empty(bundle));
}

/*
 * Check if both the nodes and targets of a bundle are sorted according to
 * a specified comparison operator. "return" true if they are, false if not.
 *
 * @param _bundle   The bundle to sort.
 * @param _cmp_op   The operator to use to compare two adjacent nodes/edges.
 *                  The lower-index one on the left, the higher-index one on
 *                  the right.
 */
#define HDAG_BUNDLE_IS_SORTED_AS(_bundle, _cmp_op) \
    do {                                                                    \
        ssize_t node_idx;                                                   \
        const struct hdag_node *prev_node;                                  \
        const struct hdag_node *node;                                       \
        size_t target_idx;                                                  \
        int64_t relation;                                                   \
                                                                            \
        assert(hdag_bundle_is_valid((_bundle)));                            \
                                                                            \
        HDAG_DARR_ITER_FORWARD(&(_bundle)->nodes, node_idx, node,           \
                               prev_node = NULL, prev_node=node) {          \
            if (prev_node != NULL &&                                        \
                !(memcmp(prev_node->hash, node->hash,                       \
                         (_bundle)->hash_len) _cmp_op 0)) {                 \
                return false;                                               \
            }                                                               \
            if (!hdag_targets_are_indirect(&node->targets)) {               \
                continue;                                                   \
            }                                                               \
            for (target_idx = hdag_node_get_first_ind_idx(node) + 1;        \
                 target_idx <= hdag_node_get_last_ind_idx(node);            \
                 target_idx++) {                                            \
                if (hdag_darr_occupied_slots(&bundle->extra_edges) != 0) {  \
                    relation =                                              \
                        (int64_t)(HDAG_DARR_ELEMENT(                        \
                            &(_bundle)->extra_edges,                        \
                            struct hdag_edge, target_idx - 1                \
                        )->node_idx) -                                      \
                        (int64_t)(HDAG_DARR_ELEMENT(                        \
                            &(_bundle)->extra_edges,                        \
                            struct hdag_edge, target_idx                    \
                        )->node_idx);                                       \
                } else {                                                    \
                    relation = memcmp(                                      \
                        hdag_darr_element_const(                            \
                            &(_bundle)->target_hashes, target_idx - 1       \
                        ),                                                  \
                        hdag_darr_element_const(                            \
                            &(_bundle)->target_hashes, target_idx           \
                        ),                                                  \
                        (_bundle)->target_hashes.slot_size                  \
                    );                                                      \
                }                                                           \
                if (!(relation _cmp_op 0)) {                                \
                    return false;                                           \
                }                                                           \
            }                                                               \
        }                                                                   \
                                                                            \
        return true;                                                        \
    } while (0)

bool
hdag_bundle_is_sorted(const struct hdag_bundle *bundle)
{
    HDAG_BUNDLE_IS_SORTED_AS(bundle, <=);
}

bool
hdag_bundle_is_sorted_and_deduped(const struct hdag_bundle *bundle)
{
    HDAG_BUNDLE_IS_SORTED_AS(bundle, <);
}

hdag_res
hdag_bundle_dedup(struct hdag_bundle *bundle)
{
    hdag_res res = HDAG_RES_INVALID;
    assert(hdag_bundle_is_valid(bundle));
    assert(hdag_bundle_is_sorted(bundle));
    assert(!hdag_bundle_has_index_targets(bundle));

    /* The size of each node */
    const size_t node_size = bundle->nodes.slot_size;
    /* The size of each target_hash */
    const size_t hash_size = bundle->target_hashes.slot_size;
    /* Previously traversed node */
    struct hdag_node *prev_node;
    /* Currently traversed node */
    struct hdag_node *node;
    /* Current output node */
    struct hdag_node *out_node;
    /* The node to keep out of all the nodes in the run */
    struct hdag_node *keep_node;
    /* Number of nodes */
    size_t node_num = bundle->nodes.slots_occupied;
    /* The end node of the node array */
    const struct hdag_node *end_node =
        hdag_darr_slot(&bundle->nodes, node_num);;

    /* New target hash array */
    struct hdag_darr target_hashes = HDAG_DARR_EMPTY(
        bundle->target_hashes.slot_size,
        bundle->target_hashes.slots_preallocate
    );
    /* The index of the node's first output hash */
    size_t first_out_hash_idx;
    /* Location of the node's original last hash */
    uint8_t *last_hash;
    /* Currently-traversed hash */
    uint8_t *hash;
    /* The hash before the current one */
    uint8_t *prev_hash;

    /* Remove duplicate nodes if we have more than two */
    if (node_num >= 2) {
        prev_node = NULL;
        node = bundle->nodes.slots;
        out_node = node;
        keep_node = NULL;
        while (true) {
            prev_node = node;
            node = (struct hdag_node *)((uint8_t *)node + node_size);
            /* Keep last known node */
            if (hdag_targets_are_known(&prev_node->targets)) {
                keep_node = prev_node;
            }
            /* If we're out of nodes */
            if (node >= end_node) {
                goto run_end;
            }
            /* If nodes have the same hash (we're still inside the run) */
            if (memcmp(node->hash, prev_node->hash, bundle->hash_len) == 0) {
                continue;
            }
            /* End of a run */
run_end:
            /* If we found no known nodes in the finished run */
            if (keep_node == NULL) {
                /* Keep the last unknown node */
                keep_node = prev_node;
            }
            if (out_node < keep_node) {
                memcpy(out_node, keep_node, node_size);
            }
            out_node = (struct hdag_node *)((uint8_t *)out_node + node_size);
            keep_node = NULL;
            /* If we're out of nodes */
            if (node >= end_node) {
                break;
            }
        }
        /* Truncate the nodes */
        end_node = out_node;
        node_num = ((uint8_t *)end_node - (uint8_t *)bundle->nodes.slots) /
                   node_size;
        bundle->nodes.slots_occupied = node_num;
    }

    /*
     * Remove duplicate edges
     */
    assert(hdag_darr_occupied_slots(&bundle->extra_edges) == 0);
    /* For each (deduplicated) node */
    for (node = bundle->nodes.slots;
         node < end_node;
         node = (struct hdag_node *)((uint8_t *)node + node_size)) {
        if (!hdag_targets_are_indirect(&node->targets)) {
            continue;
        }
        first_out_hash_idx = target_hashes.slots_occupied;
        last_hash = hdag_darr_element(&bundle->target_hashes,
                                      hdag_node_get_last_ind_idx(node));
        /* For each target hash starting with the second one */
        for (
            prev_hash = hdag_darr_element(
                &bundle->target_hashes, hdag_node_get_first_ind_idx(node)
            ),
            hash = prev_hash + hash_size;
            hash <= last_hash;
            prev_hash = hash, hash += hash_size
        ) {
            /* If the current hash is not equal to the previous one */
            if (memcmp(hash, prev_hash, hash_size) != 0) {
                /* Output the previous hash */
                if (hdag_darr_append_one(&target_hashes, prev_hash) == NULL) {
                    goto cleanup;
                }
            }
        }
        /* Output the last hash */
        if (hdag_darr_append_one(&target_hashes, prev_hash) == NULL) {
            goto cleanup;
        }
        /* Store the new targets */
        node->targets = HDAG_TARGETS_INDIRECT(
            first_out_hash_idx,
            target_hashes.slots_occupied - 1
        );
    }
    /* Replace the hashes */
    hdag_darr_cleanup(&bundle->target_hashes);
    bundle->target_hashes = target_hashes;
    target_hashes = HDAG_DARR_EMPTY(
        bundle->target_hashes.slot_size,
        bundle->target_hashes.slots_preallocate
    );

    assert(hdag_bundle_is_valid(bundle));
    assert(hdag_bundle_is_sorted_and_deduped(bundle));
    res = HDAG_RES_OK;

cleanup:
    hdag_darr_cleanup(&target_hashes);
    return HDAG_RES_ERRNO_IF_INVALID(res);
}

void
hdag_bundle_compact(struct hdag_bundle *bundle)
{
    /* The index of the currently-traversed node */
    ssize_t idx;
    /* A hash index */
    size_t hash_idx;
    /* The index of a found node */
    size_t found_idx;
    /* Currently traversed node */
    struct hdag_node *node;
    /* The new extra_edges array */
    struct hdag_darr extra_edges =
        HDAG_DARR_EMPTY(sizeof(struct hdag_edge), 64);

    assert(hdag_bundle_is_valid(bundle));
    assert(hdag_bundle_is_sorted_and_deduped(bundle));
    assert(hdag_darr_occupied_slots(&bundle->nodes) == 0 ||
           !hdag_bundle_fanout_is_empty(bundle));
    assert(!hdag_bundle_has_index_targets(bundle));
    assert(hdag_darr_occupied_slots(&bundle->extra_edges) == 0);

    /* For each node, from start to end */
    HDAG_DARR_ITER_FORWARD(&bundle->nodes, idx, node, (void)0, (void)0) {
        assert(hdag_node_is_valid(node));
        /* If the node's targets are unknown or are both absent */
        if (hdag_targets_are_unknown(&node->targets) ||
            hdag_targets_are_absent(&node->targets)) {
            continue;
        }
        /* If there's more than two targets */
        if (node->targets.last - node->targets.first > 1) {
            size_t first_extra_edge_idx = extra_edges.slots_occupied;

            /* Convert all target hashes to extra edges */
            for (hash_idx = hdag_target_to_ind_idx(node->targets.first);
                 hash_idx <= hdag_target_to_ind_idx(node->targets.last);
                 hash_idx++) {
                found_idx = hdag_bundle_find_node_idx(
                    bundle,
                    hdag_darr_element(&bundle->target_hashes, hash_idx)
                );
                /* All hashes must be locatable */
                assert(found_idx < INT32_MAX);
                /* Hash indices must be valid */
                assert(found_idx < bundle->nodes.slots_occupied);
                /* Store the edge */
                ((struct hdag_edge *)hdag_darr_cappend_one(
                    &extra_edges
                ))->node_idx = found_idx;
            }

            /* Store the extra edge indices */
            node->targets.first =
                hdag_target_from_ind_idx(first_extra_edge_idx);
            node->targets.last =
                hdag_target_from_ind_idx(extra_edges.slots_occupied - 1);
        } else {
            /* If there are two targets */
            if (node->targets.last > node->targets.first) {
                /* Store the second target inside the node */
                hash_idx = hdag_target_to_ind_idx(node->targets.last);
                found_idx = hdag_bundle_find_node_idx(
                    bundle,
                    hdag_darr_element(&bundle->target_hashes, hash_idx)
                );
                /* All hashes must be locatable */
                assert(found_idx < INT32_MAX);
                /* Hash indices must be valid */
                assert(found_idx < bundle->nodes.slots_occupied);
                /* Store the last target */
                node->targets.last = hdag_target_from_dir_idx(found_idx);
            } else {
                /* Mark second target absent */
                node->targets.last = HDAG_TARGET_ABSENT;
            }

            /* Store first target inside the node */
            hash_idx = hdag_target_to_ind_idx(node->targets.first);
            found_idx = hdag_bundle_find_node_idx(
                bundle,
                hdag_darr_element(&bundle->target_hashes, hash_idx)
            );
            /* All hashes must be locatable */
            assert(found_idx < INT32_MAX);
            /* Hash indices must be valid */
            assert(found_idx < bundle->nodes.slots_occupied);
            /* Store the first target */
            node->targets.first = hdag_target_from_dir_idx(found_idx);
        }
        assert(hdag_node_is_valid(node));
    }

    /* Remove target hashes */
    hdag_darr_cleanup(&bundle->target_hashes);
    /* Move extra edges */
    hdag_darr_cleanup(&bundle->extra_edges);
    bundle->extra_edges = extra_edges;

    assert(hdag_bundle_is_valid(bundle));
    assert(hdag_bundle_is_compacted(bundle));
}

hdag_res
hdag_bundle_invert(struct hdag_bundle *pinverted,
                   const struct hdag_bundle *original)
{
    assert(hdag_bundle_is_valid(original));
    assert(hdag_bundle_is_sorted_and_deduped(original));
    assert(!hdag_bundle_has_hash_targets(original));

    hdag_res                res = HDAG_RES_INVALID;
    struct hdag_bundle      inverted = HDAG_BUNDLE_EMPTY(original->hash_len);
    ssize_t                 node_idx;
    const struct hdag_node *original_node;
    struct hdag_node       *inverted_node;
    uint32_t                target_count;
    uint32_t                target_idx;

    if (hdag_darr_occupied_slots(&original->nodes) == 0) {
        goto output;
    }

    if (!hdag_darr_append(&inverted.nodes, original->nodes.slots,
                          hdag_darr_occupied_slots(&original->nodes))) {
        goto cleanup;
    }

    /* Put the number of node's eventual targets into the "generation" */
    HDAG_DARR_ITER_FORWARD(&inverted.nodes, node_idx, inverted_node,
                           (void)0, (void)0) {
        inverted_node->generation = 0;
    }
    HDAG_DARR_ITER_FORWARD(&original->nodes, node_idx, original_node,
                           (void)0, (void)0) {
        target_count = hdag_node_targets_count(original_node);
        for (target_idx = 0; target_idx < target_count; target_idx++) {
            hdag_bundle_node(
                &inverted,
                hdag_bundle_targets_node_idx(original,
                                             (uint32_t)node_idx, target_idx)
            )->generation++;
        }
    }
    /*
     * Set targets to absent for nodes with <= 2 targets
     * Assign indirect index ranges to nodes with > 2 targets
     * Allocate space for all indirect target edges
     */
    target_idx = 0;
    HDAG_DARR_ITER_FORWARD(&inverted.nodes, node_idx, inverted_node,
                           (void)0, (void)0) {
        inverted_node->targets = (inverted_node->generation <= 2)
            ? HDAG_TARGETS_ABSENT
            : HDAG_TARGETS_INDIRECT(
                target_idx,
                (target_idx += inverted_node->generation) - 1
            );
    }
    if (target_idx > 0) {
        if (!hdag_darr_uappend(&inverted.extra_edges, target_idx)) {
            goto cleanup;
        }
    }

    /* Assign all the targets */
    HDAG_DARR_ITER_FORWARD(&original->nodes, node_idx, original_node,
                           (void)0, (void)0) {
        target_count = hdag_node_targets_count(original_node);
        for (target_idx = 0; target_idx < target_count; target_idx++) {
            /* Get the target inverted node */
            inverted_node = hdag_bundle_node(
                &inverted,
                hdag_bundle_targets_node_idx(original,
                                             (uint32_t)node_idx, target_idx)
            );
            /* Make sure the inverted node has space for another target */
            assert(inverted_node->generation > 0);
            /* Decrement the remaining target count (and help get index) */
            inverted_node->generation--;
            /* If the inverted node is supposed to have <= 2 targets */
            if (inverted_node->targets.last == HDAG_TARGET_ABSENT) {
                /* If we're assigning the first target */
                if (inverted_node->targets.first == HDAG_TARGET_ABSENT) {
                    inverted_node->targets.first =
                        hdag_target_from_dir_idx(node_idx);
                /* Else, we're assigning the second (last) target */
                } else {
                    inverted_node->targets.last =
                        hdag_target_from_dir_idx(node_idx);
                }
            /* Else it's supposed to have > 2 targets, via extra edges */
            } else {
                /* Assign the next target */
                HDAG_DARR_ELEMENT(
                    &inverted.extra_edges,
                    struct hdag_edge,
                    hdag_target_to_ind_idx(inverted_node->targets.last) -
                    inverted_node->generation
                )->node_idx = node_idx;
            }
        }
    }

output:
    assert(hdag_bundle_is_valid(&inverted));
    assert(hdag_bundle_is_sorted_and_deduped(&inverted));
    assert(!hdag_bundle_has_hash_targets(&inverted));
    assert(!hdag_bundle_some_nodes_have_generations(&inverted));

    if (pinverted != NULL) {
        *pinverted = inverted;
        inverted = HDAG_BUNDLE_EMPTY(inverted.hash_len);
    }
    res = HDAG_RES_OK;

cleanup:
    hdag_bundle_cleanup(&inverted);
    return HDAG_RES_ERRNO_IF_INVALID(res);
}

hdag_res
hdag_bundle_generations_enumerate(struct hdag_bundle *bundle)
{
    ssize_t                 idx;
    struct hdag_node       *node;
    ssize_t                 dfs_idx;
    struct hdag_node       *dfs_node;
    ssize_t                 next_dfs_idx;
    struct hdag_node       *next_dfs_node;
    uint32_t                target_idx;

    assert(hdag_bundle_is_valid(bundle));
    assert(hdag_bundle_is_sorted_and_deduped(bundle));
    assert(hdag_bundle_is_compacted(bundle));
    assert(!hdag_bundle_some_nodes_have_generations(bundle));

#define NODE_HAS_PARENT(_node) \
    ((_node)->component >= (uint32_t)INT32_MAX)
#define NODE_GET_PARENT(_node) \
    ((_node)->component - (uint32_t)INT32_MAX)
#define NODE_SET_PARENT(_node, _idx) \
    ((_node)->component = (uint32_t)INT32_MAX + _idx)
#define NODE_REMOVE_PARENT(_node) \
    ((_node)->component = 0)

#define NODE_HAS_NEXT_TARGET(_node) \
    ((_node)->generation >= (uint32_t)INT32_MAX)

#define NODE_GET_NEXT_TARGET(_node) \
    ((_node)->generation - (uint32_t)INT32_MAX)
#define NODE_SET_NEXT_TARGET(_node, _idx) \
    ((_node)->generation = (uint32_t)INT32_MAX + _idx)
#define NODE_INC_NEXT_TARGET(_node) \
    ((_node)->generation++)

#define NODE_HAS_GENERATION(_node) \
    ((_node)->generation < (uint32_t)INT32_MAX && (_node)->generation != 0)

#define NODE_IS_BEING_TRAVERSED(_node) NODE_HAS_NEXT_TARGET(_node)
#define NODE_HAS_BEEN_TRAVERSED(_node) NODE_HAS_GENERATION(_node)

    /* For each node in the graph */
    HDAG_DARR_ITER_FORWARD(&bundle->nodes, idx, node, (void)0, (void)0) {
        /* Do a DFS */
        dfs_idx = idx;
        dfs_node = node;
        while (true) {
            /* If this node has already been traversed */
            if (NODE_HAS_BEEN_TRAVERSED(dfs_node)) {
                /* If the node has a parent */
                if (NODE_HAS_PARENT(dfs_node)) {
                    /* Go back up to the parent, removing it from the node */
                    dfs_idx = NODE_GET_PARENT(dfs_node);
                    NODE_REMOVE_PARENT(dfs_node);
                    dfs_node = hdag_bundle_node(bundle, dfs_idx);
                } else {
                    /* Go to the next DFS */
                    break;
                }
            }
            /* If this node hasn't been traversed before */
            if (!NODE_HAS_NEXT_TARGET(dfs_node)) {
                /* Start with its first target, if any */
                NODE_SET_NEXT_TARGET(dfs_node, 0);
            }
            /* If the node has more targets to traverse */
            if (NODE_GET_NEXT_TARGET(dfs_node) <
                    hdag_node_targets_count(dfs_node)) {
                /* Get the next target node down */
                next_dfs_idx = hdag_bundle_targets_node_idx(
                    bundle, dfs_idx, NODE_GET_NEXT_TARGET(dfs_node)
                );
                next_dfs_node = hdag_bundle_node(bundle, next_dfs_idx);
                /* If the next (target) node is being traversed */
                if (NODE_IS_BEING_TRAVERSED(next_dfs_node)) {
                    /* We found a cycle */
                    return HDAG_RES_GRAPH_CYCLE;
                }
                NODE_INC_NEXT_TARGET(dfs_node);
                /* Set its parent to the current node */
                NODE_SET_PARENT(next_dfs_node, dfs_idx);
                /* Go to the target node */
                dfs_node = next_dfs_node;
                dfs_idx = next_dfs_idx;
            } else {
                /* Assign the maximum target generation + 1 */
                dfs_node->generation = 0;
                for (target_idx = 0;
                     target_idx < hdag_node_targets_count(dfs_node);
                     target_idx++) {
                    uint32_t target_generation = hdag_bundle_targets_node(
                        bundle, dfs_idx, target_idx
                    )->generation;
                    if (target_generation > dfs_node->generation) {
                        dfs_node->generation = target_generation;
                    }
                }
                dfs_node->generation++;
                assert(NODE_HAS_BEEN_TRAVERSED(dfs_node));
            }
        }
    }

#undef NODE_HAS_BEEN_TRAVERSED
#undef NODE_IS_BEING_TRAVERSED
#undef NODE_HAS_GENERATION
#undef NODE_INC_NEXT_TARGET
#undef NODE_SET_NEXT_TARGET
#undef NODE_GET_NEXT_TARGET
#undef NODE_HAS_NEXT_TARGET
#undef NODE_REMOVE_PARENT
#undef NODE_SET_PARENT
#undef NODE_GET_PARENT
#undef NODE_HAS_PARENT

    assert(hdag_bundle_all_nodes_have_generations(bundle));
    assert(!hdag_bundle_some_nodes_have_components(bundle));
    return HDAG_RES_OK;
}

hdag_res
hdag_bundle_components_enumerate(struct hdag_bundle *bundle)
{
    hdag_res                res = HDAG_RES_INVALID;
    struct hdag_bundle      inverted = HDAG_BUNDLE_EMPTY(bundle->hash_len);
    struct hdag_bundle     *inv;
    struct hdag_bundle     *orig;
    ssize_t                 idx;
    struct hdag_node       *inv_node;
    ssize_t                 dfs_idx;
    struct hdag_node       *dfs_inv_node;
    struct hdag_node       *dfs_orig_node;
    ssize_t                 next_dfs_idx;
    struct hdag_node       *next_dfs_inv_node;
    uint32_t                target_idx;
    uint32_t                orig_target_num;
    uint32_t                inv_target_num;
    uint32_t                component = 0;

    assert(hdag_bundle_is_valid(bundle));
    assert(hdag_bundle_is_sorted_and_deduped(bundle));
    assert(hdag_bundle_is_compacted(bundle));
    assert(!hdag_bundle_some_nodes_have_components(bundle));

    /* Invert the graph */
    HDAG_RES_TRY(hdag_bundle_invert(&inverted, bundle));

    /* Use short alias pointers for uniformity */
    orig = bundle;
    inv = &inverted;

    /*
     * NOTE: All the below markets are for inverted nodes
     */
#define NODE_HAS_PARENT(_node) \
    ((_node)->component >= (uint32_t)INT32_MAX)
#define NODE_GET_PARENT(_node) \
    ((_node)->component - (uint32_t)INT32_MAX)
#define NODE_SET_PARENT(_node, _idx) \
    ((_node)->component = (uint32_t)INT32_MAX + _idx)
#define NODE_REMOVE_PARENT(_node) \
    ((_node)->component = 0)

#define NODE_HAS_NEXT_TARGET(_node) \
    ((_node)->generation >= (uint32_t)INT32_MAX)

#define NODE_GET_NEXT_TARGET(_node) \
    ((_node)->generation - (uint32_t)INT32_MAX)
#define NODE_SET_NEXT_TARGET(_node, _idx) \
    ((_node)->generation = (uint32_t)INT32_MAX + _idx)
#define NODE_INC_NEXT_TARGET(_node) \
    ((_node)->generation++)

    /* For each node in the (inverted) graph */
    HDAG_DARR_ITER_FORWARD(&inv->nodes, idx, inv_node, (void)0, (void)0) {
        /* If this node has been traversed (and got a component) */
        if (NODE_HAS_NEXT_TARGET(inv_node)) {
            /* Move on to the next node */
            continue;
        }
        /* Do a DFS ignoring edge direction */
        dfs_idx = idx;
        dfs_inv_node = inv_node;
        dfs_orig_node = hdag_bundle_node(orig, dfs_idx);
        NODE_SET_NEXT_TARGET(dfs_inv_node, 0);
        /* Start a new component */
        component++;
        while (true) {
            orig_target_num = hdag_node_targets_count(dfs_orig_node);
            inv_target_num = hdag_node_targets_count(dfs_inv_node);
            target_idx = NODE_GET_NEXT_TARGET(dfs_inv_node);
            /* If the node has more original targets to traverse */
            if (target_idx < orig_target_num) {
                /* Get the index of the next (original) target node down */
                next_dfs_idx = hdag_bundle_targets_node_idx(
                    orig, dfs_idx, target_idx
                );
            /* Else, if it has more inverted targets to traverse */
            } else if ((target_idx -= orig_target_num) < inv_target_num) {
                /* Get the index of the next (inverted) target node down */
                next_dfs_idx = hdag_bundle_targets_node_idx(
                    inv, dfs_idx, target_idx
                );
            /* Else it ran out of targets and we're done with it */
            } else {
                /* Assign the component */
                dfs_orig_node->component = component;
                /* If the node has a parent */
                if (NODE_HAS_PARENT(dfs_inv_node)) {
                    /* Go back up to the parent, removing it from the node */
                    dfs_idx = NODE_GET_PARENT(dfs_inv_node);
                    NODE_REMOVE_PARENT(dfs_inv_node);
                    dfs_inv_node = hdag_bundle_node(inv, dfs_idx);
                    dfs_orig_node = hdag_bundle_node(orig, dfs_idx);
                    assert(NODE_HAS_NEXT_TARGET(dfs_inv_node));
                    /* Continue with the parent */
                    continue;
                /* Else, it has no parent, and DFS is finished */
                } else {
                    /* Move onto the next DFS */
                    break;
                }
            }
            /* Get the next target */
            NODE_INC_NEXT_TARGET(dfs_inv_node);
            next_dfs_inv_node = hdag_bundle_node(inv, next_dfs_idx);
            /* If it's already traversed */
            if (NODE_HAS_NEXT_TARGET(next_dfs_inv_node)) {
                /* It must be no-component, or our-component */
                assert(
                    hdag_bundle_node(orig, dfs_idx)->component == 0 ||
                    hdag_bundle_node(orig, dfs_idx)->component == component
                );
                /* Try the next one */
                continue;
            }
            NODE_SET_NEXT_TARGET(next_dfs_inv_node, 0);
            /* Set the next target node parent to the current node */
            NODE_SET_PARENT(next_dfs_inv_node, dfs_idx);
            /* Go to the target node */
            dfs_idx = next_dfs_idx;
            dfs_inv_node = next_dfs_inv_node;
            dfs_orig_node = hdag_bundle_node(orig, dfs_idx);
        }
    }

#undef NODE_INC_NEXT_TARGET
#undef NODE_SET_NEXT_TARGET
#undef NODE_GET_NEXT_TARGET
#undef NODE_HAS_NEXT_TARGET
#undef NODE_REMOVE_PARENT
#undef NODE_SET_PARENT
#undef NODE_GET_PARENT
#undef NODE_HAS_PARENT

    assert(hdag_bundle_all_nodes_have_components(bundle));
    res = HDAG_RES_OK;
cleanup:
    hdag_bundle_cleanup(&inverted);
    return HDAG_RES_ERRNO_IF_INVALID(res);
}

hdag_res
hdag_bundle_node_seq_load(struct hdag_bundle *pbundle,
                          struct hdag_node_seq node_seq)
{
    hdag_res                res = HDAG_RES_INVALID;
    struct hdag_bundle      bundle = HDAG_BUNDLE_EMPTY(node_seq.hash_len);
    uint8_t                *node_hash = NULL;
    uint8_t                *target_hash = NULL;
    struct hdag_hash_seq    target_hash_seq;
    size_t                  first_target_hash_idx;

    assert(hdag_node_seq_is_valid(&node_seq));

    node_hash = malloc(bundle.hash_len);
    if (node_hash == NULL) {
        goto cleanup;
    }
    target_hash = malloc(bundle.hash_len);
    if (target_hash == NULL) {
        goto cleanup;
    }

    /* Add a new node */
    #define ADD_NODE(_hash, _targets) \
        do {                                                \
            struct hdag_node *_node = (struct hdag_node *)  \
                hdag_darr_cappend_one(&bundle.nodes);       \
            if (_node == NULL) {                            \
                goto cleanup;                               \
            }                                               \
            memcpy(_node->hash, _hash, bundle.hash_len);    \
            _node->targets = _targets;                      \
        } while (0)

    /* Collect each node (and its targets) in the sequence */
    while (!HDAG_RES_TRY(
        node_seq.next_fn(&node_seq, node_hash, &target_hash_seq)
    )) {
        first_target_hash_idx = bundle.target_hashes.slots_occupied;

        /* Collect each target hash */
        while (!HDAG_RES_TRY(
            target_hash_seq.next_fn(&target_hash_seq, target_hash)
        )) {
            /* Add a new target hash (and the corresponding node) */
            if (hdag_darr_append_one(
                    &bundle.target_hashes, target_hash) == NULL) {
                goto cleanup;
            }
            ADD_NODE(target_hash, HDAG_TARGETS_UNKNOWN);
        }

        /* Add the node */
        if (first_target_hash_idx == bundle.target_hashes.slots_occupied) {
            ADD_NODE(node_hash, HDAG_TARGETS_ABSENT);
        } else {
            ADD_NODE(
                node_hash,
                HDAG_TARGETS_INDIRECT(
                    first_target_hash_idx,
                    bundle.target_hashes.slots_occupied - 1
                )
            );
        }
    }

    #undef ADD_NODE

    assert(hdag_bundle_is_valid(&bundle));

    if (pbundle != NULL) {
        *pbundle = bundle;
        bundle = HDAG_BUNDLE_EMPTY(node_seq.hash_len);
    }

    res = HDAG_RES_OK;

cleanup:
    free(target_hash);
    free(node_hash);
    hdag_bundle_cleanup(&bundle);
    return HDAG_RES_ERRNO_IF_INVALID(res);
}

/** The state of loading node/target sequence from a text file */
struct hdag_bundle_txt_seq {
    /** The FILE stream containing the text to parse and load */
    FILE       *stream;
};

/**
 * Skip initial whitespace and read a hash of specified length from a stream.
 *
 * @param stream            The stream to read the hash from.
 * @param hash_buf          The buffer for the read hash (right-aligned).
 *                          Can be modified even on failure.
 * @param hash_len          The length of the hash to read,
 *                          as well as of its buffer, bytes.
 * @param skip_linebreaks   Include linebreaks into skipped initial
 *                          whitespace, if true. Assume there's no hash, if
 *                          false and an initial linebreak is encountered.
 *
 * @return  A non-negative number specifying how many hash bytes have been
 *          unfilled in the buffer, on success, or hash_len, if there was no
 *          hash.
 *          A negative number (a failure result) if hash retrieval has failed,
 *          including HDAG_RES_INVALID_FORMAT, if the file format is invalid,
 *          and HDAG_RES_ERRNO's in case of libc errors.
 */
static hdag_res
hdag_bundle_txt_read_hash(FILE *stream, uint8_t *hash_buf, uint16_t hash_len,
                          bool skip_linebreaks)
{
    uint16_t                    rem_hash_len = hash_len;
    uint8_t                    *hash_ptr = hash_buf;
    int                         c;
    bool                        high_nibble = true;
    uint8_t                     nibble;

    assert(stream != NULL);
    assert(hash_buf != NULL);
    assert(hdag_hash_len_is_valid(hash_len));

    /* Skip whitespace */
    do {
        c = getc(stream);
        if (c == EOF) {
            if (ferror(stream)) {
                return HDAG_RES_ERRNO;
            }
            goto output;
        }
        if (!skip_linebreaks && (c == '\r' || c == '\n')) {
            goto output;
        }
    } while (isspace(c));

    /* Read the node hash until whitespace */
    do {
        /* If this is an invalid character, or the hash is too long */
        if (!isxdigit(c) || rem_hash_len == 0) {
            return HDAG_RES_INVALID_FORMAT;
        }
        nibble = c >= 'A' ? ((c & ~0x20) - ('A' - 0xA)) : (c - '0');
        if (high_nibble) {
            *hash_ptr = nibble << 4;
            high_nibble = false;
        } else {
            *hash_ptr |= nibble;
            high_nibble = true;
            rem_hash_len--;
            hash_ptr++;
        }
        c = getc(stream);
        if (c == EOF) {
            if (ferror(stream)) {
                return HDAG_RES_ERRNO;
            }
            break;
        }
    } while (!isspace(c));

    /* If we didn't get a low nibble (got odd number of digits) */
    if (!high_nibble) {
        return HDAG_RES_INVALID_FORMAT;
    }

output:
    /* Move the hash to the right */
    memmove(hash_buf + rem_hash_len, hash_buf, hash_len - rem_hash_len);
    /* Fill initial missing hash bytes with zeroes */
    memset(hash_buf, 0, rem_hash_len);

    /* Put back the terminating whitespace */
    assert(c == EOF || isspace(c));
    if (c != EOF && ungetc(c, stream) == EOF) {
        return HDAG_RES_ERRNO;
    }

    return rem_hash_len;
}

/**
 * Return the next target hash from an adjacency list text file.
 */
static hdag_res
hdag_bundle_txt_hash_seq_next(const struct hdag_hash_seq *hash_seq,
                              uint8_t *phash)
{
    hdag_res                    res;
    uint16_t                    hash_len = hash_seq->hash_len;
    struct hdag_bundle_txt_seq *txt_seq = hash_seq->data;
    FILE                       *stream = txt_seq->stream;

    assert(hdag_hash_len_is_valid(hash_seq->hash_len));
    assert(txt_seq != NULL);
    assert(txt_seq->stream != NULL);

    /* Skip whitespace (excluding newlines) and read a hash */
    res = hdag_bundle_txt_read_hash(stream, phash, hash_len, false);
    /* If we failed reading */
    if (hdag_res_is_failure(res)) {
        return res;
    }
    /* Return OK if we got a hash */
    return res >= hash_len ? 1 : HDAG_RES_OK;
}

/**
 * Return the next node from an adjacency list text file.
 */
static hdag_res
hdag_bundle_txt_node_seq_next(const struct hdag_node_seq *node_seq,
                              uint8_t                    *phash,
                              struct hdag_hash_seq       *ptarget_hash_seq)
{
    hdag_res                    res;
    uint16_t                    hash_len = node_seq->hash_len;
    struct hdag_bundle_txt_seq *txt_seq = node_seq->data;
    FILE                       *stream = txt_seq->stream;

    assert(hdag_hash_len_is_valid(node_seq->hash_len));
    assert(txt_seq != NULL);
    assert(txt_seq->stream != NULL);

    /* Skip whitespace (including newlines) and read a hash */
    res = hdag_bundle_txt_read_hash(stream, phash, hash_len, true);
    /* If we failed reading */
    if (hdag_res_is_failure(res)) {
        return res;
    }
    /* Return the stream's target hash sequence */
    *ptarget_hash_seq = (struct hdag_hash_seq){
        .hash_len = hash_len,
        .next_fn = hdag_bundle_txt_hash_seq_next,
        .data = txt_seq,
    };
    /* Return OK if we got a hash */
    return res >= hash_len ? 1 : HDAG_RES_OK;
}

hdag_res
hdag_bundle_txt_load(struct hdag_bundle *pbundle,
                     FILE *stream, uint16_t hash_len)
{
    struct hdag_bundle_txt_seq txt_seq = {.stream = stream};

    assert(stream != NULL);
    assert(hdag_hash_len_is_valid(hash_len));

    return hdag_bundle_node_seq_load(pbundle, (struct hdag_node_seq){
        .hash_len = hash_len,
        .next_fn = hdag_bundle_txt_node_seq_next,
        .data = &txt_seq,
    });
}

hdag_res
hdag_bundle_txt_save(FILE *stream, const struct hdag_bundle *bundle)
{
    assert(stream != NULL);
    assert(hdag_bundle_is_valid(bundle));

    hdag_res res = HDAG_RES_INVALID;
    size_t hex_len = bundle->hash_len * 2;
    char *hex_buf = NULL;
    const struct hdag_node *node;
    ssize_t idx;
    size_t target_idx;

    hex_buf = malloc(hex_len + 1);
    if (hex_buf == NULL) {
        goto cleanup;
    }

    /* For each node */
    HDAG_DARR_ITER_FORWARD(&bundle->nodes, idx, node, (void)0, (void)0) {
        /*
         * Skip the nodes with unknown targets, as they can't be represented
         */
        if (hdag_targets_are_unknown(&node->targets)) {
            continue;
        }
        hdag_bytes_to_hex(hex_buf, node->hash, bundle->hash_len);
        if (fwrite(hex_buf, hex_len, 1, stream) != 1) {
            goto cleanup;
        }
        /* For each target */
        for (target_idx = 0;
             target_idx < hdag_node_targets_count(node);
             target_idx++) {
            if (fputc(' ', stream) == EOF) {
                goto cleanup;
            }
            hdag_bytes_to_hex(
                hex_buf,
                hdag_bundle_targets_node_hash(bundle, idx, target_idx),
                bundle->hash_len
            );
            if (fwrite(hex_buf, hex_len, 1, stream) != 1) {
                goto cleanup;
            }
        }
        if (fputc('\n', stream) == EOF) {
            goto cleanup;
        }
    }

    res = HDAG_RES_OK;

cleanup:
    free(hex_buf);
    return HDAG_RES_ERRNO_IF_INVALID(res);
}

hdag_res
hdag_bundle_txt_ingest(struct hdag_bundle *pbundle,
                       FILE *stream, uint16_t hash_len)
{
    struct hdag_bundle_txt_seq txt_seq = {.stream = stream};

    assert(stream != NULL);
    assert(hdag_hash_len_is_valid(hash_len));

    return hdag_bundle_node_seq_ingest(pbundle, (struct hdag_node_seq){
        .hash_len = hash_len,
        .next_fn = hdag_bundle_txt_node_seq_next,
        .data = &txt_seq,
    });
}

hdag_res
hdag_bundle_node_seq_ingest(struct hdag_bundle *pbundle,
                            struct hdag_node_seq node_seq)
{
    hdag_res            res      = HDAG_RES_INVALID;
    struct hdag_bundle  bundle  = HDAG_BUNDLE_EMPTY(node_seq.hash_len);

    assert(hdag_node_seq_is_valid(&node_seq));

    /* Disable profiling */
#undef HDAG_PROFILE_TIME
#define HDAG_PROFILE_TIME(_action, _statement) _statement

    /* Load the node sequence (adjacency list) */
    HDAG_PROFILE_TIME(
        "Loading node sequence",
        HDAG_RES_TRY(hdag_bundle_node_seq_load(&bundle, node_seq))
    );

    /* Sort the nodes and edges by hash lexicographically */
    HDAG_PROFILE_TIME("Sorting the bundle",
                      hdag_bundle_sort(&bundle));

    /* Deduplicate the nodes and edges */
    HDAG_PROFILE_TIME("Deduping the bundle",
                      HDAG_RES_TRY(hdag_bundle_dedup(&bundle)));

    /* Fill in the fanout array */
    HDAG_PROFILE_TIME("Filling in fanout array",
                      hdag_bundle_fanout_fill(&bundle));

    /* Compact the edges */
    HDAG_PROFILE_TIME("Compacting the bundle",
                      hdag_bundle_compact(&bundle));

    /* Try to enumerate the generations */
    HDAG_PROFILE_TIME(
        "Enumerating the generations",
        HDAG_RES_TRY(hdag_bundle_generations_enumerate(&bundle))
    );

    /* Try to enumerate the components */
    HDAG_PROFILE_TIME(
        "Enumerating the components",
        HDAG_RES_TRY(hdag_bundle_components_enumerate(&bundle))
    );

    /* Shrink the extra space allocated for the bundle */
    HDAG_RES_TRY(hdag_bundle_deflate(&bundle));

#undef HDAG_PROFILE_TIME

    assert(hdag_bundle_is_valid(&bundle));
    if (pbundle != NULL) {
        *pbundle = bundle;
        bundle = HDAG_BUNDLE_EMPTY(node_seq.hash_len);
    }
    res = HDAG_RES_OK;
cleanup:
    hdag_bundle_cleanup(&bundle);
    return HDAG_RES_ERRNO_IF_INVALID(res);
}
