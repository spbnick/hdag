/*
 * Hash DAG bundle - a file in the making
 */

#include <hdag/bundle.h>
#include <hdag/nodes.h>
#include <hdag/hash.h>
#include <hdag/misc.h>
#include <hdag/rc.h>
#include <cgraph.h>
#include <string.h>

void
hdag_bundle_cleanup(struct hdag_bundle *bundle)
{
    assert(hdag_bundle_is_valid(bundle));
    hdag_darr_cleanup(&bundle->nodes);
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
    hdag_darr_empty(&bundle->target_hashes);
    hdag_darr_empty(&bundle->extra_edges);
    assert(hdag_bundle_is_valid(bundle));
    assert(hdag_bundle_is_empty(bundle));
}

hdag_rc
hdag_bundle_deflate(struct hdag_bundle *bundle)
{
    assert(hdag_bundle_is_valid(bundle));
    if (hdag_darr_deflate(&bundle->nodes) &&
        hdag_darr_deflate(&bundle->target_hashes) &&
        hdag_darr_deflate(&bundle->extra_edges)) {
        return HDAG_RC_OK;
    }
    return HDAG_RC_ERRNO;
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

void
hdag_bundle_dedup(struct hdag_bundle *bundle)
{
    assert(hdag_bundle_is_valid(bundle));
    assert(hdag_bundle_is_sorted(bundle));
    assert(!hdag_bundle_has_index_targets(bundle));

    /* The first node in the same-hash run */
    struct hdag_node *first_node;
    /* The first node in the same-hash run which had targets */
    struct hdag_node *first_known_node;
    /* Currently traversed node */
    struct hdag_node *node;
    /* Previously traversed node */
    struct hdag_node *prev_node;
    /* The index of the first node in the same-hash run (first_node) */
    ssize_t first_idx;
    /* The index of the currently-traversed node */
    ssize_t idx;
    /* Relation between nodes' hashes */
    int relation;
    /* The index of the currently-traversed hash */
    size_t hash_idx;
    /* The index of a previously-traversed hash */
    size_t prev_hash_idx;

    /*
     * Remove duplicate nodes, preferring known ones.
     */
    /* For each node, starting from the end */
    HDAG_DARR_ITER_BACKWARD(
        &bundle->nodes, idx, node,
        (
            first_idx = idx,
            first_node = node,
            first_known_node = NULL,
            prev_node = NULL
        ),
        (prev_node = node)
    ) {
        assert(hdag_node_is_valid(node));
        /* If it's the very first node */
        if (prev_node == NULL) {
            if (hdag_targets_are_known(&node->targets)) {
                first_known_node = node;
            }
            continue;
        }
        /* Compare hashes of this and the first node in the current run */
        relation = memcmp(node->hash, first_node->hash, bundle->hash_len);
        assert(relation <= 0);
        /* If the hashes are the same (we're still in the run) */
        if (relation == 0) {
            if (first_known_node == NULL &&
                hdag_targets_are_known(&node->targets)) {
                first_known_node = node;
            }
        /* Else nodes have different hash (we're in the next run) */
        } else {
            /* If there was more than one node in the previous run */
            if (first_idx - idx > 1) {
                /* If the first known node wasn't the last node in the run */
                if (first_known_node != NULL &&
                    first_known_node != prev_node) {
                    /* Make it the last one */
                    memcpy(prev_node, first_known_node, bundle->nodes.slot_size);
                }
                /* Move the already-deduped nodes over the rest of this run */
                hdag_darr_remove(&bundle->nodes, idx + 2, first_idx + 1);
            }
            /* Start a new run with this node */
            first_idx = idx;
            first_node = node;
            first_known_node =
                hdag_targets_are_known(&node->targets) ? node : NULL;
        }
    }

    /* If there was more than one node in the last run */
    if (first_idx > 0) {
        /* If the first known node wasn't the last node in the run */
        if (first_known_node != NULL &&
            first_known_node != prev_node) {
            /* Make it the last one */
            memcpy(prev_node, first_known_node, bundle->nodes.slot_size);
        }
        /* Move the already-deduped nodes over the rest of this run */
        hdag_darr_remove(&bundle->nodes, 1, first_idx + 1);
    }

    /*
     * Remove duplicate edges
     */
    assert(hdag_darr_occupied_slots(&bundle->extra_edges) == 0);
    HDAG_DARR_ITER_FORWARD(&bundle->nodes, idx, node, (void)0, (void)0) {
        if (!hdag_targets_are_indirect(&node->targets)) {
            continue;
        }
        /* For each target hash starting with the second one */
        for (hash_idx = hdag_node_get_first_ind_idx(node) + 1;
             hash_idx <= hdag_node_get_last_ind_idx(node);) {
            /* Check if the current hash is equal to a previous one */
            for (prev_hash_idx = hdag_node_get_first_ind_idx(node);
                 prev_hash_idx < hash_idx &&
                 memcmp(hdag_darr_element(&bundle->target_hashes, hash_idx),
                        hdag_darr_element(&bundle->target_hashes,
                                          prev_hash_idx),
                        bundle->hash_len) != 0;
                 prev_hash_idx++);
            /* If it is not */
            if (prev_hash_idx >= hash_idx) {
                /* Continue with the next one */
                hash_idx++;
                continue;
            }
            /* Shift following hashes one slot closer */
            memmove(hdag_darr_element(&bundle->target_hashes, hash_idx),
                    hdag_darr_slot(&bundle->target_hashes, hash_idx + 1),
                    (hdag_node_get_last_ind_idx(node) - hash_idx) *
                    bundle->target_hashes.slot_size);
            /* Zero last hash slot */
            memset(hdag_darr_element(&bundle->target_hashes,
                                     hdag_node_get_last_ind_idx(node)),
                   0, bundle->target_hashes.slot_size);
            /* Remove it from the node's targets */
            node->targets.last--;
            /* And continue with the next hash */
        }
    }

    assert(hdag_bundle_is_valid(bundle));
    assert(hdag_bundle_is_sorted_and_deduped(bundle));
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

    assert(hdag_bundle_is_valid(bundle));
    assert(hdag_bundle_is_sorted_and_deduped(bundle));
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
            size_t first_extra_edge_idx = bundle->extra_edges.slots_occupied;

            /* Convert all target hashes to extra edges */
            for (hash_idx = hdag_target_to_ind_idx(node->targets.first);
                 hash_idx <= hdag_target_to_ind_idx(node->targets.last);
                 hash_idx++) {
                found_idx = hdag_nodes_find(
                    bundle->nodes.slots, bundle->nodes.slots_occupied,
                    bundle->hash_len,
                    hdag_darr_element(&bundle->target_hashes, hash_idx)
                );
                /* All hashes must be locatable */
                assert(found_idx < INT32_MAX);
                /* Hash indices must be valid */
                assert(found_idx < bundle->nodes.slots_occupied);
                /* Store the edge */
                ((struct hdag_edge *)hdag_darr_cappend_one(
                    &bundle->extra_edges
                ))->node_idx = found_idx;
            }

            /* Store the extra edge indices */
            node->targets.first =
                hdag_target_from_ind_idx(first_extra_edge_idx);
            node->targets.last =
                hdag_target_from_ind_idx(
                        bundle->extra_edges.slots_occupied - 1
                );
        } else {
            /* If there are two targets */
            if (node->targets.last > node->targets.first) {
                /* Store the second target inside the node */
                hash_idx = hdag_target_to_ind_idx(node->targets.last);
                found_idx = hdag_nodes_find(
                    bundle->nodes.slots, bundle->nodes.slots_occupied,
                    bundle->hash_len,
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
            found_idx = hdag_nodes_find(
                bundle->nodes.slots, bundle->nodes.slots_occupied,
                bundle->hash_len,
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

    assert(hdag_bundle_is_valid(bundle));
    assert(hdag_bundle_is_compacted(bundle));
}

hdag_rc
hdag_bundle_invert(struct hdag_bundle *pinverted,
                   const struct hdag_bundle *original)
{
    assert(hdag_bundle_is_valid(original));
    assert(hdag_bundle_is_sorted_and_deduped(original));
    assert(!hdag_bundle_has_hash_targets(original));

    hdag_rc                 rc = HDAG_RC_INVALID;
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
    rc = HDAG_RC_OK;

cleanup:
    hdag_bundle_cleanup(&inverted);
    return HDAG_RC_ERRNO_IF_INVALID(rc);
}

hdag_rc
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
                    return HDAG_RC_GRAPH_CYCLE;
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
    return HDAG_RC_OK;
}

hdag_rc
hdag_bundle_components_enumerate(struct hdag_bundle *bundle)
{
    hdag_rc                 rc = HDAG_RC_INVALID;
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
    HDAG_RC_TRY(hdag_bundle_invert(&inverted, bundle));

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
    rc = HDAG_RC_OK;
cleanup:
    hdag_bundle_cleanup(&inverted);
    return HDAG_RC_ERRNO_IF_INVALID(rc);
}

hdag_rc
hdag_bundle_load_node_seq(struct hdag_bundle *bundle,
                          struct hdag_node_seq node_seq)
{
    hdag_rc                 rc = HDAG_RC_INVALID;
    uint8_t                *node_hash = NULL;
    uint8_t                *target_hash = NULL;
    struct hdag_hash_seq    target_hash_seq;
    int                     seq_rc;
    size_t                  first_target_hash_idx;

    assert(hdag_bundle_is_valid(bundle));
    assert(hdag_bundle_is_empty(bundle));

    node_hash = malloc(bundle->hash_len);
    if (node_hash == NULL) {
        goto cleanup;
    }
    target_hash = malloc(bundle->hash_len);
    if (target_hash == NULL) {
        goto cleanup;
    }

    /* Add a new node */
    #define ADD_NODE(_hash, _first_target, _last_target) \
        do {                                                \
            struct hdag_node *_node = (struct hdag_node *)  \
                hdag_darr_cappend_one(&bundle->nodes);      \
            if (_node == NULL) {                            \
                goto cleanup;                               \
            }                                               \
            memcpy(_node->hash, _hash, bundle->hash_len);   \
            _node->targets.first = _first_target;           \
            _node->targets.last = _last_target;             \
        } while (0)

    /* Collect each node (and its targets) in the sequence */
    while (true) {
        /* Get next node */
        seq_rc = node_seq.next(node_seq.data, node_hash, &target_hash_seq);
        if (seq_rc != 0) {
            if (seq_rc < 0) {
                goto cleanup;
            } else {
                break;
            }
        }

        first_target_hash_idx = bundle->target_hashes.slots_occupied;

        /* Collect each target hash */
        while (true) {
            seq_rc = target_hash_seq.next(target_hash_seq.data, target_hash);
            if (seq_rc != 0) {
                if (seq_rc < 0) {
                    goto cleanup;
                } else {
                    break;
                }
            }
            /* Add a new target hash (and the corresponding node) */
            if (hdag_darr_append_one(
                    &bundle->target_hashes, target_hash) == NULL) {
                goto cleanup;
            }
            ADD_NODE(target_hash, HDAG_TARGET_UNKNOWN, HDAG_TARGET_UNKNOWN);
        }

        /* Add the node */
        if (first_target_hash_idx == bundle->target_hashes.slots_occupied) {
            ADD_NODE(node_hash, HDAG_TARGET_ABSENT, HDAG_TARGET_ABSENT);
        } else {
            ADD_NODE(
                node_hash,
                hdag_target_from_ind_idx(first_target_hash_idx),
                hdag_target_from_ind_idx(
                    bundle->target_hashes.slots_occupied - 1
                )
            );
        }
    }

    #undef ADD_NODE

    rc = HDAG_RC_OK;

cleanup:
    free(target_hash);
    free(node_hash);
    assert(hdag_bundle_is_valid(bundle));
    return HDAG_RC_ERRNO_IF_INVALID(rc);
}

hdag_rc
hdag_bundle_ingest_node_seq(struct hdag_bundle *bundle,
                            struct hdag_node_seq node_seq)
{
    hdag_rc rc = HDAG_RC_INVALID;

    assert(hdag_bundle_is_valid(bundle));
    assert(hdag_bundle_is_empty(bundle));

    /* Load the node sequence (adjacency list) */
    HDAG_RC_TRY(hdag_bundle_load_node_seq(bundle, node_seq));

    /* Sort the nodes by hash lexicographically */
    hdag_bundle_sort(bundle);

    /* Deduplicate the nodes and edges */
    hdag_bundle_dedup(bundle);

    /* Compact the edges */
    hdag_bundle_compact(bundle);

    /* Try to enumerate the generations */
    HDAG_RC_TRY(hdag_bundle_generations_enumerate(bundle));

    /* Try to enumerate the components */
    HDAG_RC_TRY(hdag_bundle_components_enumerate(bundle));

    /* Shrink the extra space allocated for the bundle */
    HDAG_RC_TRY(hdag_bundle_deflate(bundle));

    rc = HDAG_RC_OK;
cleanup:
    assert(hdag_bundle_is_valid(bundle));
    return HDAG_RC_ERRNO_IF_INVALID(rc);
}
