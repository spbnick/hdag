/*
 * Hash DAG database bundle operations test
 */

#include <hdag/bundle.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#define TEST(_expr) \
    do {                                                \
        if (!(_expr)) {                                 \
            fprintf(stderr, "%s:%u: Test failed: %s\n", \
                    __FILE__, __LINE__, #_expr);        \
            failed++;                                   \
        }                                               \
    } while(0)

static size_t
test_deduplicating(uint16_t hash_len)
{
    size_t failed = 0;
    struct hdag_bundle bundle = HDAG_BUNDLE_EMPTY(hash_len);
    ssize_t idx;
    struct hdag_node *node;
    ssize_t hash_idx;

    /* Create sixteen zeroed nodes */
    hdag_darr_cappend(&bundle.nodes, 16);

    /* Check basic node deduplication works */
    HDAG_DARR_ITER_FORWARD(&bundle.nodes, idx, node, (void)0, (void)0) {
        hdag_node_hash_fill(node, hash_len, (idx >> 1));
    }
    assert(bundle.nodes.slots_occupied == 16);
    TEST(hdag_bundle_is_sorted(&bundle));
    TEST(!hdag_bundle_is_sorted_and_deduped(&bundle));
    hdag_bundle_dedup(&bundle);
    TEST(bundle.nodes.slots_occupied == 8);
    HDAG_DARR_ITER_FORWARD(&bundle.nodes, idx, node, (void)0, (void)0) {
        TEST(hdag_node_hash_is_filled(node, hash_len, idx));
    }
    TEST(hdag_bundle_is_sorted(&bundle));
    TEST(hdag_bundle_is_sorted_and_deduped(&bundle));

    /* Check deduplicating single-node bundle works */
    hdag_darr_remove(&bundle.nodes, 1, bundle.nodes.slots_occupied);
    TEST(bundle.nodes.slots_occupied == 1);
    hdag_bundle_dedup(&bundle);
    TEST(bundle.nodes.slots_occupied == 1);
    hdag_node_hash_is_filled(hdag_darr_element(&bundle.nodes, 0),
                             hash_len, 0);
    TEST(hdag_bundle_is_sorted(&bundle));
    TEST(hdag_bundle_is_sorted_and_deduped(&bundle));

    /* Check deduplicating a 64-node bundle to a single-node bundle works */
    hdag_darr_empty(&bundle.nodes);
    hdag_darr_cappend(&bundle.nodes, 64);
    /* Make sure there's no more space allocated */
    hdag_darr_deflate(&bundle.nodes);
    TEST(bundle.nodes.slots_occupied == 64);
    hdag_bundle_dedup(&bundle);
    TEST(bundle.nodes.slots_occupied == 1);
    hdag_node_hash_is_filled(hdag_darr_element(&bundle.nodes, 0),
                             hash_len, 0);
    TEST(hdag_bundle_is_sorted(&bundle));
    TEST(hdag_bundle_is_sorted_and_deduped(&bundle));

    /* Empty the bundle */
    hdag_bundle_empty(&bundle);

    /*
     * Check duplicate node priority is handled correctly
     */
    idx = 0;
    hash_idx = 0;

#define APPEND_NODE_UNKNOWN \
    do {                                                \
        node = hdag_darr_cappend_one(&bundle.nodes);    \
        hdag_node_hash_fill(node, hash_len, hash_idx);  \
        node->component = idx++;                        \
        node->targets = HDAG_TARGETS_UNKNOWN;           \
    } while(0)

#define APPEND_NODE_ABSENT \
    do {                                                \
        node = hdag_darr_cappend_one(&bundle.nodes);    \
        hdag_node_hash_fill(node, hash_len, hash_idx);  \
        node->component = idx++;                        \
        node->targets = HDAG_TARGETS_ABSENT;            \
    } while(0)

    APPEND_NODE_UNKNOWN;
    APPEND_NODE_UNKNOWN;
    hash_idx++;
    APPEND_NODE_ABSENT;
    APPEND_NODE_ABSENT;
    hash_idx++;
    APPEND_NODE_ABSENT;
    APPEND_NODE_UNKNOWN;
    hash_idx++;
    APPEND_NODE_UNKNOWN;
    APPEND_NODE_ABSENT;
    hash_idx++;
    APPEND_NODE_ABSENT;
    APPEND_NODE_UNKNOWN;
    APPEND_NODE_ABSENT;
    hash_idx++;
    APPEND_NODE_UNKNOWN;
    APPEND_NODE_ABSENT;
    APPEND_NODE_UNKNOWN;
    hash_idx++;
    APPEND_NODE_UNKNOWN;
    APPEND_NODE_UNKNOWN;
    APPEND_NODE_ABSENT;
    hash_idx++;
    APPEND_NODE_ABSENT;
    APPEND_NODE_ABSENT;
    APPEND_NODE_UNKNOWN;
    hash_idx++;
    APPEND_NODE_UNKNOWN;
    APPEND_NODE_ABSENT;
    APPEND_NODE_ABSENT;
    hash_idx++;
    APPEND_NODE_ABSENT;
    APPEND_NODE_UNKNOWN;
    APPEND_NODE_UNKNOWN;
    hash_idx++;

    assert(bundle.nodes.slots_occupied == 26);
    hdag_bundle_dedup(&bundle);
    TEST(bundle.nodes.slots_occupied == 10);

#define GET_NODE_COMPONENT(_idx) \
    ((struct hdag_node *)hdag_darr_element(&bundle.nodes, (_idx)))->component

    TEST(GET_NODE_COMPONENT(0) == 0);
    TEST(GET_NODE_COMPONENT(1) == 3);
    TEST(GET_NODE_COMPONENT(2) == 4);
    TEST(GET_NODE_COMPONENT(3) == 7);
    TEST(GET_NODE_COMPONENT(4) == 10);
    TEST(GET_NODE_COMPONENT(5) == 12);
    TEST(GET_NODE_COMPONENT(6) == 16);
    TEST(GET_NODE_COMPONENT(7) == 18);
    TEST(GET_NODE_COMPONENT(8) == 22);
    TEST(GET_NODE_COMPONENT(9) == 23);

    /* Empty the bundle */
    hdag_bundle_empty(&bundle);

    /*
     * Check edge deduping works
     */

    /*
     * Create a bundle with various combinations of duplicate edges:
     *
     * 0 - 1 - 2 - 4 - 6 - 8
     *                   - 8
     *                   - 8
     *               - 6
     *               - 7
     *           - 5
     *           - 4
     *       - 3
     *       - 3
     *   - 1
     */
    /* Two same edges */
    node = hdag_darr_cappend_one(&bundle.nodes);
    hdag_node_hash_fill(node, hash_len, 0);
    node->targets.first =
        hdag_target_from_ind_idx(bundle.target_hashes.slots_occupied);
    hdag_hash_fill(hdag_darr_cappend_one(&bundle.target_hashes),
                   hash_len, 1);
    hdag_hash_fill(hdag_darr_cappend_one(&bundle.target_hashes),
                   hash_len, 1);
    node->targets.last =
        hdag_target_from_ind_idx(bundle.target_hashes.slots_occupied - 1);

    /* Edge a, edge b, edge b */
    node = hdag_darr_cappend_one(&bundle.nodes);
    hdag_node_hash_fill(node, hash_len, 1);
    node->targets.first =
        hdag_target_from_ind_idx(bundle.target_hashes.slots_occupied);
    hdag_hash_fill(hdag_darr_cappend_one(&bundle.target_hashes),
                   hash_len, 2);
    hdag_hash_fill(hdag_darr_cappend_one(&bundle.target_hashes),
                   hash_len, 3);
    hdag_hash_fill(hdag_darr_cappend_one(&bundle.target_hashes),
                   hash_len, 3);
    node->targets.last =
        hdag_target_from_ind_idx(bundle.target_hashes.slots_occupied - 1);

    /* Edge a, edge b, edge a */
    node = hdag_darr_cappend_one(&bundle.nodes);
    hdag_node_hash_fill(node, hash_len, 2);
    node->targets.first =
        hdag_target_from_ind_idx(bundle.target_hashes.slots_occupied);
    hdag_hash_fill(hdag_darr_cappend_one(&bundle.target_hashes),
                   hash_len, 4);
    hdag_hash_fill(hdag_darr_cappend_one(&bundle.target_hashes),
                   hash_len, 5);
    hdag_hash_fill(hdag_darr_cappend_one(&bundle.target_hashes),
                   hash_len, 4);
    node->targets.last =
        hdag_target_from_ind_idx(bundle.target_hashes.slots_occupied - 1);
    node = hdag_darr_cappend_one(&bundle.nodes);
    hdag_node_hash_fill(node, hash_len, 3);

    /* Edge a, edge a, edge b */
    node = hdag_darr_cappend_one(&bundle.nodes);
    hdag_node_hash_fill(node, hash_len, 4);
    node->targets.first =
        hdag_target_from_ind_idx(bundle.target_hashes.slots_occupied);
    hdag_hash_fill(hdag_darr_cappend_one(&bundle.target_hashes),
                   hash_len, 6);
    hdag_hash_fill(hdag_darr_cappend_one(&bundle.target_hashes),
                   hash_len, 6);
    hdag_hash_fill(hdag_darr_cappend_one(&bundle.target_hashes),
                   hash_len, 7);
    node->targets.last =
        hdag_target_from_ind_idx(bundle.target_hashes.slots_occupied - 1);
    node = hdag_darr_cappend_one(&bundle.nodes);
    hdag_node_hash_fill(node, hash_len, 5);

    /* Three same edges */
    node = hdag_darr_cappend_one(&bundle.nodes);
    hdag_node_hash_fill(node, hash_len, 6);
    node->targets.first =
        hdag_target_from_ind_idx(bundle.target_hashes.slots_occupied);
    hdag_hash_fill(hdag_darr_cappend_one(&bundle.target_hashes),
                   hash_len, 8);
    hdag_hash_fill(hdag_darr_cappend_one(&bundle.target_hashes),
                   hash_len, 8);
    hdag_hash_fill(hdag_darr_cappend_one(&bundle.target_hashes),
                   hash_len, 8);
    node->targets.last =
        hdag_target_from_ind_idx(bundle.target_hashes.slots_occupied - 1);
    node = hdag_darr_cappend_one(&bundle.nodes);
    hdag_node_hash_fill(node, hash_len, 7);
    node = hdag_darr_cappend_one(&bundle.nodes);
    hdag_node_hash_fill(node, hash_len, 8);

    /* Check we have the expected number of nodes and target hashes */
    TEST(bundle.nodes.slots_occupied == 9);
    TEST(bundle.target_hashes.slots_occupied == 14);

    /* Sort and dedup */
    hdag_bundle_sort(&bundle);
    hdag_bundle_dedup(&bundle);

    /* Check the expected results:
     *
     * 0 - 1 - 2 - 4 - 6 - 8
     *               - 7
     *           - 5
     *       - 3
     */
    TEST(bundle.nodes.slots_occupied == 9);
    TEST(bundle.target_hashes.slots_occupied == 14);
    HDAG_DARR_ITER_FORWARD(&bundle.nodes, idx, node, (void)0, (void)0) {
        TEST(hdag_node_hash_is_filled(node, hash_len, idx));
    }
    node = hdag_darr_element(&bundle.nodes, 0);
    TEST(hdag_targets_are_indirect(&node->targets));
    TEST(hdag_target_to_ind_idx(node->targets.first) == 0);
    TEST(hdag_target_to_ind_idx(node->targets.last) == 0);
    TEST(hdag_hash_is_filled(hdag_darr_element(&bundle.target_hashes, 0),
                             hash_len, 1));

    node = hdag_darr_element(&bundle.nodes, 1);
    TEST(hdag_targets_are_indirect(&node->targets));
    TEST(hdag_target_to_ind_idx(node->targets.first) == 2);
    TEST(hdag_target_to_ind_idx(node->targets.last) == 3);
    TEST(hdag_hash_is_filled(hdag_darr_element(&bundle.target_hashes, 2),
                             hash_len, 2));
    TEST(hdag_hash_is_filled(hdag_darr_element(&bundle.target_hashes, 3),
                             hash_len, 3));

    node = hdag_darr_element(&bundle.nodes, 2);
    TEST(hdag_targets_are_indirect(&node->targets));
    TEST(hdag_target_to_ind_idx(node->targets.first) == 5);
    TEST(hdag_target_to_ind_idx(node->targets.last) == 6);
    TEST(hdag_hash_is_filled(hdag_darr_element(&bundle.target_hashes, 5),
                             hash_len, 4));
    TEST(hdag_hash_is_filled(hdag_darr_element(&bundle.target_hashes, 6),
                             hash_len, 5));

    node = hdag_darr_element(&bundle.nodes, 3);
    TEST(hdag_targets_are_absent(&node->targets));

    node = hdag_darr_element(&bundle.nodes, 4);
    TEST(hdag_targets_are_indirect(&node->targets));
    TEST(hdag_target_to_ind_idx(node->targets.first) == 8);
    TEST(hdag_target_to_ind_idx(node->targets.last) == 9);
    TEST(hdag_hash_is_filled(hdag_darr_element(&bundle.target_hashes, 8),
                             hash_len, 6));
    TEST(hdag_hash_is_filled(hdag_darr_element(&bundle.target_hashes, 9),
                             hash_len, 7));

    node = hdag_darr_element(&bundle.nodes, 5);
    TEST(hdag_targets_are_absent(&node->targets));

    node = hdag_darr_element(&bundle.nodes, 6);
    TEST(hdag_targets_are_indirect(&node->targets));
    TEST(hdag_target_to_ind_idx(node->targets.first) == 11);
    TEST(hdag_target_to_ind_idx(node->targets.last) == 11);
    TEST(hdag_hash_is_filled(hdag_darr_element(&bundle.target_hashes, 11),
                             hash_len, 8));

    node = hdag_darr_element(&bundle.nodes, 7);
    TEST(hdag_targets_are_absent(&node->targets));

    node = hdag_darr_element(&bundle.nodes, 8);
    TEST(hdag_targets_are_absent(&node->targets));

    /* Cleanup the bundle */
    hdag_bundle_cleanup(&bundle);

    return failed;
}

static size_t
test_compacting(uint16_t hash_len)
{
    size_t failed = 0;
    struct hdag_bundle bundle = HDAG_BUNDLE_EMPTY(hash_len);
    ssize_t idx;
    struct hdag_node *node;
    ssize_t hash_idx;
    uint8_t *hash_ptr;

    /* Create a directed path */
    hdag_darr_cappend(&bundle.nodes, 16);
    hdag_darr_cappend(&bundle.target_hashes, 15);
    HDAG_DARR_ITER_FORWARD(&bundle.nodes, idx, node, (void)0, (void)0) {
        hdag_node_hash_fill(node, hash_len, 15 - idx);
        if (idx <= 14) {
            hdag_hash_fill(hdag_darr_element(&bundle.target_hashes, idx),
                           hash_len, 14 - idx);
            node->targets.first = hdag_target_from_ind_idx(idx);
            node->targets.last = hdag_target_from_ind_idx(idx);
        }
        assert(hdag_node_is_valid(node));
    }
    TEST(!hdag_bundle_is_sorted(&bundle));
    hdag_bundle_sort(&bundle);
    TEST(hdag_bundle_is_sorted(&bundle));
    TEST(hdag_bundle_is_sorted_and_deduped(&bundle));
    hdag_bundle_dedup(&bundle);
    TEST(hdag_bundle_is_sorted_and_deduped(&bundle));
    hdag_bundle_compact(&bundle);
    TEST(hdag_bundle_is_indexed(&bundle));
    HDAG_DARR_ITER_FORWARD(&bundle.nodes, idx, node, (void)0, (void)0) {
        TEST(hdag_node_is_valid(node));
        TEST(hdag_node_hash_is_filled(node, hash_len, idx));
        if (idx == 0) {
            TEST(hdag_targets_are_known(&node->targets));
            TEST(node->targets.first == HDAG_TARGET_ABSENT);
            TEST(node->targets.last == HDAG_TARGET_ABSENT);
        } else {
            TEST(hdag_targets_are_known(&node->targets));
            TEST(hdag_targets_are_direct(&node->targets));
            TEST(hdag_target_is_dir_idx(node->targets.first));
            TEST(node->targets.last == HDAG_TARGET_ABSENT);
            TEST(hdag_target_to_dir_idx(node->targets.first) ==
                 (size_t)idx - 1);
        }
    }

    /* Empty the bundle */
    hdag_bundle_empty(&bundle);

    /*
     * Check unknown nodes are handled correctly, and extra edges are created
     */
    hdag_darr_cappend(&bundle.nodes, 9);
    hdag_darr_cappend(&bundle.target_hashes, 8);
    HDAG_DARR_ITER_FORWARD(&bundle.nodes, idx, node, (void)0, (void)0) {
        hdag_node_hash_fill(node, hash_len, idx);
        if (idx == 0) {
            node->targets.first = hdag_target_from_ind_idx(0);
            node->targets.last = hdag_target_from_ind_idx(
                bundle.target_hashes.slots_occupied - 1
            );
            HDAG_DARR_ITER_FORWARD(&bundle.target_hashes,
                                   hash_idx, hash_ptr, (void)0, (void)0) {
                hdag_hash_fill(hash_ptr, hash_len, hash_idx + 1);
            }
        } else if (idx & 1) {
            node->targets.first = HDAG_TARGET_UNKNOWN;
            node->targets.last = HDAG_TARGET_UNKNOWN;
        } else {
            node->targets.first = HDAG_TARGET_ABSENT;
            node->targets.last = HDAG_TARGET_ABSENT;
        }
        assert(hdag_node_is_valid(node));
    }
    TEST(bundle.nodes.slots_occupied == 9);
    TEST(bundle.target_hashes.slots_occupied == 8);
    TEST(bundle.extra_edges.slots_occupied == 0);
    hdag_bundle_sort(&bundle);
    TEST(bundle.nodes.slots_occupied == 9);
    TEST(bundle.target_hashes.slots_occupied == 8);
    TEST(bundle.extra_edges.slots_occupied == 0);
    hdag_bundle_dedup(&bundle);
    TEST(bundle.nodes.slots_occupied == 9);
    TEST(bundle.target_hashes.slots_occupied == 8);
    TEST(bundle.extra_edges.slots_occupied == 0);
    hdag_bundle_compact(&bundle);
    TEST(bundle.nodes.slots_occupied == 9);
    TEST(bundle.target_hashes.slots_occupied == 0);
    TEST(bundle.extra_edges.slots_occupied == 8);
    hdag_bundle_empty(&bundle);

    /* Cleanup the bundle */
    hdag_bundle_cleanup(&bundle);

    return failed;
}

static size_t
test(uint16_t hash_len)
{
    size_t failed = 0;
    const struct hdag_bundle empty_bundle = HDAG_BUNDLE_EMPTY(hash_len);
    struct hdag_bundle compacted_empty_bundle = HDAG_BUNDLE_EMPTY(hash_len);
    compacted_empty_bundle.ind_extra_edges = true;
    struct hdag_bundle bundle;
    ssize_t idx;
    struct hdag_node *node;

    TEST(hdag_bundle_is_valid(&empty_bundle));
    TEST(hdag_bundle_is_clean(&empty_bundle));
    TEST(hdag_bundle_is_sorted(&empty_bundle));
    TEST(hdag_bundle_is_sorted_and_deduped(&empty_bundle));
    TEST(!hdag_bundle_is_indexed(&empty_bundle));

    bundle = empty_bundle;
    hdag_bundle_dedup(&bundle);
    TEST(memcmp(&bundle, &empty_bundle, sizeof(struct hdag_bundle)) == 0);

    bundle = empty_bundle;
    hdag_bundle_compact(&bundle);
    TEST(memcmp(&bundle, &compacted_empty_bundle,
                sizeof(struct hdag_bundle)) == 0);
    TEST(bundle.ind_extra_edges);

    hdag_bundle_cleanup(&bundle);
    TEST(memcmp(&bundle, &empty_bundle, sizeof(struct hdag_bundle)) == 0);

    /* Create sixteen zeroed nodes */
    hdag_darr_cappend(&bundle.nodes, 16);

    /* Check sorting works */
    HDAG_DARR_ITER_FORWARD(&bundle.nodes, idx, node, (void)0, (void)0) {
        hdag_node_hash_fill(node, hash_len, 15 - idx);
    }
    TEST(!hdag_bundle_is_sorted(&bundle));
    hdag_bundle_sort(&bundle);
    TEST(hdag_bundle_is_sorted(&bundle));
    TEST(hdag_bundle_is_sorted_and_deduped(&bundle));
    HDAG_DARR_ITER_FORWARD(&bundle.nodes, idx, node, (void)0, (void)0) {
        TEST(hdag_node_hash_is_filled(node, hash_len, idx));
    }

    /* Check sorting doesn't deduplicate */
    HDAG_DARR_ITER_FORWARD(&bundle.nodes, idx, node, (void)0, (void)0) {
        hdag_node_hash_fill(node, hash_len, 7 - (idx >> 1));
    }
    hdag_bundle_sort(&bundle);
    HDAG_DARR_ITER_FORWARD(&bundle.nodes, idx, node, (void)0, (void)0) {
        TEST(hdag_node_hash_is_filled(node, hash_len, idx >> 1));
    }
    TEST(hdag_bundle_is_sorted(&bundle));
    TEST(!hdag_bundle_is_sorted_and_deduped(&bundle));

    /*
     * Check deduplicating works.
     */
    failed += test_deduplicating(hash_len);

    /*
     * Check compacting works.
     */
    failed += test_compacting(hash_len);

    /* Cleanup the bundle */
    hdag_bundle_cleanup(&bundle);

    return failed;
}

int
main(void)
{
    size_t failed = test(32);
    if (failed) {
        fprintf(stderr, "%zu tests failed.\n", failed);
    }
    return failed != 0;
}
