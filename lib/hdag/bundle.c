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

hdag_res
hdag_bundle_targets_hash_seq_next(struct hdag_hash_seq *base_seq,
                                     uint8_t *phash)
{
    struct hdag_bundle_targets_hash_seq *seq = HDAG_CONTAINER_OF(
        struct hdag_bundle_targets_hash_seq, base, base_seq
    );
    assert(hdag_hash_seq_is_valid(base_seq));
    assert(seq->bundle->hash_len == base_seq->hash_len);
    assert(phash != NULL);
    if (seq->target_idx <
        hdag_bundle_targets_count(seq->bundle,
                                  seq->node_idx)) {
        memcpy(phash,
               hdag_bundle_targets_node_hash(seq->bundle,
                                             seq->node_idx,
                                             seq->target_idx),
               base_seq->hash_len);
        seq->target_idx++;
        return HDAG_RES_OK;
    }
    return 1;
}

void
hdag_bundle_targets_hash_seq_reset(struct hdag_hash_seq *base_seq)
{
    struct hdag_bundle_targets_hash_seq *seq = HDAG_CONTAINER_OF(
        struct hdag_bundle_targets_hash_seq, base, base_seq
    );
    assert(hdag_hash_seq_is_valid(base_seq));
    assert(base_seq->hash_len == seq->bundle->hash_len);
    seq->target_idx = 0;
}

hdag_res
hdag_bundle_node_seq_next(struct hdag_node_seq *base_seq,
                             uint8_t *phash,
                             struct hdag_hash_seq **ptarget_hash_seq)
{
    assert(hdag_node_seq_is_valid(base_seq));
    assert(phash != NULL);
    assert(ptarget_hash_seq != NULL);
    struct hdag_bundle_node_seq *seq = HDAG_CONTAINER_OF(
        struct hdag_bundle_node_seq, base, base_seq
    );
    const struct hdag_bundle *bundle = seq->bundle;
    assert(hdag_bundle_is_valid(bundle));
    assert(base_seq->hash_len == bundle->hash_len);

    if (seq->node_idx >= bundle->nodes.slots_occupied) {
        return 1;
    }
    memcpy(phash, HDAG_BUNDLE_NODE(bundle, seq->node_idx)->hash,
           bundle->hash_len);
    *ptarget_hash_seq = hdag_bundle_targets_hash_seq_init(
        &seq->targets_hash_seq, bundle, seq->node_idx
    );
    seq->node_idx++;
    return 0;
}

void
hdag_bundle_node_seq_reset(struct hdag_node_seq *base_seq)
{
    assert(hdag_node_seq_is_valid(base_seq));
    struct hdag_bundle_node_seq *seq = HDAG_CONTAINER_OF(
        struct hdag_bundle_node_seq, base, base_seq
    );
    assert(hdag_bundle_is_valid(seq->bundle));
    assert(base_seq->hash_len == seq->bundle->hash_len);
    seq->node_idx = 0;
}

bool
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
        hdag_darr_is_valid(&bundle->unknown_hashes) &&
        bundle->unknown_hashes.slot_size == bundle->hash_len &&
        /* A bundle cannot contain only unknown nodes */
        (hdag_darr_is_empty(&bundle->nodes) ||
         hdag_darr_occupied_slots(&bundle->unknown_hashes) <
            hdag_darr_occupied_slots(&bundle->nodes)) &&
        hdag_darr_is_valid(&bundle->extra_edges) &&
        bundle->extra_edges.slot_size == sizeof(struct hdag_edge) &&
        hdag_darr_occupied_slots(&bundle->extra_edges) < INT32_MAX &&
        (bundle->hash_len != 0 || hdag_darr_is_empty(&bundle->target_hashes)) &&
        (hdag_darr_is_empty(&bundle->target_hashes) ||
         hdag_darr_is_empty(&bundle->extra_edges));
}

bool
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

bool
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

bool
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

bool
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

uint32_t
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

struct hdag_hash_seq *
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
            .reset_fn = hdag_bundle_targets_hash_seq_reset,
            .next_fn = hdag_bundle_targets_hash_seq_next,
        },
        .bundle = bundle,
        .node_idx = node_idx,
        .target_idx = 0,
    };

    assert(hdag_hash_seq_is_valid(&pseq->base));
    assert(hdag_hash_seq_is_resettable(&pseq->base));
    return &pseq->base;
}

struct hdag_node_seq *
hdag_bundle_node_seq_init(struct hdag_bundle_node_seq *pseq,
                          const struct hdag_bundle *bundle)
{
    assert(pseq != NULL);
    assert(hdag_bundle_is_valid(bundle));

    *pseq = (struct hdag_bundle_node_seq){
        .base = {
            .hash_len = bundle->hash_len,
            .reset_fn = hdag_bundle_node_seq_reset,
            .next_fn = hdag_bundle_node_seq_next,
        },
        .bundle = bundle,
        .node_idx = 0,
    };

    assert(hdag_node_seq_is_valid(&pseq->base));
    assert(hdag_node_seq_is_resettable(&pseq->base));
    return &pseq->base;
}

void
hdag_bundle_cleanup(struct hdag_bundle *bundle)
{
    assert(hdag_bundle_is_valid(bundle));
    hdag_darr_cleanup(&bundle->nodes);
    hdag_fanout_empty(bundle->nodes_fanout,
                      HDAG_ARR_LEN(bundle->nodes_fanout));
    hdag_darr_cleanup(&bundle->target_hashes);
    hdag_darr_cleanup(&bundle->unknown_hashes);
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
    hdag_darr_empty(&bundle->unknown_hashes);
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
    assert(!hdag_bundle_is_hashless(bundle));
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
    size_t target_idx;
    int64_t rel;
    int64_t rel_min = cmp_min < 0 ? INT64_MIN : cmp_min;
    int64_t rel_max = cmp_max > 0 ? INT64_MAX : cmp_max;

    assert(hdag_bundle_is_valid(bundle));
    assert(cmp_min >= -1);
    assert(cmp_max >= cmp_min);
    assert(cmp_max <= 1);

    HDAG_DARR_ITER_FORWARD(&bundle->nodes, node_idx, node,
                           prev_node = NULL, prev_node=node) {
        if (prev_node != NULL) {
            rel = memcmp(prev_node->hash, node->hash, bundle->hash_len);
            if (rel < rel_min || rel > rel_max) {
                return false;
            }
        }
        if (!hdag_targets_are_indirect(&node->targets)) {
            continue;
        }
        for (target_idx = hdag_node_get_first_ind_idx(node) + 1;
             target_idx <= hdag_node_get_last_ind_idx(node);
             target_idx++) {
            if (hdag_darr_occupied_slots(&bundle->extra_edges) != 0) {
                rel =
                    (int64_t)(HDAG_DARR_ELEMENT(
                        &bundle->extra_edges,
                        struct hdag_edge, target_idx - 1
                    )->node_idx) -
                    (int64_t)(HDAG_DARR_ELEMENT(
                        &bundle->extra_edges,
                        struct hdag_edge, target_idx
                    )->node_idx);
            } else {
                rel = memcmp(
                    hdag_darr_element_const(
                        &bundle->target_hashes, target_idx - 1
                    ),
                    hdag_darr_element_const(
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
hdag_bundle_is_sorted(const struct hdag_bundle *bundle)
{
    return hdag_bundle_is_sorted_as(bundle, -1, 0);
}

bool
hdag_bundle_is_sorted_and_deduped(const struct hdag_bundle *bundle)
{
    ssize_t idx;
    uint8_t *hash;
    uint8_t *prev_hash;

    if (!hdag_bundle_is_sorted_as(bundle, -1, -1)) {
        return false;
    }
    HDAG_DARR_ITER_FORWARD(&bundle->unknown_hashes, idx, hash,
                           prev_hash = NULL, prev_hash = hash) {
        if (prev_hash != NULL &&
            memcmp(prev_hash, hash, bundle->unknown_hashes.slot_size) >= 0) {
            return false;
        }
    }

    return true;
}

/**
 * Find duplicate edges in a bundle, and either discard them, or raise an
 * error, depending on the specified context.
 *
 * @param bundle    The bundle to find duplicate edges in.
 *                  Must be sorted, and use target_hashes.
 * @param ctx       The context of this bundle (the abstract supergraph).
 *                  Can be NULL, which is interpreted as an empty context.
 *
 * @return A void universal result.
 */
[[nodiscard]]
static hdag_res
hdag_bundle_dedup_edges(struct hdag_bundle *bundle,
                        const struct hdag_ctx *ctx)
{
    assert(hdag_bundle_is_valid(bundle));
    assert(!hdag_bundle_is_hashless(bundle));
    assert(hdag_bundle_is_sorted(bundle));
    assert(!hdag_bundle_has_index_targets(bundle));
    assert(hdag_darr_occupied_slots(&bundle->extra_edges) == 0);
    assert(ctx == NULL || hdag_ctx_is_valid(ctx));

    if (ctx == NULL) {
        ctx = &HDAG_CTX_EMPTY(bundle->hash_len);
    }

    /* Currently traversed node index */
    ssize_t idx;
    /* Currently traversed node */
    struct hdag_node *node;

    /* The size of each target_hash */
    const size_t hash_size = bundle->target_hashes.slot_size;
    /* Location of the node's original last hash */
    uint8_t *last_hash;
    /* Currently-traversed hash */
    uint8_t *hash;
    /* The hash before the current one */
    uint8_t *prev_hash;
    /* The hash output position */
    uint8_t *out_hash;

    /* For each (potentially duplicate) node */
    HDAG_DARR_ITER_FORWARD(&bundle->nodes, idx, node, (void)0, (void)0) {
        /* If the node doesn't have indirect targets */
        if (!hdag_targets_are_indirect(&node->targets)) {
            continue;
        }
        last_hash = hdag_darr_element(&bundle->target_hashes,
                                      hdag_node_get_last_ind_idx(node));
        /* For each target hash starting with the second one */
        for (
            prev_hash = hdag_darr_element(
                &bundle->target_hashes, hdag_node_get_first_ind_idx(node)
            ),
            hash = prev_hash + hash_size,
            out_hash = hash;
            hash <= last_hash;
            prev_hash = hash, hash += hash_size
        ) {
            /* If the current hash is different from the previous one */
            if (memcmp(hash, prev_hash, hash_size) != 0) {
                /* If the output pointer is less than the current hash */
                if (out_hash < hash) {
                    /* Copy the current hash to the output pointer */
                    memcpy(out_hash, hash, bundle->hash_len);
                }
                /* Advance the output pointer */
                out_hash += hash_size;
            }
        }
        /* Update the last target hash index */
        node->targets.last = hdag_target_from_ind_idx(
            hdag_darr_element_idx(&bundle->target_hashes,
                                  out_hash - hash_size)
        );
    }
    assert(hdag_bundle_is_valid(bundle));
    return HDAG_RES_OK;
}

/**
 * Find duplicate nodes in a bundle, and either discard them, or raise an
 * error, depending on the specified context.
 *
 * @param bundle    The bundle to find duplicate nodes in.
 *                  Must be sorted, and use target_hashes.
 * @param ctx       The context of this bundle (the abstract supergraph).
 *                  Can be NULL, which is interpreted as an empty context.
 *
 * @return A void universal result. Including:
 *         * HDAG_RES_NODE_CONFLICT, if nodes with matching hashes but
 *           different targets were found.
 */
[[nodiscard]]
static hdag_res
hdag_bundle_dedup_nodes(struct hdag_bundle *bundle,
                        const struct hdag_ctx *ctx)
{
    hdag_res res = HDAG_RES_INVALID;
    assert(hdag_bundle_is_valid(bundle));
    assert(!hdag_bundle_is_hashless(bundle));
    assert(hdag_bundle_is_sorted(bundle));
    assert(!hdag_bundle_has_index_targets(bundle));
    assert(ctx == NULL || hdag_ctx_is_valid(ctx));

    if (ctx == NULL) {
        ctx = &HDAG_CTX_EMPTY(bundle->hash_len);
    }

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
    /* Number of nodes */
    size_t node_num = bundle->nodes.slots_occupied;
    /* The end node of the node array */
    const struct hdag_node *end_node =
        hdag_darr_slot(&bundle->nodes, node_num);

    prev_node = NULL;
    node = bundle->nodes.slots;
    out_node = node;
    keep_node = NULL;
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
                /* Add the node hash to unknown hashes */
                if (hdag_darr_append_one(&bundle->unknown_hashes,
                                         keep_node->hash) == NULL) {
                    goto cleanup;
                }
            }
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
            /* If this is the first known node in this run */
            if (keep_node == NULL) {
                keep_node = node;
            /* Else it's not the first one and if its targets differ */
            } else if (
                hdag_targets_count(&node->targets) !=
                hdag_targets_count(&keep_node->targets) ||
                (
                    hdag_targets_count(&node->targets) != 0 &&
                    memcmp(hdag_darr_element(
                                &bundle->target_hashes,
                                hdag_node_get_first_ind_idx(node)
                           ),
                           hdag_darr_element(
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
        prev_node = node;
        node = (struct hdag_node *)((uint8_t *)node + node_size);
    }
    /* Truncate the nodes */
    end_node = out_node;
    node_num = ((uint8_t *)end_node - (uint8_t *)bundle->nodes.slots) /
               node_size;
    bundle->nodes.slots_occupied = node_num;

    assert(hdag_bundle_is_valid(bundle));
    res = HDAG_RES_OK;
cleanup:
    return HDAG_RES_ERRNO_IF_INVALID(res);
}

hdag_res
hdag_bundle_dedup(struct hdag_bundle *bundle,
                  const struct hdag_ctx *ctx)
{
    hdag_res res = HDAG_RES_INVALID;
    assert(hdag_bundle_is_valid(bundle));
    assert(!hdag_bundle_is_hashless(bundle));
    assert(hdag_bundle_is_sorted(bundle));
    assert(!hdag_bundle_has_index_targets(bundle));
    assert(ctx == NULL || hdag_ctx_is_valid(ctx));

    /* Dedup edges first, so targets can be compared between nodes */
    HDAG_RES_TRY(hdag_bundle_dedup_edges(bundle, ctx));
    /* Dedup nodes */
    HDAG_RES_TRY(hdag_bundle_dedup_nodes(bundle, ctx));

    assert(hdag_bundle_is_valid(bundle));
    assert(hdag_bundle_is_sorted_and_deduped(bundle));
    res = HDAG_RES_OK;
cleanup:
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
    struct hdag_darr extra_edges =
        HDAG_DARR_EMPTY(sizeof(struct hdag_edge), 64);
    struct hdag_edge *edge;

    assert(hdag_bundle_is_valid(bundle));
    assert(!hdag_bundle_is_hashless(bundle));
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
                edge = hdag_darr_cappend_one(&extra_edges);
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
    extra_edges = HDAG_DARR_VOID;

    assert(hdag_bundle_is_valid(bundle));
    assert(hdag_bundle_is_compacted(bundle));
    res = HDAG_RES_OK;

cleanup:
    hdag_darr_cleanup(&extra_edges);
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

    if (hdag_darr_occupied_slots(&original->nodes) == 0) {
        goto output;
    }

    /* Append uninitialized nodes to inverted bundle */
    if (!hdag_darr_uappend(&inverted.nodes,
                           hdag_darr_occupied_slots(&original->nodes))) {
        goto cleanup;
    }
    /* If we're not dropping hashes */
    if (hdag_darr_occupied_size(&inverted.nodes) ==
        hdag_darr_occupied_size(&original->nodes)) {
        /* Copy all nodes in one go */
        memcpy(inverted.nodes.slots, original->nodes.slots,
               hdag_darr_occupied_size(&inverted.nodes));
    } else {
        /* Copy nodes without hashes, one-by-one */
        HDAG_DARR_ITER_FORWARD(&inverted.nodes, node_idx, inverted_node,
                               (void)0, (void)0) {
            memcpy(inverted_node, HDAG_BUNDLE_NODE(original, node_idx),
                   inverted.nodes.slot_size);
        }
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
hdag_bundle_enumerate_generations(struct hdag_bundle *bundle,
                                  const struct hdag_ctx *ctx)
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
    assert(ctx == NULL || hdag_ctx_is_valid(ctx));

    if (ctx == NULL) {
        ctx = &HDAG_CTX_EMPTY(bundle->hash_len);
    }

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
hdag_bundle_enumerate_components(struct hdag_bundle *bundle,
                                 const struct hdag_ctx *ctx)
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
    assert(ctx == NULL || hdag_ctx_is_valid(ctx));

    if (ctx == NULL) {
        ctx = &HDAG_CTX_EMPTY(bundle->hash_len);
    }

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

    res = HDAG_RES_OK;
cleanup:
    hdag_bundle_cleanup(&inverted);
    return HDAG_RES_ERRNO_IF_INVALID(res);
}

hdag_res
hdag_bundle_enumerate(struct hdag_bundle *bundle, const struct hdag_ctx *ctx)
{
    hdag_res            res      = HDAG_RES_INVALID;

    assert(hdag_bundle_is_valid(bundle));
    assert(hdag_bundle_is_unenumerated(bundle));

    /* Disable profiling */
#undef HDAG_PROFILE_TIME
#define HDAG_PROFILE_TIME(_action, _statement) _statement

    /* Try to enumerate the generations */
    HDAG_PROFILE_TIME(
        "Enumerating the generations",
        HDAG_RES_TRY(hdag_bundle_enumerate_generations(bundle, ctx))
    );

    /* Try to enumerate the components */
    HDAG_PROFILE_TIME(
        "Enumerating the components",
        HDAG_RES_TRY(hdag_bundle_enumerate_components(bundle, ctx))
    );

#undef HDAG_PROFILE_TIME

    assert(hdag_bundle_is_valid(bundle));
    res = HDAG_RES_OK;
cleanup:
    return HDAG_RES_ERRNO_IF_INVALID(res);
}

hdag_res
hdag_bundle_from_node_seq(struct hdag_bundle *pbundle,
                          struct hdag_node_seq *node_seq)
{
    hdag_res                res = HDAG_RES_INVALID;
    struct hdag_bundle      bundle = HDAG_BUNDLE_EMPTY(node_seq->hash_len);
    uint8_t                *node_hash = NULL;
    uint8_t                *target_hash = NULL;
    struct hdag_hash_seq   *target_hash_seq;
    size_t                  first_target_hash_idx;

    assert(hdag_node_seq_is_valid(node_seq));

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
        hdag_node_seq_next(node_seq, node_hash, &target_hash_seq)
    )) {
        first_target_hash_idx = bundle.target_hashes.slots_occupied;

        /* Collect each target hash */
        while (!HDAG_RES_TRY(
            hdag_hash_seq_next(target_hash_seq, target_hash)
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
        bundle = HDAG_BUNDLE_EMPTY(node_seq->hash_len);
    }

    res = HDAG_RES_OK;

cleanup:
    free(target_hash);
    free(node_hash);
    hdag_bundle_cleanup(&bundle);
    return HDAG_RES_ERRNO_IF_INVALID(res);
}

/** A text file node sequence */
struct hdag_bundle_txt_node_seq {
    /** The base abstract node sequence */
    struct hdag_node_seq    base;
    /** The FILE stream containing the text to parse and load */
    FILE                   *stream;
    /** The returned node's target hash sequence */
    struct hdag_hash_seq    target_hash_seq;
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
[[nodiscard]]
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
[[nodiscard]]
static hdag_res
hdag_bundle_txt_hash_seq_next(struct hdag_hash_seq *hash_seq, uint8_t *phash)
{
    hdag_res                    res;
    uint16_t                    hash_len = hash_seq->hash_len;
    struct hdag_bundle_txt_node_seq    *txt_node_seq = HDAG_CONTAINER_OF(
        struct hdag_bundle_txt_node_seq, target_hash_seq, hash_seq
    );
    FILE                       *stream = txt_node_seq->stream;

    assert(hdag_hash_seq_is_valid(hash_seq));
    assert(phash != NULL);
    assert(stream != NULL);

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
[[nodiscard]]
static hdag_res
hdag_bundle_txt_node_seq_next(struct hdag_node_seq     *base_seq,
                              uint8_t                  *phash,
                              struct hdag_hash_seq    **ptarget_hash_seq)
{
    hdag_res                            res;
    uint16_t                            hash_len = base_seq->hash_len;
    struct hdag_bundle_txt_node_seq    *seq = HDAG_CONTAINER_OF(
        struct hdag_bundle_txt_node_seq, base, base_seq
    );
    FILE                               *stream = seq->stream;

    assert(hdag_node_seq_is_valid(base_seq));
    assert(stream != NULL);
    assert(phash != NULL);
    assert(ptarget_hash_seq != NULL);

    /* Skip whitespace (including newlines) and read a hash */
    res = hdag_bundle_txt_read_hash(stream, phash, hash_len, true);
    /* If we failed reading */
    if (hdag_res_is_failure(res)) {
        return res;
    }
    /* Return the stream's target hash sequence */
    *ptarget_hash_seq = &seq->target_hash_seq;
    /* Return OK if we got a hash */
    return res >= hash_len ? 1 : HDAG_RES_OK;
}

hdag_res
hdag_bundle_from_txt(struct hdag_bundle *pbundle,
                     FILE *stream, uint16_t hash_len)
{
    struct hdag_bundle_txt_node_seq seq = {
        .base = {
            .hash_len = hash_len,
            .next_fn = hdag_bundle_txt_node_seq_next,
        },
        .stream = stream,
        .target_hash_seq = {
            .hash_len = hash_len,
            .next_fn = hdag_bundle_txt_hash_seq_next,
        },
    };

    assert(stream != NULL);
    assert(hdag_hash_len_is_valid(hash_len));
    assert(hdag_node_seq_is_valid(&seq.base));
    assert(hdag_hash_seq_is_valid(&seq.target_hash_seq));

    return hdag_bundle_from_node_seq(pbundle, &seq.base);
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
hdag_bundle_organize(struct hdag_bundle *bundle,
                     const struct hdag_ctx *ctx)
{
    hdag_res            res      = HDAG_RES_INVALID;

    assert(hdag_bundle_is_valid(bundle));
    assert(!hdag_bundle_is_hashless(bundle));
    assert(hdag_bundle_is_unorganized(bundle));
    assert(ctx == NULL || hdag_ctx_is_valid(ctx));

    /* Disable profiling */
#undef HDAG_PROFILE_TIME
#define HDAG_PROFILE_TIME(_action, _statement) _statement

    /* Sort the nodes and edges by hash lexicographically */
    HDAG_PROFILE_TIME("Sorting the bundle",
                      hdag_bundle_sort(bundle));

    /* Deduplicate the nodes and edges */
    HDAG_PROFILE_TIME("Deduping the bundle",
                      HDAG_RES_TRY(hdag_bundle_dedup(bundle, ctx)));

    /* Fill in the fanout array */
    HDAG_PROFILE_TIME("Filling in fanout array",
                      hdag_bundle_fanout_fill(bundle));

    /* Compact the edges */
    HDAG_PROFILE_TIME("Compacting the bundle",
                      HDAG_RES_TRY(hdag_bundle_compact(bundle)));

    /* Try to enumerate the bundle's components and generations */
    HDAG_PROFILE_TIME("Enumerating the bundle",
                      HDAG_RES_TRY(hdag_bundle_enumerate(bundle, ctx)));

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
                               const struct hdag_ctx *ctx,
                               FILE *stream, uint16_t hash_len)
{
    hdag_res            res     = HDAG_RES_INVALID;
    struct hdag_bundle  bundle  = HDAG_BUNDLE_EMPTY(hash_len);

    assert(ctx == NULL || hdag_ctx_is_valid(ctx));
    assert(stream != NULL);
    assert(hdag_hash_len_is_valid(hash_len));

    HDAG_RES_TRY(hdag_bundle_from_txt(&bundle, stream, hash_len));
    HDAG_RES_TRY(hdag_bundle_organize(&bundle, ctx));

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
hdag_bundle_organized_from_node_seq(struct hdag_bundle *pbundle,
                                    const struct hdag_ctx *ctx,
                                    struct hdag_node_seq *node_seq)
{
    hdag_res            res      = HDAG_RES_INVALID;
    struct hdag_bundle  bundle  = HDAG_BUNDLE_EMPTY(node_seq->hash_len);

    assert(ctx == NULL || hdag_ctx_is_valid(ctx));
    assert(hdag_node_seq_is_valid(node_seq));

    /* Disable profiling */
#undef HDAG_PROFILE_TIME
#define HDAG_PROFILE_TIME(_action, _statement) _statement

    /* Load the node sequence (adjacency list) */
    HDAG_PROFILE_TIME(
        "Loading node sequence",
        HDAG_RES_TRY(hdag_bundle_from_node_seq(&bundle, node_seq))
    );

    /* Organize */
    HDAG_PROFILE_TIME("Organizing the bundle",
                      HDAG_RES_TRY(hdag_bundle_organize(&bundle, ctx)));

#undef HDAG_PROFILE_TIME

    assert(hdag_bundle_is_valid(&bundle));
    if (pbundle != NULL) {
        *pbundle = bundle;
        bundle = HDAG_BUNDLE_EMPTY(node_seq->hash_len);
    }
    res = HDAG_RES_OK;
cleanup:
    hdag_bundle_cleanup(&bundle);
    return HDAG_RES_ERRNO_IF_INVALID(res);
}
