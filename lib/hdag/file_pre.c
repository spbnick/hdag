/*
 * Hash DAG file preprocessing
 */

#include <hdag/file_pre.h>
#include <hdag/nodes.h>
#include <hdag/hash.h>
#include <string.h>


/**
 * Make sure an allocated array has enough space to append another element, by
 * reallocating it to twice the current size when it's full.
 *
 * @param parray        Location of and for the array pointer.
 * @param pamemb        Location of and for the number of allocated member
 *                      slots.
 * @param init_amemb    Number of member slots to allocate initially, when the
 *                      array is zero size.
 * @param nmemb         The number of currently-occupied member slots.
 * @param size          The member size in bytes.
 *
 * @return True if there was enough space, or if the reallocation succeeded.
 *         False if it failed. The errno is set in case of failure and parray
 *         and pamemb locations are not changed.
 */
static bool
hdag_file_pre_make_space(void **parray, size_t *pamemb, size_t init_amemb,
                         size_t nmemb, size_t size)
{
    assert(parray != NULL);
    assert(pamemb != NULL);
    assert(nmemb <= *pamemb);
    assert(size != 0);

    if (nmemb == *pamemb) {
        size_t new_amemb = (*pamemb << 1) || init_amemb;
        void *new_array = reallocarray(*parray, new_amemb, size);
        if (new_array == NULL) {
            return false;
        }
        *pamemb = new_amemb;
        *parray = new_array;
    }

    return true;
}

/**
 * Remove any extra space allocated for an array.
 *
 * @param parray        Location of and for the array pointer.
 * @param pamemb        Location of and for the number of allocated member
 *                      slots.
 * @param nmemb         The number of currently-occupied member slots.
 * @param size          The member size in bytes.
 *
 * @return True if there was no extra space, or if the reallocation succeeded.
 *         False if it failed. The errno is set in case of failure and parray
 *         and pamemb locations are not changed.
 */
static bool
hdag_file_pre_shrink_space(void **parray, size_t *pamemb,
                           size_t nmemb, size_t size)
{
    assert(parray != NULL);
    assert(pamemb != NULL);
    assert(nmemb <= *pamemb);
    assert(size != 0);

    if (nmemb != *pamemb) {
        size_t new_amemb = nmemb;
        void *new_array = reallocarray(*parray, new_amemb, size);
        if (new_amemb != 0 && new_array == NULL) {
            return false;
        }
        *pamemb = new_amemb;
        *parray = new_array;
    }

    return true;
}

void
hdag_file_pre_cleanup(struct hdag_file_pre *file_pre)
{
    free(file_pre->nodes);
    free(file_pre->target_hashes);
    free(file_pre->extra_edges);
    *file_pre = HDAG_FILE_PRE_EMPTY;
}

/**
 * Remove duplicate node entries, preferring known ones, from a preprocessed
 * file, assuming nodes are sorted by hash.
 *
 * @param file_pre  The preprocessed file to deduplicate nodes in.
 */
static void
hdag_file_pre_dedup(struct hdag_file_pre *file_pre)
{
    assert(file_pre != NULL);
    assert(hdag_hash_len_is_valid(file_pre->hash_len));

#define NODE_OFF(_node, _off) hdag_node_off(_node, file_pre->hash_len, _off)

#define NODE(_idx) NODE_OFF(file_pre->nodes, _idx)

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
    for (idx = (ssize_t)file_pre->nodes_num - 1, node = NODE(idx),
         first_idx = idx, first_node = node;
         idx >= 0;
         idx--, prev_node = node, node = NODE_OFF(node, -1)) {
        assert(hdag_node_is_valid(node));
        relation = memcmp(node->hash, first_node->hash, file_pre->hash_len);
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
                        file_pre->nodes_num - (first_idx + 1));
                file_pre->nodes_num -= first_idx - (idx + 1);
            }
            first_idx = idx;
            first_node = node;
        }
    }
#undef NODE
#undef NODE_OFF
}

/**
 * Compact edge targets into nodes, putting the rest into "extra edges",
 * assuming the nodes are sorted and de-duplicated.
 *
 * @param file_pre  The preprocessed file to compact edges in.
 *
 * @return True if compaction succeeded, false if it failed.
 *         Errno is set in case of failure.
 */
static bool
hdag_file_pre_compact(struct hdag_file_pre *file_pre)
{
    /* Minimal number of extra edges to allocate */
    const size_t init_extra_edges_allocated = 64;
    /* The index of the currently-traversed node */
    size_t idx;
    /* A hash index */
    size_t hash_idx;
    /* The index of a found node */
    size_t found_idx;
    /* Currently traversed node */
    struct hdag_node *node;

    assert(file_pre != NULL);
    assert(hdag_hash_len_is_valid(file_pre->hash_len));

    /* For each node, from start to end */
    for (idx = 0, node = file_pre->nodes;
         idx < file_pre->nodes_num;
         idx++, node = hdag_node_off(node, file_pre->hash_len, 1)) {
        assert(hdag_node_is_valid(node));
        /* If the node's targets are unknown */
        if (hdag_targets_are_unknown(&node->targets)) {
            continue;
        }
        /* If there's more than two targets */
        if (node->targets.last - node->targets.first > 1) {
            size_t first_extra_edge_idx = file_pre->extra_edges_num;

            /* Convert all target hashes to extra edges */
            for (hash_idx = hdag_target_to_ind_idx(node->targets.first);
                 hash_idx <= hdag_target_to_ind_idx(node->targets.last);
                 hash_idx++) {
                found_idx = hdag_nodes_find(
                    file_pre->nodes, file_pre->nodes_num, file_pre->hash_len,
                    file_pre->target_hashes + file_pre->hash_len * hash_idx
                );
                /* All hashes must be locatable */
                assert(found_idx < INT32_MAX);
                /* Hash indices must be valid */
                assert(found_idx < file_pre->target_hashes_num);

                /* Make space for another edge */
                if (!hdag_file_pre_make_space(
                        (void **)&file_pre->extra_edges,
                        &file_pre->extra_edges_allocated,
                        init_extra_edges_allocated,
                        file_pre->extra_edges_num,
                        sizeof(file_pre->extra_edges)
                )) {
                    return false;
                }

                /* Store the edge */
                file_pre->extra_edges[
                    file_pre->extra_edges_num
                ].node_idx = found_idx;
                file_pre->extra_edges_num++;
            }

            /* Store the extra edge indices */
            node->targets.first =
                hdag_target_from_ind_idx(first_extra_edge_idx);
            node->targets.last =
                hdag_target_from_ind_idx(file_pre->extra_edges_num - 1);
        } else {
            /* Store targets inside the node */
            hash_idx = hdag_target_to_ind_idx(node->targets.first);
            found_idx = hdag_nodes_find(
                file_pre->nodes, file_pre->nodes_num, file_pre->hash_len,
                file_pre->target_hashes + file_pre->hash_len * hash_idx
            );
            /* All hashes must be locatable */
            assert(found_idx < INT32_MAX);
            /* Hash indices must be valid */
            assert(found_idx < file_pre->target_hashes_num);
            /* Store the first target */
            node->targets.first = hdag_target_from_ind_idx(found_idx);

            /* If there's a second target */
            if (node->targets.last > node->targets.first) {
                hash_idx = hdag_target_to_ind_idx(node->targets.first);
                found_idx = hdag_nodes_find(
                    file_pre->nodes, file_pre->nodes_num, file_pre->hash_len,
                    file_pre->target_hashes + file_pre->hash_len * hash_idx
                );
                /* All hashes must be locatable */
                assert(found_idx < INT32_MAX);
                /* Hash indices must be valid */
                assert(found_idx < file_pre->target_hashes_num);
                /* Store the last target */
                node->targets.last = hdag_target_from_ind_idx(found_idx);
            }
        }
    }

    return true;
}

bool
hdag_file_pre_create(struct hdag_file_pre *pfile_pre,
                     uint16_t hash_len,
                     struct hdag_node_seq node_seq)
{
    bool                    result = false;
    const size_t            init_nodes_allocated = 64;
    const size_t            init_target_hashes_allocated = 64;

    struct hdag_file_pre    file_pre = HDAG_FILE_PRE_EMPTY;
    const size_t            node_size = hdag_node_size(hash_len);

    uint8_t                *node_hash = NULL;
    uint8_t                *target_hash = NULL;
    struct hdag_hash_seq    target_hash_seq;

    int                     seq_rc;

    size_t                  first_target_hash_idx;

    file_pre.hash_len = hash_len;

    node_hash = malloc(file_pre.hash_len);
    if (node_hash == NULL) {
        goto cleanup;
    }
    target_hash = malloc(file_pre.hash_len);
    if (target_hash == NULL) {
        goto cleanup;
    }

    /* Add a new node */
    #define ADD_NODE(_hash, _first_target, _last_target) \
        do {                                                \
            if (!hdag_file_pre_make_space(                  \
                    (void **)&file_pre.nodes,               \
                    &file_pre.nodes_allocated,              \
                    init_nodes_allocated,                   \
                    file_pre.nodes_num, node_size           \
            )) {                                            \
                goto cleanup;                               \
            }                                               \
                                                            \
            struct hdag_node *_node = (struct hdag_node *)( \
                (uint8_t *)file_pre.nodes +                 \
                file_pre.nodes_num * node_size              \
            );                                              \
                                                            \
            memcpy(_node->hash, _hash, file_pre.hash_len);  \
            _node->targets.first = _first_target;           \
            _node->targets.last = _last_target;             \
            file_pre.nodes_num++;                           \
        } while (0)

    /* Add a new target hash (and the corresponding node) */
    #define ADD_TARGET_HASH(_hash) \
        do {                                                \
            if (!hdag_file_pre_make_space(                  \
                    (void **)&file_pre.target_hashes,       \
                    &file_pre.target_hashes_allocated,      \
                    init_target_hashes_allocated,           \
                    file_pre.target_hashes_num,             \
                    file_pre.hash_len                       \
            )) {                                            \
                goto cleanup;                               \
            }                                               \
            memcpy(file_pre.target_hashes +                 \
                   file_pre.hash_len *                      \
                    file_pre.target_hashes_num,             \
                   _hash, file_pre.hash_len);               \
            file_pre.target_hashes_num++;                   \
            ADD_NODE(_hash,                                 \
                     HDAG_TARGET_UNKNOWN,                   \
                     HDAG_TARGET_UNKNOWN);                  \
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

        first_target_hash_idx = file_pre.target_hashes_num;

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
        if (first_target_hash_idx == file_pre.target_hashes_num) {
            ADD_NODE(node_hash, HDAG_TARGET_INVALID, HDAG_TARGET_INVALID);
        } else {
            ADD_NODE(
                node_hash,
                hdag_target_from_ind_idx(first_target_hash_idx),
                hdag_target_from_ind_idx(file_pre.target_hashes_num - 1)
            );
        }
    }

    #undef MAKE_TARGET_HASH_SPACE
    #undef MAKE_NODE_SPACE

    /* Sort the nodes by hash lexicographically */
    qsort_r(file_pre.nodes, file_pre.nodes_num, node_size,
            hdag_node_cmp, &file_pre.hash_len);

    /* Deduplicate the nodes */
    hdag_file_pre_dedup(&file_pre);

    /* Attempt to compact the edges */
    if (!hdag_file_pre_compact(&file_pre)) {
        goto cleanup;
    }

    /* Hand over the preprocessed file, if requested */
    if (pfile_pre != NULL) {
        if (!hdag_file_pre_shrink_space((void **)&file_pre.nodes,
                                        &file_pre.nodes_allocated,
                                        file_pre.nodes_num, node_size)) {
            goto cleanup;
        }
        if (!hdag_file_pre_shrink_space((void **)&file_pre.target_hashes,
                                        &file_pre.target_hashes_allocated,
                                        file_pre.target_hashes_num,
                                        file_pre.hash_len)) {
            goto cleanup;
        }
        *pfile_pre = file_pre;
        file_pre = HDAG_FILE_PRE_EMPTY;
    }

    result = true;

cleanup:
    hdag_file_pre_cleanup(&file_pre);
    free(target_hash);
    free(node_hash);
    return result;
}
