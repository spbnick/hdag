/*
 * Hash DAG bundle - a file in the making
 */

#include <hdag/bundle.h>
#include <hdag/txt_node_iter.h>
#include <hdag/nodes.h>
#include <hdag/misc.h>
#include <hdag/res.h>
#include <string.h>

hdag_res
hdag_bundle_targets_hash_iter_next(const struct hdag_iter *iter,
                                   void **pitem)
{
    struct hdag_bundle_iter_data *data =
        hdag_bundle_iter_data_validate(iter->data);
    if (data->target_idx <
        hdag_bundle_targets_count(data->bundle,
                                  data->node_idx - 1)) {
        /* The "item_mutable" flag is supposed to protect us */
        *pitem = (void *)hdag_bundle_targets_node_hash(
            data->bundle, data->node_idx - 1, data->target_idx
        );
        data->target_idx++;
        return 1;
    }
    return 0;
}

hdag_res
hdag_bundle_node_iter_targets_hash_iter_next(const struct hdag_iter *iter,
                                             void **pitem)
{
    struct hdag_bundle_iter_data *data = hdag_bundle_iter_data_validate(
        HDAG_CONTAINER_OF(
            struct hdag_bundle_iter_data, item,
            hdag_node_iter_item_validate(
                /* Only the iterator is const, the parent item is not */
                (struct hdag_node_iter_item *)HDAG_CONTAINER_OF(
                    struct hdag_node_iter_item, target_hash_iter, iter
                )
            )
        )
    );
    const struct hdag_bundle *bundle = data->bundle;
    assert(data->node_idx > 0);
    /* If we have any (more) targets */
    if (data->target_idx <
        hdag_bundle_targets_count(bundle, data->node_idx - 1)) {
        /* The "item_mutable" flag is supposed to protect us */
        *pitem = (void *)hdag_bundle_targets_node_hash(
            bundle, data->node_idx - 1, data->target_idx
        );
        data->target_idx++;
        return 1;
    }
    return 0;
}

hdag_res
hdag_bundle_node_iter_next(const struct hdag_iter *iter,
                           void **pitem)
{
    struct hdag_bundle_iter_data *data =
        hdag_bundle_iter_data_validate(iter->data);
    const struct hdag_bundle *bundle = data->bundle;
    const struct hdag_node *node;

    /* Find next suitable node */
    do {
        /* If we ran out of nodes */
        if (data->node_idx >= bundle->nodes.slots_occupied) {
            return 0;
        }
        node = HDAG_BUNDLE_NODE(bundle, data->node_idx++);
    } while (!(data->with_unknown || hdag_node_is_known(node)));
    /* Output */
    data->item.hash = node->hash;
    data->target_idx = 0;
    *pitem = &data->item;
    return 1;
}

static inline bool
hdag_bundle_is_mutable_or_invalid(const struct hdag_bundle *bundle)
{
    return
        hdag_arr_is_mutable(&bundle->nodes) &&
        hdag_arr_is_mutable(&bundle->nodes_fanout) &&
        hdag_arr_is_mutable(&bundle->target_hashes) &&
        hdag_arr_is_mutable(&bundle->extra_edges) &&
        hdag_arr_is_mutable(&bundle->unknown_indexes);
}

static inline bool
hdag_bundle_is_immutable_or_invalid(const struct hdag_bundle *bundle)
{
    return !hdag_bundle_is_mutable_or_invalid(bundle);
}


bool
hdag_bundle_is_valid(const struct hdag_bundle *bundle)
{
    return
        bundle != NULL &&

        /* Hash length */
        (bundle->hash_len == 0 || hdag_hash_len_is_valid(bundle->hash_len)) &&

        /* Nodes */
        hdag_arr_is_valid(&bundle->nodes) &&
        bundle->nodes.slot_size == hdag_node_size(bundle->hash_len) &&
        hdag_arr_slots_occupied(&bundle->nodes) < INT32_MAX &&

        /* Nodes fanout */
        hdag_fanout_arr_is_valid(&bundle->nodes_fanout) &&
        (hdag_fanout_arr_is_empty(&bundle->nodes_fanout) ||
         (bundle->hash_len != 0 &&
          hdag_arr_slots_occupied(&bundle->nodes_fanout) == UINT8_MAX + 1 &&
          hdag_fanout_arr_get(&bundle->nodes_fanout, UINT8_MAX) ==
            hdag_arr_slots_occupied(&bundle->nodes))) &&

        /* Target hashes */
        hdag_arr_is_valid(&bundle->target_hashes) &&
        bundle->target_hashes.slot_size == bundle->hash_len &&
        hdag_arr_slots_occupied(&bundle->target_hashes) < INT32_MAX &&

        /* Extra edges */
        hdag_arr_is_valid(&bundle->extra_edges) &&
        bundle->extra_edges.slot_size == sizeof(struct hdag_edge) &&
        hdag_arr_slots_occupied(&bundle->extra_edges) < INT32_MAX &&
        (bundle->hash_len != 0 ||
         hdag_arr_is_empty(&bundle->target_hashes)) &&

        /* We should have either target hashes or extra edges, not both */
        (hdag_arr_is_empty(&bundle->target_hashes) ||
         hdag_arr_is_empty(&bundle->extra_edges)) &&

        /* Unknown indexes */
        hdag_arr_is_valid(&bundle->unknown_indexes) &&
        bundle->unknown_indexes.slot_size == sizeof(uint32_t) &&

        /* Unknown indexes vs nodes */
        (
            hdag_arr_is_empty(&bundle->nodes)
            /* Cannot have unknown nodes if there are no nodes at all */
            ? hdag_arr_is_empty(&bundle->unknown_indexes)
            : (
                /* A bundle cannot contain only unknown nodes */
                hdag_arr_slots_occupied(&bundle->unknown_indexes) <
                hdag_arr_slots_occupied(&bundle->nodes)
            )
        ) &&

        /* File */
        hdag_file_is_valid(&bundle->file) &&
        (!hdag_file_is_open(&bundle->file) ||
         hdag_bundle_is_immutable_or_invalid(bundle)) &&

        /* End - just making it possible to have trailing && everywhere */
        true;
}

bool
hdag_bundle_is_mutable(const struct hdag_bundle *bundle)
{
    assert(hdag_bundle_is_valid(bundle));
    return hdag_bundle_is_mutable_or_invalid(bundle);
}

bool
hdag_bundle_is_immutable(const struct hdag_bundle *bundle)
{
    assert(hdag_bundle_is_valid(bundle));
    return hdag_bundle_is_immutable_or_invalid(bundle);
}

bool
hdag_bundle_has_index_targets(const struct hdag_bundle *bundle)
{
    ssize_t idx;
    const struct hdag_node *node;
    assert(hdag_bundle_is_valid(bundle));
    HDAG_ARR_IFWD_UNSIZED_CONST(&bundle->nodes, idx, node, (void)0, (void)0) {
        if (hdag_targets_are_direct(&node->targets) ||
            (hdag_targets_are_indirect(&node->targets) &&
             hdag_arr_slots_occupied(&bundle->extra_edges) != 0)) {
            return true;
        }
    }
    return false;
}

bool
hdag_bundle_is_compacted(const struct hdag_bundle *bundle)
{
    ssize_t idx;
    const struct hdag_node *node;
    assert(hdag_bundle_is_valid(bundle));
    if (!hdag_arr_is_empty(&bundle->target_hashes)) {
        return false;
    }
    HDAG_ARR_IFWD_UNSIZED_CONST(&bundle->nodes, idx, node, (void)0, (void)0) {
        if (hdag_targets_are_indirect(&node->targets)) {
            if (hdag_node_get_last_ind_idx(node) -
                hdag_node_get_first_ind_idx(node) <= 1) {
                return false;
            }
        }
    }
    return true;
}

bool
hdag_bundle_is_unenumerated(const struct hdag_bundle *bundle)
{
    ssize_t idx;
    const struct hdag_node *node;

    assert(hdag_bundle_is_valid(bundle));

    HDAG_ARR_IFWD_UNSIZED_CONST(&bundle->nodes, idx, node, (void)0, (void)0) {
        if (node->component || node->generation) {
            return false;
        }
    }

    return true;
}

bool
hdag_bundle_is_enumerated(const struct hdag_bundle *bundle)
{
    ssize_t idx;
    const struct hdag_node *node;

    assert(hdag_bundle_is_valid(bundle));

    HDAG_ARR_IFWD_UNSIZED_CONST(&bundle->nodes, idx, node, (void)0, (void)0) {
        if (!(node->component && node->generation)) {
            return false;
        }
    }

    return true;
}

uint32_t
hdag_bundle_targets_node_idx(const struct hdag_bundle *bundle,
                             uint32_t node_idx, uint32_t target_idx)
{
    const struct hdag_targets *targets;
    assert(hdag_bundle_is_valid(bundle));
    targets = HDAG_BUNDLE_TARGETS(bundle, node_idx);
    assert(target_idx < hdag_targets_count(targets));

    if (hdag_target_is_ind_idx(targets->first)) {
        if (hdag_arr_is_empty(&bundle->extra_edges)) {
            uint32_t target_node_idx = hdag_nodes_find(
                bundle->nodes.slots,
                bundle->nodes.slots_occupied,
                bundle->hash_len,
                HDAG_ARR_ELEMENT_UNSIZED(
                    &bundle->target_hashes, uint8_t,
                    hdag_target_to_ind_idx(targets->first) +
                    target_idx
                )
            );
            assert(target_node_idx < INT32_MAX);
            return target_node_idx;
        } else {
            return HDAG_ARR_ELEMENT(
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

const uint8_t *
hdag_bundle_targets_node_hash(const struct hdag_bundle *bundle,
                              uint32_t node_idx, uint32_t target_idx)
{
    const struct hdag_targets *targets;
    uint32_t target_node_idx;
    assert(hdag_bundle_is_valid(bundle));
    targets = HDAG_BUNDLE_TARGETS(bundle, node_idx);
    assert(target_idx < hdag_targets_count(targets));

    if (hdag_target_is_ind_idx(targets->first)) {
        if (hdag_arr_is_empty(&bundle->extra_edges)) {
            return HDAG_ARR_ELEMENT_UNSIZED(
                &bundle->target_hashes, uint8_t,
                hdag_target_to_ind_idx(targets->first) +
                target_idx
            );
        } else {
            target_node_idx = HDAG_ARR_ELEMENT(
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

hdag_res
hdag_bundle_node_hash_iter_next(const struct hdag_iter *iter, void **pitem)
{
    struct hdag_bundle_iter_data *data =
        hdag_bundle_iter_data_validate(iter->data);
    const struct hdag_bundle *bundle = data->bundle;
    const struct hdag_node *node;

    /* Find next suitable node */
    do {
        /* If we ran out of nodes */
        if (data->node_idx >= bundle->nodes.slots_occupied) {
            return 0;
        }
        node = HDAG_BUNDLE_NODE(bundle, data->node_idx++);
    } while (!(data->with_unknown || hdag_node_is_known(node)));
    /* Output its hash */
    /* The "item_mutable" flag should protect us */
    *pitem = (void *)node->hash;
    return 1;
}

void
hdag_bundle_cleanup(struct hdag_bundle *bundle)
{
    hdag_res res;
    assert(hdag_bundle_is_valid(bundle));
    hdag_arr_cleanup(&bundle->nodes);
    hdag_arr_cleanup(&bundle->nodes_fanout);
    hdag_arr_cleanup(&bundle->target_hashes);
    hdag_arr_cleanup(&bundle->extra_edges);
    hdag_arr_cleanup(&bundle->unknown_indexes);
    /*
     * We're syncing the file after creating, and then never change it.
     * This should not fail.
     */
    res = hdag_file_close(&bundle->file);
    assert(res == HDAG_RES_OK);
    assert(hdag_bundle_is_valid(bundle));
    assert(hdag_bundle_is_clean(bundle));
}

void
hdag_bundle_empty(struct hdag_bundle *bundle)
{
    assert(hdag_bundle_is_valid(bundle));
    hdag_arr_empty(&bundle->nodes);
    hdag_fanout_arr_empty(&bundle->nodes_fanout);
    hdag_arr_empty(&bundle->target_hashes);
    hdag_arr_empty(&bundle->extra_edges);
    hdag_arr_empty(&bundle->unknown_indexes);
    assert(hdag_bundle_is_valid(bundle));
    assert(hdag_bundle_is_empty(bundle));
}

hdag_res
hdag_bundle_deflate(struct hdag_bundle *bundle)
{
    assert(hdag_bundle_is_valid(bundle));
    if (hdag_arr_deflate(&bundle->nodes) &&
        hdag_arr_deflate(&bundle->target_hashes) &&
        hdag_arr_deflate(&bundle->extra_edges)) {
        return HDAG_RES_OK;
    }
    return HDAG_RES_ERRNO;
}

hdag_res
hdag_bundle_fanout_fill(struct hdag_bundle *bundle)
{
    /* Position in the fanout array == first byte of node hash */
    size_t pos;
    /* Currently traversed node */
    const struct hdag_node *node;
    /* The index of the currently-traversed node */
    ssize_t idx;

    assert(hdag_bundle_is_valid(bundle));
    assert(hdag_bundle_is_mutable(bundle));
    assert(!hdag_bundle_is_hashless(bundle));
    assert(hdag_bundle_is_sorted_and_deduped(bundle));

    if (!hdag_arr_uresize(&bundle->nodes_fanout, UINT8_MAX + 1)) {
        return HDAG_RES_ERRNO;
    }
    hdag_fanout_arr_zero(&bundle->nodes_fanout);

    HDAG_ARR_IFWD_UNSIZED_CONST(&bundle->nodes, idx, node, pos = 0, (void)0) {
        for (; pos < node->hash[0]; pos++) {
            hdag_fanout_arr_set(&bundle->nodes_fanout, pos, idx);
        }
    }
    for (; pos < hdag_arr_slots_occupied(&bundle->nodes_fanout); pos++) {
        hdag_fanout_arr_set(&bundle->nodes_fanout, pos, idx);
    }

    assert(hdag_fanout_arr_is_valid(&bundle->nodes_fanout));
    assert(hdag_fanout_arr_get(&bundle->nodes_fanout, UINT8_MAX) == idx);

    return HDAG_RES_OK;
}

/**
 * Check if both the nodes and targets of a bundle are sorted according to
 * a specified range of comparison results.
 *
 * @param bundle    The bundle to check the sorting of.
 * @param cmp_min   The minimum comparison result of adjacent nodes [-1, cmp_max].
 * @param cmp_max   The maximum comparison result of adjacent nodes [cmp_min, 1].
 *
 * @return  True if they bundle is sorted according to specification,
 *          false if not.
 */
static bool
hdag_bundle_is_sorted_as(const struct hdag_bundle *bundle,
                         int cmp_min, int cmp_max)
{
    ssize_t node_idx;
    const struct hdag_node *prev_node;
    const struct hdag_node *node;
    int64_t rel;
    int64_t rel_min = cmp_min < 0 ? INT64_MIN : cmp_min;
    int64_t rel_max = cmp_max > 0 ? INT64_MAX : cmp_max;

    /* (Last) target index */
    size_t target_idx = 0;

    assert(hdag_bundle_is_valid(bundle));
    assert(cmp_min >= -1);
    assert(cmp_max >= cmp_min);
    assert(cmp_max <= 1);

    HDAG_ARR_IFWD_UNSIZED_CONST(&bundle->nodes, node_idx, node,
                                prev_node = NULL, prev_node=node) {
        /* Check hash relation */
        if (prev_node != NULL) {
            rel = memcmp(prev_node->hash, node->hash, bundle->hash_len);
            if (rel < rel_min || rel > rel_max) {
                return false;
            }
        }
        if (!hdag_targets_are_indirect(&node->targets)) {
            continue;
        }
        /* Check target relations */
        for (target_idx = hdag_node_get_first_ind_idx(node) + 1;
             target_idx <= hdag_node_get_last_ind_idx(node);
             target_idx++) {
            if (hdag_arr_slots_occupied(&bundle->extra_edges) != 0) {
                rel =
                    (int64_t)(HDAG_ARR_ELEMENT(
                        &bundle->extra_edges,
                        struct hdag_edge, target_idx - 1
                    )->node_idx) -
                    (int64_t)(HDAG_ARR_ELEMENT(
                        &bundle->extra_edges,
                        struct hdag_edge, target_idx
                    )->node_idx);
            } else {
                rel = memcmp(
                    hdag_arr_element_const(
                        &bundle->target_hashes, target_idx - 1
                    ),
                    hdag_arr_element_const(
                        &bundle->target_hashes, target_idx
                    ),
                    bundle->target_hashes.slot_size
                );
            }
            if (rel < rel_min || rel > rel_max) {
                return false;
            }
        }
    }

    return true;
}

bool
hdag_bundle_is_sorted_and_deduped(const struct hdag_bundle *bundle)
{
    return hdag_bundle_is_sorted_as(bundle, -1, -1);
}

hdag_res
hdag_bundle_sort_and_dedup(struct hdag_bundle *bundle, bool merge_targets)
{
    hdag_res res = HDAG_RES_INVALID;
    /* The size of each node */
    const size_t node_size = bundle->nodes.slot_size;
    /* Previously traversed node */
    struct hdag_node *prev_node;
    /* Currently traversed node */
    struct hdag_node *node;
    /* Current output node */
    struct hdag_node *out_node;
    /* The node to keep out of all the nodes in the run */
    struct hdag_node *keep_node;
    /* The sorted and deduped target hashes array */
    struct hdag_arr target_hashes = HDAG_ARR_EMPTY(
        bundle->target_hashes.slot_size,
        bundle->target_hashes.slots_occupied
    );
    size_t first_target_hash_idx;
    /* The end node of the node array */
    const struct hdag_node *end_node =
        hdag_arr_end_slot_const(&bundle->nodes);

    assert(hdag_bundle_is_valid(bundle));
    assert(!hdag_bundle_has_index_targets(bundle));
    assert(!hdag_bundle_is_hashless(bundle));
    assert(hdag_bundle_is_mutable(bundle));

    /* Sort the nodes by hash lexicographically */
    hdag_arr_sort(&bundle->nodes, hdag_node_cmp,
                  (void *)(uintptr_t)bundle->hash_len);

    /*
     * Deduplicate nodes and targets
     */
    prev_node = NULL;
    node = bundle->nodes.slots;
    out_node = node;
    keep_node = NULL;
    first_target_hash_idx = 0;
    /* For each node, plus one slot after */
    while (true) {
        /*
         * If we have a previous node and its hash is different or we ran out
         * of nodes
         */
        if (prev_node != NULL &&
            (node >= end_node ||
             memcmp(node->hash, prev_node->hash, bundle->hash_len) != 0)) {
            /* Our run has ended */
            /* If we found no known nodes */
            if (keep_node == NULL) {
                /* Keep the last unknown node */
                keep_node = prev_node;
            /* Else, if we're told to merge targets */
            } else if (merge_targets) {
                /* Sort and deduplicate combined target hashes */
                /* Update the kept node with new targets */
                keep_node->targets = HDAG_TARGETS_INDIRECT(
                    first_target_hash_idx,
                    target_hashes.slots_occupied =
                        first_target_hash_idx +
                        hdag_arr_mem_sort_and_dedup(
                            &HDAG_ARR_PINNED_SLICE(
                                &target_hashes,
                                first_target_hash_idx,
                                target_hashes.slots_occupied
                            )
                        )
                );
                first_target_hash_idx = target_hashes.slots_occupied;
            }
            /* Output the node to keep */
            if (out_node < keep_node) {
                memcpy(out_node, keep_node, node_size);
            }
            out_node = (struct hdag_node *)((uint8_t *)out_node + node_size);
            keep_node = NULL;
        }
        /* If we ran out of nodes */
        if (node >= end_node) {
            break;
        }
        /* If this is a known node */
        if (hdag_targets_are_known(&node->targets)) {
            /* If it has any targets */
            if (hdag_targets_count(&node->targets) != 0) {
                /* If asked to merge targets */
                if (merge_targets) {
                    /* Append the node's target hashes to the new array */
                    if (!hdag_arr_append(
                        &target_hashes,
                        hdag_arr_slot(
                            &bundle->target_hashes,
                            hdag_node_get_first_ind_idx(node)
                        ),
                        hdag_node_targets_count(node)
                    )) {
                        goto cleanup;
                    }
                /* Else, we're asked to verify target consistency */
                } else {
                    /* Sort and dedup node's targets, so we could compare */
                    node->targets.last = hdag_target_from_ind_idx(
                        hdag_node_get_first_ind_idx(node) +
                        hdag_arr_mem_sort_and_dedup(
                            &HDAG_ARR_PINNED_SLICE(
                                &bundle->target_hashes,
                                hdag_node_get_first_ind_idx(node),
                                hdag_node_get_last_ind_idx(node) + 1
                            )
                        ) - 1
                    );
                }
            }
            /* If this is the first known node in this run */
            if (keep_node == NULL) {
                keep_node = node;
            /* Else, if we're asked to check they're consistent */
            } else if (!merge_targets) {
                /* If they're not */
                if (
                    hdag_targets_count(&node->targets) !=
                    hdag_targets_count(&keep_node->targets) ||
                    (
                        hdag_targets_count(&node->targets) != 0 &&
                        memcmp(hdag_arr_element(
                                    &bundle->target_hashes,
                                    hdag_node_get_first_ind_idx(node)
                               ),
                               hdag_arr_element(
                                    &bundle->target_hashes,
                                    hdag_node_get_first_ind_idx(keep_node)
                               ),
                               hdag_targets_count(&node->targets) *
                               bundle->hash_len
                        ) != 0
                    )
                ) {
                    res = HDAG_RES_NODE_CONFLICT;
                    goto cleanup;
                }
            }
        }
        prev_node = node;
        node = (struct hdag_node *)((uint8_t *)node + node_size);
    }
    /* Truncate the nodes */
    bundle->nodes.slots_occupied =
        hdag_arr_slot_idx(&bundle->nodes, out_node);

    /* If we're asked to merge targets */
    if (merge_targets) {
        /* Replace the target hashes with new ones */
        hdag_arr_cleanup(&bundle->target_hashes);
        bundle->target_hashes = target_hashes;
        target_hashes = HDAG_ARR_EMPTY(target_hashes.slot_size, 0);
    }

    assert(hdag_bundle_is_valid(bundle));
    assert(hdag_bundle_is_sorted_and_deduped(bundle));
    res = HDAG_RES_OK;
cleanup:
    hdag_arr_cleanup(&target_hashes);
    return HDAG_RES_ERRNO_IF_INVALID(res);
}

hdag_res
hdag_bundle_compact(struct hdag_bundle *bundle)
{
    hdag_res res = HDAG_RES_INVALID;
    /* The index of the currently-traversed node */
    ssize_t idx;
    /* A hash index */
    size_t hash_idx;
    /* The index of a found node */
    size_t found_idx;
    /* Currently traversed node */
    struct hdag_node *node;
    /* The new extra_edges array */
    struct hdag_arr extra_edges =
        HDAG_ARR_EMPTY(sizeof(struct hdag_edge), 64);
    struct hdag_edge *edge;

    assert(hdag_bundle_is_valid(bundle));
    assert(hdag_bundle_is_mutable(bundle));
    assert(!hdag_bundle_is_hashless(bundle));
    assert(hdag_bundle_is_sorted_and_deduped(bundle));
    assert(!hdag_bundle_has_index_targets(bundle));
    assert(hdag_arr_slots_occupied(&bundle->extra_edges) == 0);

    /* Empty the unknown indexes, as we'll fill them */
    hdag_arr_empty(&bundle->unknown_indexes);

    /* For each node, from start to end */
    HDAG_ARR_IFWD_UNSIZED(&bundle->nodes, idx, node, (void)0, (void)0) {
        assert(hdag_node_is_valid(node));
        /* If the node's targets are unknown */
        if (hdag_targets_are_unknown(&node->targets)) {
            uint32_t u32_idx = idx;
            /* Add the node's index to unknown indexes */
            if (hdag_arr_append_one(&bundle->unknown_indexes,
                                    &u32_idx) == NULL) {
                goto cleanup;
            }
            continue;
        }
        /* If the node's targets are both absent */
        if (hdag_targets_are_absent(&node->targets)) {
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
                    hdag_arr_element(&bundle->target_hashes, hash_idx)
                );
                /* All hashes must be locatable */
                assert(found_idx < INT32_MAX);
                /* Hash indices must be valid */
                assert(found_idx < bundle->nodes.slots_occupied);
                /* Store the edge */
                edge = hdag_arr_cappend_one(&extra_edges);
                if (edge == NULL) {
                    goto cleanup;
                }
                edge->node_idx = found_idx;
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
                    hdag_arr_element(&bundle->target_hashes, hash_idx)
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
                hdag_arr_element(&bundle->target_hashes, hash_idx)
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
    hdag_arr_cleanup(&bundle->target_hashes);

    /* Move extra edges */
    hdag_arr_cleanup(&bundle->extra_edges);
    bundle->extra_edges = extra_edges;
    extra_edges = HDAG_ARR_VOID;

    assert(hdag_bundle_is_valid(bundle));
    assert(hdag_bundle_is_compacted(bundle));
    res = HDAG_RES_OK;

cleanup:
    hdag_arr_cleanup(&extra_edges);
    return HDAG_RES_ERRNO_IF_INVALID(res);
}

hdag_res
hdag_bundle_invert(struct hdag_bundle *pinverted,
                   const struct hdag_bundle *original,
                   bool hashless)
{
    assert(hdag_bundle_is_valid(original));
    assert(hdag_bundle_is_sorted_and_deduped(original));
    assert(!hdag_bundle_has_hash_targets(original));

    hdag_res                res = HDAG_RES_INVALID;
    struct hdag_bundle      inverted = HDAG_BUNDLE_EMPTY(
        hashless ? 0 : original->hash_len
    );
    ssize_t                 node_idx;
    const struct hdag_node *original_node;
    struct hdag_node       *inverted_node;
    uint32_t                target_count;
    uint32_t                target_idx;

    if (hdag_arr_slots_occupied(&original->nodes) == 0) {
        goto output;
    }

    /* Append uninitialized nodes to inverted bundle */
    if (!hdag_arr_uappend(&inverted.nodes,
                          hdag_arr_slots_occupied(&original->nodes))) {
        goto cleanup;
    }
    /* If we're not dropping hashes */
    if (hdag_arr_size_occupied(&inverted.nodes) ==
        hdag_arr_size_occupied(&original->nodes)) {
        /* Copy all nodes in one go */
        memcpy(inverted.nodes.slots, original->nodes.slots,
               hdag_arr_size_occupied(&inverted.nodes));
    } else {
        /* Copy nodes without hashes, one-by-one */
        HDAG_ARR_IFWD_UNSIZED(&inverted.nodes, node_idx, inverted_node,
                              (void)0, (void)0) {
            memcpy(inverted_node, HDAG_BUNDLE_NODE(original, node_idx),
                   inverted.nodes.slot_size);
        }
    }

    /* Put the number of node's eventual targets into the "generation" */
    HDAG_ARR_IFWD_UNSIZED(&inverted.nodes, node_idx, inverted_node,
                          (void)0, (void)0) {
        inverted_node->generation = 0;
    }
    HDAG_ARR_IFWD_UNSIZED_CONST(&original->nodes, node_idx, original_node,
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
    HDAG_ARR_IFWD_UNSIZED(&inverted.nodes, node_idx, inverted_node,
                          (void)0, (void)0) {
        inverted_node->targets = (inverted_node->generation <= 2)
            ? HDAG_TARGETS_ABSENT
            : HDAG_TARGETS_INDIRECT(
                target_idx,
                (target_idx += inverted_node->generation) - 1
            );
    }
    if (target_idx > 0) {
        if (!hdag_arr_uappend(&inverted.extra_edges, target_idx)) {
            goto cleanup;
        }
    }

    /* Assign all the targets */
    HDAG_ARR_IFWD_UNSIZED_CONST(&original->nodes, node_idx, original_node,
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
                HDAG_ARR_ELEMENT(
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
    assert(hashless ? hdag_bundle_is_hashless(&inverted)
                    : hdag_bundle_is_sorted_and_deduped(&inverted));
    assert(!hdag_bundle_has_hash_targets(&inverted));

    if (pinverted != NULL) {
        *pinverted = inverted;
        inverted = HDAG_BUNDLE_EMPTY(inverted.hash_len);
    }
    res = HDAG_RES_OK;

cleanup:
    hdag_bundle_cleanup(&inverted);
    return HDAG_RES_ERRNO_IF_INVALID(res);
}

/**
 * Enumerate generations in a bundle: assign generation numbers to every node.
 * Resets component IDs.
 *
 * @param bundle    The bundle to enumerate.
 *
 * @return A void universal result.
 */
[[nodiscard]]
static hdag_res
hdag_bundle_enumerate_generations(struct hdag_bundle *bundle)
{
    ssize_t                 idx;
    struct hdag_node       *node;
    ssize_t                 dfs_idx;
    struct hdag_node       *dfs_node;
    ssize_t                 next_dfs_idx;
    struct hdag_node       *next_dfs_node;
    uint32_t                target_idx;

    assert(hdag_bundle_is_valid(bundle));
    assert(hdag_bundle_is_mutable(bundle));
    assert(hdag_bundle_is_sorted_and_deduped(bundle));
    assert(hdag_bundle_is_compacted(bundle));

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
    HDAG_ARR_IFWD_UNSIZED(&bundle->nodes, idx, node, (void)0, (void)0) {
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

    return HDAG_RES_OK;
}

/**
 * Enumerate components in a bundle: assign component numbers to every node.
 *
 * @param bundle    The bundle to enumerate.
 *
 * @return A void universal result.
 */
[[nodiscard]]
static hdag_res
hdag_bundle_enumerate_components(struct hdag_bundle *bundle)
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
    assert(hdag_bundle_is_mutable(bundle));
    assert(hdag_bundle_is_sorted_and_deduped(bundle));
    assert(hdag_bundle_is_compacted(bundle));

    /* Invert the graph */
    HDAG_RES_TRY(hdag_bundle_invert(&inverted, bundle, true));

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
    HDAG_ARR_IFWD_UNSIZED(&inv->nodes, idx, inv_node, (void)0, (void)0) {
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

    res = HDAG_RES_OK;
cleanup:
    hdag_bundle_cleanup(&inverted);
    return HDAG_RES_ERRNO_IF_INVALID(res);
}

hdag_res
hdag_bundle_enumerate(struct hdag_bundle *bundle)
{
    hdag_res            res      = HDAG_RES_INVALID;

    assert(hdag_bundle_is_valid(bundle));
    assert(hdag_bundle_is_mutable(bundle));
    assert(hdag_bundle_is_unenumerated(bundle));

    /* Disable profiling */
#undef HDAG_PROFILE_TIME
#define HDAG_PROFILE_TIME(_action, _statement) _statement

    /* Try to enumerate the generations */
    HDAG_PROFILE_TIME(
        "Enumerating the generations",
        HDAG_RES_TRY(hdag_bundle_enumerate_generations(bundle))
    );

    /* Try to enumerate the components */
    HDAG_PROFILE_TIME(
        "Enumerating the components",
        HDAG_RES_TRY(hdag_bundle_enumerate_components(bundle))
    );

#undef HDAG_PROFILE_TIME

    assert(hdag_bundle_is_valid(bundle));
    res = HDAG_RES_OK;
cleanup:
    return HDAG_RES_ERRNO_IF_INVALID(res);
}

hdag_res
hdag_bundle_from_node_iter(struct hdag_bundle *pbundle,
                           const struct hdag_iter *iter)
{
    assert(hdag_iter_is_valid(iter));
    assert(hdag_node_iter_item_type_is_valid(iter->item_type));
    size_t hash_len = hdag_node_iter_item_type_get_hash_len(iter->item_type);
    assert(hdag_hash_len_is_valid(hash_len));

    hdag_res            res = HDAG_RES_INVALID;
    struct hdag_bundle  bundle = HDAG_BUNDLE_EMPTY(hash_len);
    const uint8_t      *target_hash;
    size_t              first_target_hash_idx;
    const struct hdag_node_iter_item    *item;

    /* Add a new node */
    #define ADD_NODE(_hash, _targets) \
        do {                                                \
            struct hdag_node *_node = (struct hdag_node *)  \
                hdag_arr_cappend_one(&bundle.nodes);        \
            if (_node == NULL) {                            \
                goto cleanup;                               \
            }                                               \
            memcpy(_node->hash, _hash, bundle.hash_len);    \
            _node->targets = _targets;                      \
        } while (0)

    /* Collect each node (and its targets) in the iterator */
    while (HDAG_RES_TRY(hdag_iter_next_const(iter, (const void **)&item))) {
        first_target_hash_idx = bundle.target_hashes.slots_occupied;

        /* Collect each target hash */
        while (HDAG_RES_TRY(hdag_iter_next_const(
            &item->target_hash_iter, (const void **)&target_hash
        ))) {
            /* Add a new target hash (and the corresponding node) */
            if (hdag_arr_append_one(
                    &bundle.target_hashes, target_hash) == NULL) {
                goto cleanup;
            }
            ADD_NODE(target_hash, HDAG_TARGETS_UNKNOWN);
        }

        /* Add the node */
        if (first_target_hash_idx == bundle.target_hashes.slots_occupied) {
            ADD_NODE(item->hash, HDAG_TARGETS_ABSENT);
        } else {
            ADD_NODE(
                item->hash,
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
        bundle = HDAG_BUNDLE_EMPTY(0);
    }

    res = HDAG_RES_OK;

cleanup:
    hdag_bundle_cleanup(&bundle);
    return HDAG_RES_ERRNO_IF_INVALID(res);
}

hdag_res
hdag_bundle_from_txt(struct hdag_bundle *pbundle,
                     FILE *stream, uint16_t hash_len)
{
    hdag_res res = HDAG_RES_INVALID;
    uint8_t *hash_buf = NULL;
    uint8_t *target_hash_buf = NULL;

    assert(stream != NULL);
    assert(hdag_hash_len_is_valid(hash_len));

    hash_buf = malloc(hash_len);
    if (hash_buf == NULL) {
        goto cleanup;
    }
    target_hash_buf = malloc(hash_len);
    if (target_hash_buf == NULL) {
        goto cleanup;
    }

    struct hdag_txt_node_iter_data data;
    struct hdag_iter iter = hdag_txt_node_iter(
        hash_len, &data, stream, hash_buf, target_hash_buf
    );

    HDAG_RES_TRY(hdag_bundle_from_node_iter(pbundle, &iter));

    res = HDAG_RES_OK;
cleanup:
    free(hash_buf);
    free(target_hash_buf);
    return HDAG_RES_ERRNO_IF_INVALID(res);
}

hdag_res
hdag_bundle_to_txt(FILE *stream, const struct hdag_bundle *bundle)
{
    assert(stream != NULL);
    assert(hdag_bundle_is_valid(bundle));
    assert(!hdag_bundle_is_hashless(bundle));

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
    HDAG_ARR_IFWD_UNSIZED_CONST(&bundle->nodes, idx, node, (void)0, (void)0) {
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

bool
hdag_bundle_is_unorganized(const struct hdag_bundle *bundle)
{
    assert(hdag_bundle_is_valid(bundle));
    assert(!hdag_bundle_is_hashless(bundle));
    return !hdag_bundle_has_index_targets(bundle) &&
           hdag_bundle_fanout_is_empty(bundle) &&
           hdag_bundle_is_unenumerated(bundle);
}

hdag_res
hdag_bundle_organize(struct hdag_bundle *bundle, bool merge_targets)
{
    hdag_res            res      = HDAG_RES_INVALID;

    assert(hdag_bundle_is_valid(bundle));
    assert(hdag_bundle_is_mutable(bundle));
    assert(!hdag_bundle_is_hashless(bundle));
    assert(hdag_bundle_is_unorganized(bundle));

    /* Disable profiling */
#undef HDAG_PROFILE_TIME
#define HDAG_PROFILE_TIME(_action, _statement) _statement

    /* Deduplicate the nodes and edges */
    HDAG_PROFILE_TIME(
        "Sorting and deduplicating the bundle",
        HDAG_RES_TRY(hdag_bundle_sort_and_dedup(bundle, merge_targets))
    );

    /* Fill in the fanout array */
    HDAG_PROFILE_TIME("Filling in fanout array",
                      HDAG_RES_TRY(hdag_bundle_fanout_fill(bundle)));

    /* Compact the edges */
    HDAG_PROFILE_TIME("Compacting the bundle",
                      HDAG_RES_TRY(hdag_bundle_compact(bundle)));

    /* Try to enumerate the bundle's components and generations */
    HDAG_PROFILE_TIME("Enumerating the bundle",
                      HDAG_RES_TRY(hdag_bundle_enumerate(bundle)));

    /* Shrink the extra space allocated for the bundle */
    HDAG_RES_TRY(hdag_bundle_deflate(bundle));

#undef HDAG_PROFILE_TIME

    assert(hdag_bundle_is_valid(bundle));
    assert(hdag_bundle_is_organized(bundle));
    res = HDAG_RES_OK;
cleanup:
    return HDAG_RES_ERRNO_IF_INVALID(res);
}

bool
hdag_bundle_is_organized(const struct hdag_bundle *bundle)
{
    assert(hdag_bundle_is_valid(bundle));
    assert(!hdag_bundle_is_hashless(bundle));
    return !hdag_bundle_has_hash_targets(bundle) &&
           (bundle->nodes.slots_occupied == 0 ||
            !hdag_bundle_fanout_is_empty(bundle)) &&
           hdag_bundle_is_sorted_and_deduped(bundle) &&
           hdag_bundle_is_enumerated(bundle);
}

hdag_res
hdag_bundle_organized_from_txt(struct hdag_bundle *pbundle,
                               bool merge_targets,
                               FILE *stream, uint16_t hash_len)
{
    hdag_res            res     = HDAG_RES_INVALID;
    struct hdag_bundle  bundle  = HDAG_BUNDLE_EMPTY(hash_len);

    assert(stream != NULL);
    assert(hdag_hash_len_is_valid(hash_len));

    HDAG_RES_TRY(hdag_bundle_from_txt(&bundle, stream, hash_len));
    HDAG_RES_TRY(hdag_bundle_organize(&bundle, merge_targets));

    assert(hdag_bundle_is_valid(&bundle));
    assert(hdag_bundle_is_organized(&bundle));
    if (pbundle != NULL) {
        *pbundle = bundle;
        bundle = HDAG_BUNDLE_EMPTY(hash_len);
    }
    res = HDAG_RES_OK;
cleanup:
    hdag_bundle_cleanup(&bundle);
    return HDAG_RES_ERRNO_IF_INVALID(res);
}

hdag_res
hdag_bundle_organized_from_node_iter(struct hdag_bundle *pbundle,
                                     bool merge_targets,
                                     const struct hdag_iter *iter)
{
    assert(hdag_iter_is_valid(iter));
    assert(hdag_node_iter_item_type_is_valid(iter->item_type));
    size_t hash_len = hdag_node_iter_item_type_get_hash_len(iter->item_type);
    assert(hdag_hash_len_is_valid(hash_len));

    hdag_res            res     = HDAG_RES_INVALID;
    struct hdag_bundle  bundle  = HDAG_BUNDLE_EMPTY(hash_len);

    /* Disable profiling */
#undef HDAG_PROFILE_TIME
#define HDAG_PROFILE_TIME(_action, _statement) _statement

    /* Load the node iterator (adjacency list) */
    HDAG_PROFILE_TIME(
        "Loading node iterator",
        HDAG_RES_TRY(hdag_bundle_from_node_iter(&bundle, iter))
    );

    /* Organize */
    HDAG_PROFILE_TIME(
        "Organizing the bundle",
        HDAG_RES_TRY(hdag_bundle_organize(&bundle, merge_targets))
    );

#undef HDAG_PROFILE_TIME

    assert(hdag_bundle_is_valid(&bundle));
    if (pbundle != NULL) {
        *pbundle = bundle;
        bundle = HDAG_BUNDLE_EMPTY(hash_len);
    }
    res = HDAG_RES_OK;
cleanup:
    hdag_bundle_cleanup(&bundle);
    return HDAG_RES_ERRNO_IF_INVALID(res);
}

/**
 * Create a bundle from an HDAG file, taking ownership over it and linking its
 * contents in.
 *
 * @param pbundle   The location for the created bundle with the file
 *                  embedded. Can be NULL to not have the bundle output, and
 *                  to have it destroyed after creation instead.
 * @param file      The file to link into the bundle.
 *                  Must be open. Will be closed.
 */
void
hdag_bundle_from_file(struct hdag_bundle *pbundle, struct hdag_file *file)
{
    assert(hdag_file_is_valid(file));
    assert(hdag_file_is_open(file));

    struct hdag_bundle bundle = HDAG_BUNDLE_EMPTY(file->header->hash_len);

    bundle.hash_len = file->header->hash_len;
    bundle.nodes = HDAG_ARR_IMMUTABLE(
        file->nodes,
        hdag_node_size(file->header->hash_len),
        file->header->node_num
    );
    bundle.nodes_fanout = HDAG_ARR_IMMUTABLE(
        file->header->node_fanout,
        sizeof(*file->header->node_fanout),
        HDAG_ARR_LEN(file->header->node_fanout)
    );
    bundle.extra_edges = HDAG_ARR_IMMUTABLE(
        file->extra_edges,
        sizeof(struct hdag_edge),
        file->header->extra_edge_num
    );
    bundle.unknown_indexes = HDAG_ARR_IMMUTABLE(
        file->unknown_indexes,
        sizeof(*file->unknown_indexes),
        file->header->unknown_index_num
    );

    assert(hdag_bundle_is_valid(&bundle));
    assert(hdag_bundle_is_organized(&bundle));

    bundle.file = *file;
    *file = HDAG_FILE_CLOSED;
    assert(hdag_bundle_is_filed(&bundle));

    if (pbundle == NULL) {
        hdag_bundle_cleanup(&bundle);
    } else {
        *pbundle = bundle;
    }
}

hdag_res
hdag_bundle_to_file(struct hdag_file *pfile,
                    const char *pathname,
                    int template_sfxlen,
                    mode_t open_mode,
                    const struct hdag_bundle *bundle)
{
    hdag_res res = HDAG_RES_INVALID;
    struct hdag_file file = HDAG_FILE_CLOSED;
    int orig_errno;

    assert(hdag_bundle_is_valid(bundle));
    assert(hdag_bundle_is_organized(bundle));

    HDAG_RES_TRY(hdag_file_create(&file,
                                  pathname,
                                  template_sfxlen,
                                  open_mode,
                                  bundle->hash_len,
                                  bundle->nodes.slots,
                                  bundle->nodes_fanout.slots,
                                  bundle->extra_edges.slots,
                                  bundle->extra_edges.slots_occupied,
                                  bundle->unknown_indexes.slots,
                                  bundle->unknown_indexes.slots_occupied));
    HDAG_RES_TRY(hdag_file_sync(&file));

    if (pfile != NULL) {
        *pfile = file;
        file = HDAG_FILE_CLOSED;
    }

    res = HDAG_RES_OK;
cleanup:
    orig_errno = errno;
    (void)hdag_file_close(&file);
    errno = orig_errno;
    return HDAG_RES_ERRNO_IF_INVALID(res);
}

hdag_res
hdag_bundle_unfile(struct hdag_bundle *bundle)
{
    hdag_res res = HDAG_RES_INVALID;

    assert(hdag_bundle_is_valid(bundle));
    assert(hdag_bundle_is_filed(bundle));

    struct hdag_bundle new_bundle = HDAG_BUNDLE_EMPTY(bundle->hash_len);

    if (!(
        hdag_arr_copy(&new_bundle.nodes, &bundle->nodes) &&
        hdag_arr_copy(&new_bundle.nodes_fanout, &bundle->nodes_fanout) &&
        hdag_arr_copy(&new_bundle.target_hashes, &bundle->target_hashes) &&
        hdag_arr_copy(&new_bundle.extra_edges, &bundle->extra_edges) &&
        hdag_arr_copy(&new_bundle.unknown_indexes, &bundle->unknown_indexes)
    )) {
        goto cleanup;
    }

    assert(hdag_bundle_is_valid(&new_bundle));
    assert(hdag_bundle_is_organized(&new_bundle));
    assert(!hdag_bundle_is_filed(&new_bundle));

    *bundle = new_bundle;
    new_bundle = HDAG_BUNDLE_EMPTY(bundle->hash_len);

    res = HDAG_RES_OK;
cleanup:
    hdag_bundle_cleanup(&new_bundle);
    return HDAG_RES_ERRNO_IF_INVALID(res);
}

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
hdag_res
hdag_bundle_file(struct hdag_bundle *bundle,
                 const char *pathname,
                 int template_sfxlen,
                 mode_t open_mode)
{
    hdag_res res = HDAG_RES_INVALID;
    struct hdag_file file = HDAG_FILE_CLOSED;
    int orig_errno;

    assert(hdag_bundle_is_valid(bundle));
    assert(hdag_bundle_is_organized(bundle));

    HDAG_RES_TRY(hdag_bundle_to_file(&file, pathname,
                                     template_sfxlen, open_mode,
                                     bundle));
    hdag_bundle_cleanup(bundle);
    hdag_bundle_from_file(bundle, &file);

    res = HDAG_RES_OK;
cleanup:
    orig_errno = errno;
    (void)hdag_file_close(&file);
    errno = orig_errno;
    return HDAG_RES_ERRNO_IF_INVALID(res);
}
