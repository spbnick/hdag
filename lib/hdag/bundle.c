/*
 * Hash DAG bundle - a file in the making
 */

#include <hdag/bundle.h>
#include <hdag/nodes.h>
#include <hdag/hash.h>
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

/**
 * Remove duplicate node entries, preferring known ones, from a bundle,
 * assuming nodes are sorted by hash.
 *
 * @param bundle    The bundle to deduplicate nodes in.
 */
static void
hdag_bundle_dedup(struct hdag_bundle *bundle)
{
    assert(bundle != NULL);
    assert(hdag_hash_len_is_valid(bundle->hash_len));

#define NODE(_idx) hdag_darr_element(&bundle->nodes, _idx)

    /* The first node in the same-hash run */
    struct hdag_node *first_node;
    /* The first node in the same-hash run which had targets */
    struct hdag_node *first_known_node = NULL;
    /* Currently traversed node */
    struct hdag_node *node;
    /* Previously traversed node */
    struct hdag_node *prev_node = NULL;
    /* The index of the first node in the same-hash run (first_node) */
    ssize_t first_idx;
    /* The index of the currently-traversed node */
    ssize_t idx;
    int relation;

    /* For each node, starting from the end */
    HDAG_DARR_ITER_BACKWARD(&bundle->nodes, idx, node,
                            (first_idx = idx, first_node = node),
                            (prev_node = node)) {
        assert(hdag_node_is_valid(node));
        relation = memcmp(node->hash, first_node->hash, bundle->hash_len);
        assert(relation <= 0);
        /* If the nodes have the same hash */
        if (relation == 0) {
            if (first_known_node == NULL &&
                hdag_targets_are_known(&node->targets)) {
                first_known_node = node;
            }
        /* Else nodes have different hash */
        } else {
            /* If there was more than one node */
            if (first_idx - idx > 1) {
                /* If the first known node wasn't the last node in the run */
                if (first_known_node != NULL &&
                    first_known_node != prev_node) {
                    /* Make it the last one */
                    *prev_node = *first_known_node;
                }
                /* Move the already-deduped nodes over the rest of this run */
                memmove(NODE(idx + 2), NODE(first_idx + 1),
                        bundle->nodes.slots_occupied - (first_idx + 1));
                bundle->nodes.slots_occupied -= first_idx - (idx + 1);
            }
            first_idx = idx;
            first_node = node;
        }
    }
#undef NODE
}

/**
 * Compact edge targets into nodes, putting the rest into "extra edges",
 * assuming the nodes are sorted and de-duplicated.
 *
 * @param bundle    The bundle to compact edges in.
 *
 * @return True if compaction succeeded, false if it failed.
 *         Errno is set in case of failure.
 */
static bool
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

    assert(bundle != NULL);
    assert(hdag_hash_len_is_valid(bundle->hash_len));

    /* For each node, from start to end */
    HDAG_DARR_ITER_FORWARD(&bundle->nodes, idx, node, (void)0, (void)0) {
        assert(hdag_node_is_valid(node));
        /* If the node's targets are unknown */
        if (hdag_targets_are_unknown(&node->targets)) {
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
                assert(found_idx < bundle->target_hashes.slots_occupied);
                /* Store the edge */
                ((struct hdag_edge *)hdag_darr_calloc_one(
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
            /* Store targets inside the node */
            hash_idx = hdag_target_to_ind_idx(node->targets.first);
            found_idx = hdag_nodes_find(
                bundle->nodes.slots, bundle->nodes.slots_occupied,
                bundle->hash_len,
                hdag_darr_element(&bundle->target_hashes, hash_idx)
            );
            /* All hashes must be locatable */
            assert(found_idx < INT32_MAX);
            /* Hash indices must be valid */
            assert(found_idx < bundle->target_hashes.slots_occupied);
            /* Store the first target */
            node->targets.first = hdag_target_from_ind_idx(found_idx);

            /* If there's a second target */
            if (node->targets.last > node->targets.first) {
                hash_idx = hdag_target_to_ind_idx(node->targets.first);
                found_idx = hdag_nodes_find(
                    bundle->nodes.slots, bundle->nodes.slots_occupied,
                    bundle->hash_len,
                    hdag_darr_element(&bundle->target_hashes, hash_idx)
                );
                /* All hashes must be locatable */
                assert(found_idx < INT32_MAX);
                /* Hash indices must be valid */
                assert(found_idx < bundle->target_hashes.slots_occupied);
                /* Store the last target */
                node->targets.last = hdag_target_from_ind_idx(found_idx);
            }
        }
    }

    /* Remove target hashes */
    hdag_darr_cleanup(&bundle->target_hashes);

    return true;
}

bool
hdag_bundle_load_node_seq(struct hdag_bundle *bundle,
                          struct hdag_node_seq node_seq)
{
    bool                    result = false;
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
                hdag_darr_calloc_one(&bundle->nodes);       \
            if (_node == NULL) {                            \
                goto cleanup;                               \
            }                                               \
            memcpy(_node->hash, _hash, bundle->hash_len);   \
            _node->targets.first = _first_target;           \
            _node->targets.last = _last_target;             \
        } while (0)

    /* Add a new target hash (and the corresponding node) */
    #define ADD_TARGET_HASH(_hash) \
        do {                                                    \
            if (hdag_darr_append_one(                           \
                    &bundle->target_hashes, _hash) == NULL) {   \
                goto cleanup;                                   \
            }                                                   \
            ADD_NODE(_hash,                                     \
                     HDAG_TARGET_UNKNOWN,                       \
                     HDAG_TARGET_UNKNOWN);                      \
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
            ADD_TARGET_HASH(target_hash);
        }

        /* Add the node */
        if (first_target_hash_idx == bundle->target_hashes.slots_occupied) {
            ADD_NODE(node_hash, HDAG_TARGET_INVALID, HDAG_TARGET_INVALID);
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

    #undef MAKE_TARGET_HASH_SPACE
    #undef MAKE_NODE_SPACE

    /* Sort the nodes by hash lexicographically */
    qsort_r(bundle->nodes.slots, bundle->nodes.slots_occupied,
            bundle->nodes.slot_size, hdag_node_cmp, &bundle->hash_len);

    /* Deduplicate the nodes */
    hdag_bundle_dedup(bundle);

    /* Attempt to compact the edges */
    if (!hdag_bundle_compact(bundle)) {
        goto cleanup;
    }

    /* Shrink the extra space allocated for the bundle */
    if (!(hdag_darr_deflate(&bundle->nodes) &&
          hdag_darr_deflate(&bundle->target_hashes) &&
          hdag_darr_deflate(&bundle->extra_edges))) {
        goto cleanup;
    }

    result = true;

cleanup:
    free(target_hash);
    free(node_hash);
    assert(hdag_bundle_is_valid(bundle));
    return result;
}
