/*
 * Hash DAG database bundle operations test
 */

#include <hdag/bundle.h>
#include <hdag/misc.h>
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
test_sorting_and_deduplicating(uint16_t hash_len)
{
    size_t failed = 0;
    struct hdag_bundle bundle = HDAG_BUNDLE_EMPTY(hash_len);
    ssize_t idx;
    struct hdag_node *node;
    ssize_t hash_idx;

    /* Check sorting and deduplicating empty bundle works */
    TEST(hdag_bundle_sort_and_dedup(&bundle, false) == HDAG_RES_OK);

    /* Create sixteen zeroed nodes */
    TEST(hdag_darr_cappend(&bundle.nodes, 16));

    /* Check basic node deduplication works */
    HDAG_DARR_ITER_FORWARD(&bundle.nodes, idx, node, (void)0, (void)0) {
        hdag_node_hash_fill(node, hash_len, (idx >> 1));
    }
    assert(bundle.nodes.slots_occupied == 16);
    TEST(!hdag_bundle_is_sorted_and_deduped(&bundle));
    TEST(hdag_bundle_sort_and_dedup(&bundle, false) == HDAG_RES_OK);
    TEST(bundle.nodes.slots_occupied == 8);
    TEST(bundle.unknown_indexes.slots_occupied == 0);
    HDAG_DARR_ITER_FORWARD(&bundle.nodes, idx, node, (void)0, (void)0) {
        TEST(hdag_node_hash_is_filled(node, hash_len, idx));
    }
    TEST(hdag_bundle_is_sorted_and_deduped(&bundle));

    /* Check deduplicating single-node bundle works */
    hdag_darr_remove(&bundle.nodes, 1, bundle.nodes.slots_occupied);
    TEST(bundle.nodes.slots_occupied == 1);
    TEST(hdag_bundle_sort_and_dedup(&bundle, false) == HDAG_RES_OK);
    TEST(bundle.nodes.slots_occupied == 1);
    TEST(bundle.unknown_indexes.slots_occupied == 0);
    hdag_node_hash_is_filled(hdag_darr_element(&bundle.nodes, 0),
                             hash_len, 0);
    TEST(hdag_bundle_is_sorted_and_deduped(&bundle));

    /* Check deduplicating a 64-node bundle to a single-node bundle works */
    hdag_darr_empty(&bundle.nodes);
    TEST(hdag_darr_cappend(&bundle.nodes, 64));
    /* Make sure there's no more space allocated */
    TEST(hdag_darr_deflate(&bundle.nodes));
    TEST(bundle.nodes.slots_occupied == 64);
    TEST(hdag_bundle_sort_and_dedup(&bundle, false) == HDAG_RES_OK);
    TEST(bundle.nodes.slots_occupied == 1);
    TEST(bundle.unknown_indexes.slots_occupied == 0);
    hdag_node_hash_is_filled(hdag_darr_element(&bundle.nodes, 0),
                             hash_len, 0);
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
        TEST(node);                                     \
        hdag_node_hash_fill(node, hash_len, hash_idx);  \
        node->component = idx++;                        \
        node->targets = HDAG_TARGETS_UNKNOWN;           \
    } while(0)

#define APPEND_NODE_ABSENT \
    do {                                                \
        node = hdag_darr_cappend_one(&bundle.nodes);    \
        TEST(node);                                     \
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
    TEST(hdag_bundle_sort_and_dedup(&bundle, false) == HDAG_RES_OK);
    TEST(bundle.nodes.slots_occupied == 10);

#define GET_NODE_COMPONENT(_idx) \
    ((struct hdag_node *)hdag_darr_element(&bundle.nodes, (_idx)))->component

    TEST(GET_NODE_COMPONENT(0) == 1);
    TEST(GET_NODE_COMPONENT(1) == 2);
    TEST(GET_NODE_COMPONENT(2) == 4);
    TEST(GET_NODE_COMPONENT(3) == 7);
    TEST(GET_NODE_COMPONENT(4) == 8);
    TEST(GET_NODE_COMPONENT(5) == 12);
    TEST(GET_NODE_COMPONENT(6) == 16);
    TEST(GET_NODE_COMPONENT(7) == 17);
    TEST(GET_NODE_COMPONENT(8) == 21);
    TEST(GET_NODE_COMPONENT(9) == 23);

    /* Empty the bundle */
    hdag_bundle_empty(&bundle);

    /*
     * Check edge deduping works
     */

    /*
     * Create a bundle with various combinations of duplicate edges:
     *
     * 0 -> 1 -> 2 -> 4 -> 6 -> 8
     * |    |    |    |    |--> 8
     * |    |    |    |    `--> 8
     * |    |    |    |--> 6
     * |    |    |    `--> 7
     * |    |    |--> 5
     * |    |    `--> 4
     * |    |--> 3
     * |    `--> 3
     * `--> 1
     */
    /* Two same edges */
    node = hdag_darr_cappend_one(&bundle.nodes);
    TEST(node);
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
    TEST(node);
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
    TEST(node);
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
    TEST(node);
    hdag_node_hash_fill(node, hash_len, 3);

    /* Edge a, edge a, edge b */
    node = hdag_darr_cappend_one(&bundle.nodes);
    TEST(node);
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
    TEST(node);
    hdag_node_hash_fill(node, hash_len, 5);

    /* Three same edges */
    node = hdag_darr_cappend_one(&bundle.nodes);
    TEST(node);
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
    TEST(node);
    hdag_node_hash_fill(node, hash_len, 7);
    node = hdag_darr_cappend_one(&bundle.nodes);
    TEST(node);
    hdag_node_hash_fill(node, hash_len, 8);

    /* Check we have the expected number of nodes and target hashes */
    TEST(bundle.nodes.slots_occupied == 9);
    TEST(bundle.target_hashes.slots_occupied == 14);

    /* Sort and dedup */
    TEST(hdag_bundle_sort_and_dedup(&bundle, false) == HDAG_RES_OK);

    /* Check the expected results:
     *
     * 0 -> 1 -> 2 -> 4 -> 6 -> 8
     *      |    |    `--> 7
     *      |    `--> 5
     *      `--> 3
     */
    TEST(bundle.nodes.slots_occupied == 9);
    TEST(bundle.unknown_indexes.slots_occupied == 0);
    TEST(bundle.target_hashes.slots_occupied == 14);
    TEST(hdag_bundle_is_sorted_and_deduped(&bundle));
    HDAG_DARR_ITER_FORWARD(&bundle.nodes, idx, node, (void)0, (void)0) {
        TEST(hdag_node_hash_is_filled(node, hash_len, idx));
    }
    TEST(hdag_targets_are_indirect(HDAG_BUNDLE_TARGETS(&bundle, 0)));
    TEST(hdag_bundle_targets_count(&bundle, 0) == 1);
    TEST(hdag_hash_is_filled(hdag_bundle_targets_node_hash(&bundle, 0, 0),
                             hash_len, 1));

    TEST(hdag_targets_are_indirect(HDAG_BUNDLE_TARGETS(&bundle, 1)));
    TEST(hdag_bundle_targets_count(&bundle, 1) == 2);
    TEST(hdag_hash_is_filled(hdag_bundle_targets_node_hash(&bundle, 1, 0),
                             hash_len, 2));
    TEST(hdag_hash_is_filled(hdag_bundle_targets_node_hash(&bundle, 1, 1),
                             hash_len, 3));

    TEST(hdag_targets_are_indirect(HDAG_BUNDLE_TARGETS(&bundle, 2)));
    TEST(hdag_bundle_targets_count(&bundle, 2) == 2);
    TEST(hdag_hash_is_filled(hdag_bundle_targets_node_hash(&bundle, 2, 0),
                             hash_len, 4));
    TEST(hdag_hash_is_filled(hdag_bundle_targets_node_hash(&bundle, 2, 1),
                             hash_len, 5));

    TEST(hdag_targets_are_absent(HDAG_BUNDLE_TARGETS(&bundle, 3)));

    TEST(hdag_targets_are_indirect(HDAG_BUNDLE_TARGETS(&bundle, 4)));
    TEST(hdag_bundle_targets_count(&bundle, 4) == 2);
    TEST(hdag_hash_is_filled(hdag_bundle_targets_node_hash(&bundle, 4, 0),
                             hash_len, 6));
    TEST(hdag_hash_is_filled(hdag_bundle_targets_node_hash(&bundle, 4, 1),
                             hash_len, 7));

    TEST(hdag_targets_are_absent(HDAG_BUNDLE_TARGETS(&bundle, 5)));

    TEST(hdag_targets_are_indirect(HDAG_BUNDLE_TARGETS(&bundle, 6)));
    TEST(hdag_bundle_targets_count(&bundle, 6) == 1);
    TEST(hdag_hash_is_filled(hdag_bundle_targets_node_hash(&bundle, 6, 0),
                             hash_len, 8));

    TEST(hdag_targets_are_absent(HDAG_BUNDLE_TARGETS(&bundle, 7)));

    TEST(hdag_targets_are_absent(HDAG_BUNDLE_TARGETS(&bundle, 8)));

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

    /* Create a directed path: 15->14->13->...->2->1->0 */
    TEST(hdag_darr_cappend(&bundle.nodes, 16));
    TEST(hdag_darr_cappend(&bundle.target_hashes, 15));
    HDAG_DARR_ITER_FORWARD(&bundle.nodes, idx, node, (void)0, (void)0) {
        hdag_node_hash_fill(node, hash_len, 15 - idx);
        if (idx <= 14) {
            hdag_hash_fill(hdag_darr_element(&bundle.target_hashes, idx),
                           hash_len, 14 - idx);
            node->targets = hdag_targets_indirect(idx, idx);
        }
        assert(hdag_node_is_valid(node));
    }
    TEST(!hdag_bundle_is_sorted_and_deduped(&bundle));
    TEST(hdag_bundle_sort_and_dedup(&bundle, false) == HDAG_RES_OK);
    TEST(hdag_bundle_is_sorted_and_deduped(&bundle));
    hdag_bundle_fanout_fill(&bundle);
    TEST(!hdag_bundle_fanout_is_empty(&bundle));
    TEST(hdag_bundle_compact(&bundle) == HDAG_RES_OK);
    TEST(!hdag_bundle_has_hash_targets(&bundle));
    TEST(hdag_bundle_has_index_targets(&bundle));
    /* Expecting: 0<-1<-2<-...13<-14<-15 */
    HDAG_DARR_ITER_FORWARD(&bundle.nodes, idx, node, (void)0, (void)0) {
        TEST(hdag_node_is_valid(node));
        TEST(hdag_node_hash_is_filled(node, hash_len, idx));
        if (idx == 0) {
            TEST(hdag_targets_are_known(&node->targets));
            TEST(hdag_targets_are_absent(&node->targets));
        } else {
            TEST(hdag_targets_are_known(&node->targets));
            TEST(hdag_targets_are_direct(&node->targets));
            TEST(hdag_target_is_dir_idx(node->targets.first));
            TEST(hdag_target_to_dir_idx(node->targets.first) ==
                 (size_t)idx - 1);
            TEST(node->targets.last == HDAG_TARGET_ABSENT);
        }
    }

    /* Empty the bundle */
    hdag_bundle_empty(&bundle);

    /*
     * Check unknown nodes are handled correctly, and extra edges are created
     */
    TEST(hdag_darr_cappend(&bundle.nodes, 9));
    TEST(hdag_darr_cappend(&bundle.target_hashes, 8));
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
    TEST(hdag_bundle_sort_and_dedup(&bundle, false) == HDAG_RES_OK);
    TEST(bundle.nodes.slots_occupied == 9);
    TEST(bundle.target_hashes.slots_occupied == 8);
    TEST(bundle.extra_edges.slots_occupied == 0);
    hdag_bundle_fanout_fill(&bundle);
    TEST(!hdag_bundle_fanout_is_empty(&bundle));
    TEST(hdag_bundle_compact(&bundle) == HDAG_RES_OK);
    TEST(bundle.nodes.slots_occupied == 9);
    TEST(bundle.target_hashes.slots_occupied == 0);
    TEST(bundle.extra_edges.slots_occupied == 8);
    TEST(bundle.unknown_indexes.slots_occupied == 4);
    TEST(*HDAG_DARR_ELEMENT(&bundle.unknown_indexes, uint32_t, 0) == 1);
    TEST(*HDAG_DARR_ELEMENT(&bundle.unknown_indexes, uint32_t, 1) == 3);
    TEST(*HDAG_DARR_ELEMENT(&bundle.unknown_indexes, uint32_t, 2) == 5);
    TEST(*HDAG_DARR_ELEMENT(&bundle.unknown_indexes, uint32_t, 3) == 7);
    hdag_bundle_empty(&bundle);

    /* Cleanup the bundle */
    hdag_bundle_cleanup(&bundle);

    return failed;
}

static size_t
test_inverting(uint16_t hash_len)
{
    size_t failed = 0;
    struct hdag_bundle original = HDAG_BUNDLE_EMPTY(hash_len);
    struct hdag_bundle inverted;
    struct hdag_edge *edge;

#define ORIGINAL_ADD_NODES(_num) \
    do {                                                        \
        ssize_t _idx;                                           \
        struct hdag_node *_node;                                \
        TEST(hdag_darr_cappend(&original.nodes, _num));         \
        HDAG_DARR_ITER_FORWARD(&original.nodes, _idx, _node,    \
                               (void)0, (void)0) {              \
            hdag_node_hash_fill(_node, hash_len, _idx + 1);     \
        }                                                       \
    } while (0)

#define INVERTED_CHECK_NODES(_num) \
    do {                                                                \
        ssize_t _idx;                                                   \
        struct hdag_node *_node;                                        \
        TEST(hdag_darr_occupied_slots(&inverted.nodes) == (_num));      \
        HDAG_DARR_ITER_FORWARD(&original.nodes, _idx, _node,            \
                               (void)0, (void)0) {                      \
            TEST(hdag_node_hash_is_filled(                              \
                HDAG_BUNDLE_NODE(&inverted, _idx), hash_len, (_idx) + 1 \
            ));                                                         \
        }                                                               \
    } while (0)

#define INVERTED_CHECK_HASHLESS_NODES(_num) \
    TEST(hdag_darr_occupied_slots(&inverted.nodes) == (_num))

    /* Invert empty bundle */
    TEST(!hdag_bundle_invert(&inverted, &original, false));
    TEST(memcmp(&inverted, &original, sizeof(struct hdag_bundle)) == 0);
    hdag_bundle_cleanup(&inverted);

    /* Invert empty bundle without hashes */
    TEST(!hdag_bundle_is_hashless(&original));
    TEST(!hdag_bundle_invert(&inverted, &original, true));
    TEST(hdag_bundle_is_hashless(&inverted));
    TEST(hdag_bundle_is_empty(&inverted));
    hdag_bundle_cleanup(&inverted);

    /* Invert single-node bundle */
    ORIGINAL_ADD_NODES(1);
    TEST(!hdag_bundle_invert(&inverted, &original, false));
    INVERTED_CHECK_NODES(1);
    TEST(hdag_bundle_targets_count(&inverted, 0) == 0);
    TEST(hdag_darr_occupied_slots(&inverted.extra_edges) == 0);
    TEST(hdag_darr_occupied_slots(&inverted.target_hashes) == 0);
    hdag_bundle_empty(&original);
    hdag_bundle_cleanup(&inverted);

    /* Invert single-node bundle without hashes */
    ORIGINAL_ADD_NODES(1);
    TEST(!hdag_bundle_invert(&inverted, &original, true));
    INVERTED_CHECK_HASHLESS_NODES(1);
    TEST(hdag_bundle_targets_count(&inverted, 0) == 0);
    TEST(hdag_darr_occupied_slots(&inverted.extra_edges) == 0);
    TEST(hdag_darr_occupied_slots(&inverted.target_hashes) == 0);
    hdag_bundle_empty(&original);
    hdag_bundle_cleanup(&inverted);

    /* Invert single unknown-node bundle */
    ORIGINAL_ADD_NODES(1);
    HDAG_BUNDLE_NODE(&original, 0)->targets = HDAG_TARGETS_UNKNOWN;
    TEST(!hdag_bundle_invert(&inverted, &original, false));
    INVERTED_CHECK_NODES(1);
    TEST(hdag_bundle_targets_count(&inverted, 0) == 0);
    TEST(hdag_darr_occupied_slots(&inverted.extra_edges) == 0);
    TEST(hdag_darr_occupied_slots(&inverted.target_hashes) == 0);
    hdag_bundle_empty(&original);
    hdag_bundle_cleanup(&inverted);

    /* Invert single unknown-node bundle without hashes */
    ORIGINAL_ADD_NODES(1);
    HDAG_BUNDLE_NODE(&original, 0)->targets = HDAG_TARGETS_UNKNOWN;
    TEST(!hdag_bundle_invert(&inverted, &original, true));
    INVERTED_CHECK_HASHLESS_NODES(1);
    TEST(hdag_bundle_targets_count(&inverted, 0) == 0);
    TEST(hdag_darr_occupied_slots(&inverted.extra_edges) == 0);
    TEST(hdag_darr_occupied_slots(&inverted.target_hashes) == 0);
    hdag_bundle_empty(&original);
    hdag_bundle_cleanup(&inverted);

    /* Invert two disconnected-node bundle */
    ORIGINAL_ADD_NODES(2);
    TEST(!hdag_bundle_invert(&inverted, &original, false));
    INVERTED_CHECK_NODES(2);
    TEST(hdag_bundle_targets_count(&inverted, 0) == 0);
    TEST(hdag_bundle_targets_count(&inverted, 1) == 0);
    TEST(hdag_darr_occupied_slots(&inverted.extra_edges) == 0);
    TEST(hdag_darr_occupied_slots(&inverted.target_hashes) == 0);
    hdag_bundle_empty(&original);
    hdag_bundle_cleanup(&inverted);

    /* Invert two unknown-node bundle */
    ORIGINAL_ADD_NODES(2);
    HDAG_BUNDLE_NODE(&original, 0)->targets = HDAG_TARGETS_UNKNOWN;
    HDAG_BUNDLE_NODE(&original, 1)->targets = HDAG_TARGETS_UNKNOWN;
    TEST(!hdag_bundle_invert(&inverted, &original, false));
    INVERTED_CHECK_NODES(2);
    TEST(hdag_bundle_targets_count(&inverted, 0) == 0);
    TEST(hdag_bundle_targets_count(&inverted, 1) == 0);
    TEST(hdag_darr_occupied_slots(&inverted.extra_edges) == 0);
    TEST(hdag_darr_occupied_slots(&inverted.target_hashes) == 0);
    hdag_bundle_empty(&original);
    hdag_bundle_cleanup(&inverted);

    /* Invert two connected-node bundle */
    ORIGINAL_ADD_NODES(2);
    HDAG_BUNDLE_NODE(&original, 0)->targets = hdag_targets_direct_one(1);
    TEST(hdag_bundle_targets_count(&original, 0) == 1);
    TEST(hdag_bundle_targets_count(&original, 1) == 0);
    TEST(hdag_bundle_targets_node_idx(&original, 0, 0) == 1);
    TEST(!hdag_bundle_invert(&inverted, &original, false));
    INVERTED_CHECK_NODES(2);
    TEST(hdag_bundle_targets_count(&inverted, 0) == 0);
    TEST(hdag_bundle_targets_count(&inverted, 1) == 1);
    TEST(hdag_bundle_targets_node_idx(&inverted, 1, 0) == 0);
    TEST(hdag_darr_occupied_slots(&inverted.extra_edges) == 0);
    TEST(hdag_darr_occupied_slots(&inverted.target_hashes) == 0);
    hdag_bundle_empty(&original);
    hdag_bundle_cleanup(&inverted);

    /* Invert N0 -> (N1, N2) */
    ORIGINAL_ADD_NODES(3);
    HDAG_BUNDLE_NODE(&original, 0)->targets = hdag_targets_direct_two(1, 2);
    TEST(hdag_bundle_targets_count(&original, 0) == 2);
    TEST(hdag_bundle_targets_count(&original, 1) == 0);
    TEST(hdag_bundle_targets_count(&original, 2) == 0);
    TEST(hdag_bundle_targets_node_idx(&original, 0, 0) == 1);
    TEST(hdag_bundle_targets_node_idx(&original, 0, 1) == 2);
    TEST(!hdag_bundle_invert(&inverted, &original, false));
    INVERTED_CHECK_NODES(3);
    TEST(hdag_bundle_targets_count(&inverted, 0) == 0);
    TEST(hdag_bundle_targets_count(&inverted, 1) == 1);
    TEST(hdag_bundle_targets_count(&inverted, 2) == 1);
    TEST(hdag_bundle_targets_node_idx(&inverted, 1, 0) == 0);
    TEST(hdag_bundle_targets_node_idx(&inverted, 2, 0) == 0);
    TEST(hdag_darr_occupied_slots(&inverted.extra_edges) == 0);
    TEST(hdag_darr_occupied_slots(&inverted.target_hashes) == 0);
    hdag_bundle_empty(&original);
    hdag_bundle_cleanup(&inverted);

    /* Invert N0 -> (N1, N2, N3) (indirect->direct) */
    ORIGINAL_ADD_NODES(4);
    edge = hdag_darr_uappend(&original.extra_edges, 3);
    TEST(edge);
    edge++->node_idx = 1;
    edge++->node_idx = 2;
    edge++->node_idx = 3;
    HDAG_BUNDLE_NODE(&original, 0)->targets = hdag_targets_indirect(0, 2);
    assert(hdag_bundle_targets_count(&original, 0) == 3);
    assert(hdag_bundle_targets_count(&original, 1) == 0);
    assert(hdag_bundle_targets_count(&original, 2) == 0);
    assert(hdag_bundle_targets_count(&original, 3) == 0);
    assert(hdag_bundle_targets_node_idx(&original, 0, 0) == 1);
    assert(hdag_bundle_targets_node_idx(&original, 0, 1) == 2);
    assert(hdag_bundle_targets_node_idx(&original, 0, 2) == 3);
    assert(hdag_bundle_is_valid(&original));
    TEST(!hdag_bundle_invert(&inverted, &original, false));
    INVERTED_CHECK_NODES(4);
    TEST(hdag_bundle_targets_count(&inverted, 0) == 0);
    TEST(hdag_bundle_targets_count(&inverted, 1) == 1);
    TEST(hdag_bundle_targets_count(&inverted, 2) == 1);
    TEST(hdag_bundle_targets_count(&inverted, 3) == 1);
    TEST(hdag_bundle_targets_node_idx(&inverted, 1, 0) == 0);
    TEST(hdag_bundle_targets_node_idx(&inverted, 2, 0) == 0);
    TEST(hdag_bundle_targets_node_idx(&inverted, 3, 0) == 0);
    TEST(hdag_darr_occupied_slots(&inverted.extra_edges) == 0);
    TEST(hdag_darr_occupied_slots(&inverted.target_hashes) == 0);
    hdag_bundle_empty(&original);
    hdag_bundle_cleanup(&inverted);

    /* Invert (N0, N1, N2) -> N3 (direct->indirect) */
    ORIGINAL_ADD_NODES(4);
    HDAG_BUNDLE_NODE(&original, 0)->targets = hdag_targets_direct_one(3);
    HDAG_BUNDLE_NODE(&original, 1)->targets = hdag_targets_direct_one(3);
    HDAG_BUNDLE_NODE(&original, 2)->targets = hdag_targets_direct_one(3);
    assert(hdag_bundle_targets_count(&original, 0) == 1);
    assert(hdag_bundle_targets_count(&original, 1) == 1);
    assert(hdag_bundle_targets_count(&original, 2) == 1);
    assert(hdag_bundle_targets_count(&original, 3) == 0);
    assert(hdag_bundle_targets_node_idx(&original, 0, 0) == 3);
    assert(hdag_bundle_targets_node_idx(&original, 1, 0) == 3);
    assert(hdag_bundle_targets_node_idx(&original, 2, 0) == 3);
    assert(hdag_bundle_is_valid(&original));
    TEST(!hdag_bundle_invert(&inverted, &original, false));
    INVERTED_CHECK_NODES(4);
    TEST(hdag_bundle_targets_count(&inverted, 0) == 0);
    TEST(hdag_bundle_targets_count(&inverted, 1) == 0);
    TEST(hdag_bundle_targets_count(&inverted, 2) == 0);
    TEST(hdag_bundle_targets_count(&inverted, 3) == 3);
    TEST(hdag_bundle_targets_node_idx(&inverted, 3, 0) == 0);
    TEST(hdag_bundle_targets_node_idx(&inverted, 3, 1) == 1);
    TEST(hdag_bundle_targets_node_idx(&inverted, 3, 2) == 2);
    TEST(hdag_darr_occupied_slots(&inverted.extra_edges) == 3);
    TEST(hdag_darr_occupied_slots(&inverted.target_hashes) == 0);
    hdag_bundle_empty(&original);
    hdag_bundle_cleanup(&inverted);

    /*
     * Invert (N0, N1, N2) -> N3 (N4, N5, N6) -> N7
     * (direct->multiple-indirect)
     */
    ORIGINAL_ADD_NODES(8);
    HDAG_BUNDLE_NODE(&original, 0)->targets = hdag_targets_direct_one(3);
    HDAG_BUNDLE_NODE(&original, 1)->targets = hdag_targets_direct_one(3);
    HDAG_BUNDLE_NODE(&original, 2)->targets = hdag_targets_direct_one(3);
    HDAG_BUNDLE_NODE(&original, 4)->targets = hdag_targets_direct_one(7);
    HDAG_BUNDLE_NODE(&original, 5)->targets = hdag_targets_direct_one(7);
    HDAG_BUNDLE_NODE(&original, 6)->targets = hdag_targets_direct_one(7);
    assert(hdag_bundle_is_valid(&original));
    assert(hdag_darr_occupied_slots(&original.extra_edges) == 0);
    TEST(!hdag_bundle_invert(&inverted, &original, false));
    INVERTED_CHECK_NODES(8);
    TEST(hdag_bundle_targets_count(&inverted, 0) == 0);
    TEST(hdag_bundle_targets_count(&inverted, 1) == 0);
    TEST(hdag_bundle_targets_count(&inverted, 2) == 0);
    TEST(hdag_bundle_targets_count(&inverted, 3) == 3);
    TEST(hdag_bundle_targets_node_idx(&inverted, 3, 0) == 0);
    TEST(hdag_bundle_targets_node_idx(&inverted, 3, 1) == 1);
    TEST(hdag_bundle_targets_node_idx(&inverted, 3, 2) == 2);
    TEST(hdag_bundle_targets_count(&inverted, 4) == 0);
    TEST(hdag_bundle_targets_count(&inverted, 5) == 0);
    TEST(hdag_bundle_targets_count(&inverted, 6) == 0);
    TEST(hdag_bundle_targets_count(&inverted, 7) == 3);
    TEST(hdag_bundle_targets_node_idx(&inverted, 7, 0) == 4);
    TEST(hdag_bundle_targets_node_idx(&inverted, 7, 1) == 5);
    TEST(hdag_bundle_targets_node_idx(&inverted, 7, 2) == 6);
    TEST(hdag_darr_occupied_slots(&inverted.extra_edges) == 6);
    TEST(hdag_darr_occupied_slots(&inverted.target_hashes) == 0);
    hdag_bundle_empty(&original);
    hdag_bundle_cleanup(&inverted);

    /*
     * Invert (N0, N1, N2) -> N3 (N4, N5, N6) -> N7 (hashless)
     * (direct->multiple-indirect)
     */
    ORIGINAL_ADD_NODES(8);
    HDAG_BUNDLE_NODE(&original, 0)->targets = hdag_targets_direct_one(3);
    HDAG_BUNDLE_NODE(&original, 1)->targets = hdag_targets_direct_one(3);
    HDAG_BUNDLE_NODE(&original, 2)->targets = hdag_targets_direct_one(3);
    HDAG_BUNDLE_NODE(&original, 4)->targets = hdag_targets_direct_one(7);
    HDAG_BUNDLE_NODE(&original, 5)->targets = hdag_targets_direct_one(7);
    HDAG_BUNDLE_NODE(&original, 6)->targets = hdag_targets_direct_one(7);
    assert(hdag_bundle_is_valid(&original));
    assert(hdag_darr_occupied_slots(&original.extra_edges) == 0);
    TEST(!hdag_bundle_invert(&inverted, &original, true));
    INVERTED_CHECK_HASHLESS_NODES(8);
    TEST(hdag_bundle_targets_count(&inverted, 0) == 0);
    TEST(hdag_bundle_targets_count(&inverted, 1) == 0);
    TEST(hdag_bundle_targets_count(&inverted, 2) == 0);
    TEST(hdag_bundle_targets_count(&inverted, 3) == 3);
    TEST(hdag_bundle_targets_node_idx(&inverted, 3, 0) == 0);
    TEST(hdag_bundle_targets_node_idx(&inverted, 3, 1) == 1);
    TEST(hdag_bundle_targets_node_idx(&inverted, 3, 2) == 2);
    TEST(hdag_bundle_targets_count(&inverted, 4) == 0);
    TEST(hdag_bundle_targets_count(&inverted, 5) == 0);
    TEST(hdag_bundle_targets_count(&inverted, 6) == 0);
    TEST(hdag_bundle_targets_count(&inverted, 7) == 3);
    TEST(hdag_bundle_targets_node_idx(&inverted, 7, 0) == 4);
    TEST(hdag_bundle_targets_node_idx(&inverted, 7, 1) == 5);
    TEST(hdag_bundle_targets_node_idx(&inverted, 7, 2) == 6);
    TEST(hdag_darr_occupied_slots(&inverted.extra_edges) == 6);
    TEST(hdag_darr_occupied_slots(&inverted.target_hashes) == 0);
    hdag_bundle_empty(&original);
    hdag_bundle_cleanup(&inverted);

    /* Invert N0 -> N1 <- N2 */
    ORIGINAL_ADD_NODES(3);
    HDAG_BUNDLE_NODE(&original, 0)->targets = hdag_targets_direct_one(1);
    HDAG_BUNDLE_NODE(&original, 2)->targets = hdag_targets_direct_one(1);
    assert(hdag_bundle_is_valid(&original));
    assert(hdag_darr_occupied_slots(&original.extra_edges) == 0);
    TEST(!hdag_bundle_invert(&inverted, &original, false));
    INVERTED_CHECK_NODES(3);
    TEST(hdag_bundle_targets_count(&inverted, 0) == 0);
    TEST(hdag_bundle_targets_count(&inverted, 1) == 2);
    TEST(hdag_bundle_targets_count(&inverted, 2) == 0);
    TEST(hdag_bundle_targets_node_idx(&inverted, 1, 0) == 0);
    TEST(hdag_bundle_targets_node_idx(&inverted, 1, 1) == 2);
    TEST(hdag_darr_occupied_slots(&inverted.extra_edges) == 0);
    TEST(hdag_darr_occupied_slots(&inverted.target_hashes) == 0);
    hdag_bundle_empty(&original);
    hdag_bundle_cleanup(&inverted);

#undef ORIGINAL_ADD_NODES
#undef INVERTED_CHECK_NODES

    hdag_bundle_cleanup(&original);

    return failed;
}

static size_t
test_enumerating(uint16_t hash_len)
{
    size_t failed = 0;
    struct hdag_bundle bundle = HDAG_BUNDLE_EMPTY(hash_len);
    struct hdag_edge *edge;

#define ADD_NODES(_num) \
    do {                                                    \
        ssize_t _idx;                                       \
        struct hdag_node *_node;                            \
        TEST(hdag_darr_cappend(&bundle.nodes, _num));       \
        HDAG_DARR_ITER_FORWARD(&bundle.nodes, _idx, _node,  \
                               (void)0, (void)0) {          \
            hdag_node_hash_fill(_node, hash_len, _idx + 1); \
        }                                                   \
    } while (0)

    /* Enumerate empty bundle */
    TEST(hdag_bundle_enumerate(&bundle) == HDAG_RES_OK);
    TEST(hdag_darr_occupied_slots(&bundle.nodes) == 0);
    TEST(hdag_darr_occupied_slots(&bundle.target_hashes) == 0);
    TEST(hdag_darr_occupied_slots(&bundle.extra_edges) == 0);
    hdag_bundle_cleanup(&bundle);

    /* Enumerate single-node bundle */
    ADD_NODES(1);
    TEST(hdag_bundle_enumerate(&bundle) == HDAG_RES_OK);
    TEST(hdag_darr_occupied_slots(&bundle.nodes) == 1);
    TEST(HDAG_BUNDLE_NODE(&bundle, 0)->generation == 1);
    TEST(HDAG_BUNDLE_NODE(&bundle, 0)->component == 1);
    TEST(hdag_darr_occupied_slots(&bundle.target_hashes) == 0);
    TEST(hdag_darr_occupied_slots(&bundle.extra_edges) == 0);
    hdag_bundle_cleanup(&bundle);

    /* Enumerate two disconnected nodes */
    ADD_NODES(2);
    TEST(hdag_bundle_enumerate(&bundle) == HDAG_RES_OK);
    TEST(hdag_darr_occupied_slots(&bundle.nodes) == 2);
    TEST(HDAG_BUNDLE_NODE(&bundle, 0)->generation == 1);
    TEST(HDAG_BUNDLE_NODE(&bundle, 0)->component == 1);
    TEST(HDAG_BUNDLE_NODE(&bundle, 1)->generation == 1);
    TEST(HDAG_BUNDLE_NODE(&bundle, 1)->component == 2);
    TEST(hdag_darr_occupied_slots(&bundle.target_hashes) == 0);
    TEST(hdag_darr_occupied_slots(&bundle.extra_edges) == 0);
    hdag_bundle_cleanup(&bundle);

    /* Enumerate N0 -> N1 */
    ADD_NODES(2);
    HDAG_BUNDLE_NODE(&bundle, 0)->targets = hdag_targets_direct_one(1);
    TEST(hdag_bundle_enumerate(&bundle) == HDAG_RES_OK);
    TEST(hdag_darr_occupied_slots(&bundle.nodes) == 2);
    TEST(HDAG_BUNDLE_NODE(&bundle, 0)->generation == 2);
    TEST(HDAG_BUNDLE_NODE(&bundle, 0)->component == 1);
    TEST(HDAG_BUNDLE_NODE(&bundle, 1)->generation == 1);
    TEST(HDAG_BUNDLE_NODE(&bundle, 1)->component == 1);
    TEST(hdag_darr_occupied_slots(&bundle.target_hashes) == 0);
    TEST(hdag_darr_occupied_slots(&bundle.extra_edges) == 0);
    hdag_bundle_cleanup(&bundle);

    /* Enumerate N0 <- N1 */
    ADD_NODES(2);
    HDAG_BUNDLE_NODE(&bundle, 1)->targets = hdag_targets_direct_one(0);
    TEST(hdag_bundle_enumerate(&bundle) == HDAG_RES_OK);
    TEST(hdag_darr_occupied_slots(&bundle.nodes) == 2);
    TEST(HDAG_BUNDLE_NODE(&bundle, 0)->generation == 1);
    TEST(HDAG_BUNDLE_NODE(&bundle, 0)->component == 1);
    TEST(HDAG_BUNDLE_NODE(&bundle, 1)->generation == 2);
    TEST(HDAG_BUNDLE_NODE(&bundle, 1)->component == 1);
    TEST(hdag_darr_occupied_slots(&bundle.target_hashes) == 0);
    TEST(hdag_darr_occupied_slots(&bundle.extra_edges) == 0);
    hdag_bundle_cleanup(&bundle);

    /* Enumerate N0 -> N1 <- N2 */
    ADD_NODES(3);
    HDAG_BUNDLE_NODE(&bundle, 0)->targets = hdag_targets_direct_one(1);
    HDAG_BUNDLE_NODE(&bundle, 2)->targets = hdag_targets_direct_one(1);
    TEST(hdag_bundle_enumerate(&bundle) == HDAG_RES_OK);
    TEST(hdag_darr_occupied_slots(&bundle.nodes) == 3);
    TEST(HDAG_BUNDLE_NODE(&bundle, 0)->generation == 2);
    TEST(HDAG_BUNDLE_NODE(&bundle, 0)->component == 1);
    TEST(HDAG_BUNDLE_NODE(&bundle, 1)->generation == 1);
    TEST(HDAG_BUNDLE_NODE(&bundle, 1)->component == 1);
    TEST(HDAG_BUNDLE_NODE(&bundle, 2)->generation == 2);
    TEST(HDAG_BUNDLE_NODE(&bundle, 2)->component == 1);
    TEST(hdag_darr_occupied_slots(&bundle.target_hashes) == 0);
    TEST(hdag_darr_occupied_slots(&bundle.extra_edges) == 0);
    hdag_bundle_cleanup(&bundle);

    /* Enumerate N0 <- N1 -> N2 */
    ADD_NODES(3);
    HDAG_BUNDLE_NODE(&bundle, 1)->targets = hdag_targets_direct_two(0, 2);
    TEST(hdag_bundle_enumerate(&bundle) == HDAG_RES_OK);
    TEST(hdag_darr_occupied_slots(&bundle.nodes) == 3);
    TEST(HDAG_BUNDLE_NODE(&bundle, 0)->generation == 1);
    TEST(HDAG_BUNDLE_NODE(&bundle, 0)->component == 1);
    TEST(HDAG_BUNDLE_NODE(&bundle, 1)->generation == 2);
    TEST(HDAG_BUNDLE_NODE(&bundle, 1)->component == 1);
    TEST(HDAG_BUNDLE_NODE(&bundle, 2)->generation == 1);
    TEST(HDAG_BUNDLE_NODE(&bundle, 2)->component == 1);
    TEST(hdag_darr_occupied_slots(&bundle.target_hashes) == 0);
    TEST(hdag_darr_occupied_slots(&bundle.extra_edges) == 0);
    hdag_bundle_cleanup(&bundle);

    /* Enumerate N0 <- N1 <- N2 N3 -> (N1)  */
    ADD_NODES(4);
    HDAG_BUNDLE_NODE(&bundle, 1)->targets = hdag_targets_direct_one(0);
    HDAG_BUNDLE_NODE(&bundle, 2)->targets = hdag_targets_direct_one(1);
    HDAG_BUNDLE_NODE(&bundle, 3)->targets = hdag_targets_direct_one(1);
    TEST(hdag_bundle_enumerate(&bundle) == HDAG_RES_OK);
    TEST(hdag_darr_occupied_slots(&bundle.nodes) == 4);
    TEST(HDAG_BUNDLE_NODE(&bundle, 0)->generation == 1);
    TEST(HDAG_BUNDLE_NODE(&bundle, 0)->component == 1);
    TEST(HDAG_BUNDLE_NODE(&bundle, 1)->generation == 2);
    TEST(HDAG_BUNDLE_NODE(&bundle, 1)->component == 1);
    TEST(HDAG_BUNDLE_NODE(&bundle, 2)->generation == 3);
    TEST(HDAG_BUNDLE_NODE(&bundle, 2)->component == 1);
    TEST(HDAG_BUNDLE_NODE(&bundle, 3)->generation == 3);
    TEST(HDAG_BUNDLE_NODE(&bundle, 3)->component == 1);
    TEST(hdag_darr_occupied_slots(&bundle.target_hashes) == 0);
    TEST(hdag_darr_occupied_slots(&bundle.extra_edges) == 0);
    hdag_bundle_cleanup(&bundle);

    /* Enumerate N0 -> (N2) N1 <- N2 <- N3  */
    ADD_NODES(4);
    HDAG_BUNDLE_NODE(&bundle, 0)->targets = hdag_targets_direct_one(2);
    HDAG_BUNDLE_NODE(&bundle, 2)->targets = hdag_targets_direct_one(1);
    HDAG_BUNDLE_NODE(&bundle, 3)->targets = hdag_targets_direct_one(2);
    TEST(hdag_bundle_enumerate(&bundle) == HDAG_RES_OK);
    TEST(hdag_darr_occupied_slots(&bundle.nodes) == 4);
    TEST(HDAG_BUNDLE_NODE(&bundle, 0)->generation == 3);
    TEST(HDAG_BUNDLE_NODE(&bundle, 0)->component == 1);
    TEST(HDAG_BUNDLE_NODE(&bundle, 1)->generation == 1);
    TEST(HDAG_BUNDLE_NODE(&bundle, 1)->component == 1);
    TEST(HDAG_BUNDLE_NODE(&bundle, 2)->generation == 2);
    TEST(HDAG_BUNDLE_NODE(&bundle, 2)->component == 1);
    TEST(HDAG_BUNDLE_NODE(&bundle, 3)->generation == 3);
    TEST(HDAG_BUNDLE_NODE(&bundle, 3)->component == 1);
    TEST(hdag_darr_occupied_slots(&bundle.target_hashes) == 0);
    TEST(hdag_darr_occupied_slots(&bundle.extra_edges) == 0);
    hdag_bundle_cleanup(&bundle);

    /* Enumerate cyclic bundle: N0 <-> N1 */
    ADD_NODES(2);
    HDAG_BUNDLE_NODE(&bundle, 0)->targets = hdag_targets_direct_one(1);
    HDAG_BUNDLE_NODE(&bundle, 1)->targets = hdag_targets_direct_one(0);
    TEST(hdag_bundle_enumerate(&bundle) == HDAG_RES_GRAPH_CYCLE);
    hdag_bundle_cleanup(&bundle);

    /* Enumerate cyclic bundle: N0 -> N1 -> N2 -> (N0) */
    ADD_NODES(3);
    HDAG_BUNDLE_NODE(&bundle, 0)->targets = hdag_targets_direct_one(1);
    HDAG_BUNDLE_NODE(&bundle, 1)->targets = hdag_targets_direct_one(2);
    HDAG_BUNDLE_NODE(&bundle, 2)->targets = hdag_targets_direct_one(0);
    TEST(hdag_bundle_enumerate(&bundle) == HDAG_RES_GRAPH_CYCLE);
    hdag_bundle_cleanup(&bundle);

    /*
     * Enumerate (N0, N1, N2) -> N3 (N4, N5, N6) -> N7
     */
    ADD_NODES(8);
    HDAG_BUNDLE_NODE(&bundle, 0)->targets = hdag_targets_direct_one(3);
    HDAG_BUNDLE_NODE(&bundle, 1)->targets = hdag_targets_direct_one(3);
    HDAG_BUNDLE_NODE(&bundle, 2)->targets = hdag_targets_direct_one(3);
    HDAG_BUNDLE_NODE(&bundle, 4)->targets = hdag_targets_direct_one(7);
    HDAG_BUNDLE_NODE(&bundle, 5)->targets = hdag_targets_direct_one(7);
    HDAG_BUNDLE_NODE(&bundle, 6)->targets = hdag_targets_direct_one(7);
    TEST(hdag_bundle_enumerate(&bundle) == HDAG_RES_OK);
    TEST(hdag_darr_occupied_slots(&bundle.nodes) == 8);
    TEST(HDAG_BUNDLE_NODE(&bundle, 0)->generation == 2);
    TEST(HDAG_BUNDLE_NODE(&bundle, 0)->component == 1);
    TEST(HDAG_BUNDLE_NODE(&bundle, 1)->generation == 2);
    TEST(HDAG_BUNDLE_NODE(&bundle, 1)->component == 1);
    TEST(HDAG_BUNDLE_NODE(&bundle, 2)->generation == 2);
    TEST(HDAG_BUNDLE_NODE(&bundle, 2)->component == 1);
    TEST(HDAG_BUNDLE_NODE(&bundle, 3)->generation == 1);
    TEST(HDAG_BUNDLE_NODE(&bundle, 3)->component == 1);
    TEST(HDAG_BUNDLE_NODE(&bundle, 4)->generation == 2);
    TEST(HDAG_BUNDLE_NODE(&bundle, 4)->component == 2);
    TEST(HDAG_BUNDLE_NODE(&bundle, 5)->generation == 2);
    TEST(HDAG_BUNDLE_NODE(&bundle, 5)->component == 2);
    TEST(HDAG_BUNDLE_NODE(&bundle, 6)->generation == 2);
    TEST(HDAG_BUNDLE_NODE(&bundle, 6)->component == 2);
    TEST(HDAG_BUNDLE_NODE(&bundle, 7)->generation == 1);
    TEST(HDAG_BUNDLE_NODE(&bundle, 7)->component == 2);
    TEST(hdag_darr_occupied_slots(&bundle.target_hashes) == 0);
    TEST(hdag_darr_occupied_slots(&bundle.extra_edges) == 0);
    hdag_bundle_cleanup(&bundle);

    /* Enumerate N0 -> (N1, N2, N3) */
    ADD_NODES(4);
    edge = hdag_darr_uappend(&bundle.extra_edges, 3);
    TEST(edge);
    edge++->node_idx = 1;
    edge++->node_idx = 2;
    edge++->node_idx = 3;
    HDAG_BUNDLE_NODE(&bundle, 0)->targets = hdag_targets_indirect(0, 2);
    TEST(hdag_bundle_enumerate(&bundle) == HDAG_RES_OK);
    TEST(hdag_darr_occupied_slots(&bundle.nodes) == 4);
    TEST(HDAG_BUNDLE_NODE(&bundle, 0)->generation == 2);
    TEST(HDAG_BUNDLE_NODE(&bundle, 0)->component == 1);
    TEST(HDAG_BUNDLE_NODE(&bundle, 1)->generation == 1);
    TEST(HDAG_BUNDLE_NODE(&bundle, 1)->component == 1);
    TEST(HDAG_BUNDLE_NODE(&bundle, 2)->generation == 1);
    TEST(HDAG_BUNDLE_NODE(&bundle, 2)->component == 1);
    TEST(HDAG_BUNDLE_NODE(&bundle, 3)->generation == 1);
    TEST(HDAG_BUNDLE_NODE(&bundle, 3)->component == 1);
    TEST(hdag_darr_occupied_slots(&bundle.target_hashes) == 0);
    TEST(hdag_darr_occupied_slots(&bundle.extra_edges) == 3);
    hdag_bundle_cleanup(&bundle);

    /* Enumerate N0 -> N1? */
    ADD_NODES(2);
    HDAG_BUNDLE_NODE(&bundle, 0)->targets = hdag_targets_direct_one(1);
    HDAG_BUNDLE_NODE(&bundle, 1)->targets = HDAG_TARGETS_UNKNOWN;
    TEST(hdag_bundle_enumerate(&bundle) == HDAG_RES_OK);
    TEST(hdag_darr_occupied_slots(&bundle.nodes) == 2);
    TEST(HDAG_BUNDLE_NODE(&bundle, 0)->generation == 2);
    TEST(HDAG_BUNDLE_NODE(&bundle, 0)->component == 1);
    TEST(HDAG_BUNDLE_NODE(&bundle, 1)->generation == 1);
    TEST(HDAG_BUNDLE_NODE(&bundle, 1)->component == 1);
    TEST(hdag_darr_occupied_slots(&bundle.target_hashes) == 0);
    TEST(hdag_darr_occupied_slots(&bundle.extra_edges) == 0);
    hdag_bundle_cleanup(&bundle);

#undef ADD_NODES

    return failed;
}

#define WITH_BUNDLES_AND_FILES(...) \
    for (                                                                   \
        const char **_contents_ptr = (const char *[]){__VA_ARGS__, NULL};   \
        *_contents_ptr != NULL &&                                           \
        (                                                                   \
            input_file = fmemopen((char *)*_contents_ptr,                   \
                            strlen(*_contents_ptr), "r"),                   \
            output_file = fmemopen(output_buf, sizeof(output_buf), "w"),    \
            setbuf(output_file, NULL),                                      \
            failed += (input_file == NULL || output_file == NULL),          \
            input_file != NULL && output_file != NULL                       \
        );                                                                  \
        _contents_ptr++,                                                    \
        hdag_bundle_cleanup(&bundle),                                       \
        fclose(input_file), input_file = NULL,                              \
        fclose(output_file), output_file = NULL                             \
    )

static size_t
test_txt(uint16_t hash_len)
{
    const char hex_digits[17] = "0123456789abcdef";
    size_t failed = 0;
    struct hdag_bundle bundle;
    FILE *input_file;
    FILE *output_file;
    char output_buf[4096];
    size_t i;
    const struct hdag_node *node;

    /* Test empty file */
    WITH_BUNDLES_AND_FILES("", " ", "\n", " \n", "\n ", " \t\r\n") {
        TEST(hdag_bundle_organized_from_txt(
                &bundle, NULL, input_file, hash_len
             ) == HDAG_RES_OK);
        TEST(bundle.hash_len == hash_len);
        TEST(bundle.nodes.slots_occupied == 0);
        TEST(bundle.target_hashes.slots_occupied == 0);
        TEST(bundle.extra_edges.slots_occupied == 0);
    }
    WITH_BUNDLES_AND_FILES("", " ", "\n", " \n", "\n ", " \t\r\n") {
        TEST(hdag_bundle_organized_from_txt(
                &bundle, NULL, input_file, hash_len
             ) == HDAG_RES_OK);
        TEST(bundle.hash_len == hash_len);
        TEST(bundle.nodes.slots_occupied == 0);
        TEST(bundle.target_hashes.slots_occupied == 0);
        TEST(bundle.extra_edges.slots_occupied == 0);
    }

    /* Test various invalid files */
    WITH_BUNDLES_AND_FILES("x", "1", "01x", "01 x", "01\nx", "01\rx", "012") {
        TEST(hdag_bundle_from_txt(&bundle, input_file, hash_len) ==
             HDAG_RES_INVALID_FORMAT);
    }

    /* Test single-node files with variations of whitespace padding */
    WITH_BUNDLES_AND_FILES(
        "01", "01\n", "\n01", " 01", "01 ", "\t\r\n01", "01\t\r\n"
    ) {
        TEST(hdag_bundle_from_txt(&bundle, input_file, hash_len) ==
             HDAG_RES_OK);
        TEST(bundle.hash_len == hash_len);
        TEST(bundle.nodes.slots_occupied == 1);
        node = HDAG_BUNDLE_NODE(&bundle, 0);
        for (i = 0;
             i < hash_len && node->hash[i] == (i == (size_t)(hash_len - 1));
             i++);
        TEST(i == hash_len);
        TEST(bundle.target_hashes.slots_occupied == 0);
        TEST(bundle.extra_edges.slots_occupied == 0);
    }

    WITH_BUNDLES_AND_FILES(
        "00", "00\n", "\n00", " 00", "00 ", "\t\r\n00", "00\t\r\n"
    ) {
        TEST(hdag_bundle_from_txt(&bundle, input_file, hash_len) ==
             HDAG_RES_OK);
        TEST(bundle.nodes.slots_occupied == 1);
        node = HDAG_BUNDLE_NODE(&bundle, 0);
        TEST(hdag_hash_is_filled(node->hash, hash_len, 0));
        TEST(bundle.target_hashes.slots_occupied == 0);
        TEST(bundle.extra_edges.slots_occupied == 0);
    }

    /* Test oversize, exact-size, and one byte undersize hash */
    {
        uint8_t hash[hash_len + 1];
        char text[(hash_len + 1) * 2 + 1];

        for (i = 0; i <= hash_len; i++) {
            text[i << 1] = hex_digits[(i >> 4) & 0xf];
            text[(i << 1) + 1] = hex_digits[i & 0xf];
            hash[i] = i & 0xff;
        }
        text[i << 1] = '\0';
        WITH_BUNDLES_AND_FILES(text) {
            TEST(hdag_bundle_from_txt(&bundle, input_file, hash_len) ==
                 HDAG_RES_INVALID_FORMAT);
        }

        i--;
        text[i << 1] = '\0';
        WITH_BUNDLES_AND_FILES(text) {
            TEST(hdag_bundle_from_txt(&bundle, input_file, hash_len) ==
                 HDAG_RES_OK);
            TEST(bundle.hash_len == hash_len);
            TEST(bundle.nodes.slots_occupied == 1);
            node = HDAG_BUNDLE_NODE(&bundle, 0);
            TEST(memcmp(node->hash, hash, hash_len) == 0);
            TEST(bundle.target_hashes.slots_occupied == 0);
            TEST(bundle.extra_edges.slots_occupied == 0);
        }

        i--;
        text[i << 1] = '\0';
        WITH_BUNDLES_AND_FILES(text) {
            TEST(hdag_bundle_from_txt(&bundle, input_file, hash_len) ==
                 HDAG_RES_OK);
            TEST(bundle.hash_len == hash_len);
            TEST(bundle.nodes.slots_occupied == 1);
            node = HDAG_BUNDLE_NODE(&bundle, 0);
            TEST(node->hash[0] == 0);
            TEST(memcmp(node->hash + 1, hash, hash_len - 1) == 0);
            TEST(bundle.target_hashes.slots_occupied == 0);
            TEST(bundle.extra_edges.slots_occupied == 0);
        }
    }

    /* Test a two node (source and target) file */
    WITH_BUNDLES_AND_FILES("01 02") {
        TEST(hdag_bundle_from_txt(&bundle, input_file, hash_len) ==
             HDAG_RES_OK);
        TEST(bundle.nodes.slots_occupied == 2);
        TEST(HDAG_BUNDLE_NODE(&bundle, 0)->hash[hash_len - 1] == 2);
        TEST(HDAG_BUNDLE_NODE(&bundle, 1)->hash[hash_len - 1] == 1);
        TEST(bundle.target_hashes.slots_occupied == 1);
        TEST(((uint8_t *)
              hdag_darr_element(&bundle.target_hashes, 0))[hash_len - 1] ==
             2);
        TEST(bundle.extra_edges.slots_occupied == 0);
    }
    WITH_BUNDLES_AND_FILES("01 02") {
        TEST(hdag_bundle_organized_from_txt(
                &bundle, NULL, input_file, hash_len
             ) == HDAG_RES_OK);
        TEST(bundle.nodes.slots_occupied == 2);
        TEST(HDAG_BUNDLE_NODE(&bundle, 0)->hash[hash_len - 1] == 1);
        TEST(hdag_bundle_targets_count(&bundle, 0) == 1);
        TEST(hdag_bundle_targets_node_idx(&bundle, 0, 0) == 1);
        TEST(HDAG_BUNDLE_NODE(&bundle, 1)->hash[hash_len - 1] == 2);
        TEST(bundle.target_hashes.slots_occupied == 0);
        TEST(bundle.extra_edges.slots_occupied == 0);
    }

    /* Test a little more complicated graph */
    WITH_BUNDLES_AND_FILES("01 02 03\n03 02 01") {
        TEST(hdag_bundle_from_txt(&bundle, input_file, hash_len) ==
             HDAG_RES_OK);
        TEST(bundle.nodes.slots_occupied == 6);
        TEST(HDAG_BUNDLE_NODE(&bundle, 0)->hash[hash_len - 1] == 2);
        TEST(HDAG_BUNDLE_NODE(&bundle, 1)->hash[hash_len - 1] == 3);
        TEST(HDAG_BUNDLE_NODE(&bundle, 2)->hash[hash_len - 1] == 1);
        TEST(HDAG_BUNDLE_NODE(&bundle, 3)->hash[hash_len - 1] == 2);
        TEST(HDAG_BUNDLE_NODE(&bundle, 4)->hash[hash_len - 1] == 1);
        TEST(HDAG_BUNDLE_NODE(&bundle, 5)->hash[hash_len - 1] == 3);
        TEST(bundle.target_hashes.slots_occupied == 4);
        TEST(((uint8_t *)
              hdag_darr_element(&bundle.target_hashes, 0))[hash_len - 1] ==
             2);
        TEST(((uint8_t *)
              hdag_darr_element(&bundle.target_hashes, 1))[hash_len - 1] ==
             3);
        TEST(((uint8_t *)
              hdag_darr_element(&bundle.target_hashes, 2))[hash_len - 1] ==
             2);
        TEST(((uint8_t *)
              hdag_darr_element(&bundle.target_hashes, 3))[hash_len - 1] ==
             1);
        TEST(bundle.extra_edges.slots_occupied == 0);
    }
    WITH_BUNDLES_AND_FILES("01 02 03\n03 02 01") {
        TEST(hdag_bundle_organized_from_txt(
                &bundle, NULL, input_file, hash_len
             ) == HDAG_RES_GRAPH_CYCLE);
    }

    WITH_BUNDLES_AND_FILES("01 02 03\n04 01 02") {
        TEST(hdag_bundle_organized_from_txt(
                &bundle, NULL, input_file, hash_len
             ) == HDAG_RES_OK);
        TEST(bundle.nodes.slots_occupied == 4);
        TEST(HDAG_BUNDLE_NODE(&bundle, 0)->hash[hash_len - 1] == 1);
        TEST(hdag_bundle_targets_count(&bundle, 0) == 2);
        TEST(hdag_bundle_targets_node_idx(&bundle, 0, 0) == 1);
        TEST(hdag_bundle_targets_node_idx(&bundle, 0, 1) == 2);
        TEST(HDAG_BUNDLE_NODE(&bundle, 1)->hash[hash_len - 1] == 2);
        TEST(hdag_bundle_targets_count(&bundle, 1) == 0);
        TEST(HDAG_BUNDLE_NODE(&bundle, 2)->hash[hash_len - 1] == 3);
        TEST(hdag_bundle_targets_count(&bundle, 2) == 0);
        TEST(HDAG_BUNDLE_NODE(&bundle, 3)->hash[hash_len - 1] == 4);
        TEST(hdag_bundle_targets_count(&bundle, 3) == 2);
        TEST(hdag_bundle_targets_node_idx(&bundle, 3, 0) == 0);
        TEST(hdag_bundle_targets_node_idx(&bundle, 3, 1) == 1);
        TEST(bundle.target_hashes.slots_occupied == 0);
        TEST(bundle.extra_edges.slots_occupied == 0);
    }

    return failed;
}

static size_t
test_txt_buggy_case(void)
{
    size_t failed = 0;
    struct hdag_bundle bundle;
    FILE *input_file;
    FILE *output_file;
    char output_buf[4096];
    size_t i;

    WITH_BUNDLES_AND_FILES("0a06811d664b8695a7612d3e59c1defb4382f4e0 "
                           "52f1192887f825bd29c1e72303d9e62f8382ba20 "
                           "c3565a35d97102fbea69e324086d5e33ee2ab34e "
                           "339d9d8792aef5b42909c8732ee7c228d0eca310 "
                           "ffa1f26d3ddf6416119f0354bd9ab340dbdb163e\n"
                           "f765274d0c9436bc130911abbd97e52b1648d13c "
                           "58ff04e2e22319e63ea646d9a38890c17836a7f6 "
                           "3c217a182018e6c6d381b3fdc32626275eefbfb0") {
        TEST(hdag_bundle_organized_from_txt(
                &bundle, NULL, input_file, 20
             ) == HDAG_RES_OK);
        TEST(bundle.nodes.slots_occupied == 8);
        uint8_t hashes[][20] = {
            {0x0a, 0x06, 0x81, 0x1d, 0x66, 0x4b, 0x86, 0x95, 0xa7, 0x61,
             0x2d, 0x3e, 0x59, 0xc1, 0xde, 0xfb, 0x43, 0x82, 0xf4, 0xe0},
            {0x33, 0x9d, 0x9d, 0x87, 0x92, 0xae, 0xf5, 0xb4, 0x29, 0x09,
             0xc8, 0x73, 0x2e, 0xe7, 0xc2, 0x28, 0xd0, 0xec, 0xa3, 0x10},
            {0x3c, 0x21, 0x7a, 0x18, 0x20, 0x18, 0xe6, 0xc6, 0xd3, 0x81,
             0xb3, 0xfd, 0xc3, 0x26, 0x26, 0x27, 0x5e, 0xef, 0xbf, 0xb0},
            {0x52, 0xf1, 0x19, 0x28, 0x87, 0xf8, 0x25, 0xbd, 0x29, 0xc1,
             0xe7, 0x23, 0x03, 0xd9, 0xe6, 0x2f, 0x83, 0x82, 0xba, 0x20},
            {0x58, 0xff, 0x04, 0xe2, 0xe2, 0x23, 0x19, 0xe6, 0x3e, 0xa6,
             0x46, 0xd9, 0xa3, 0x88, 0x90, 0xc1, 0x78, 0x36, 0xa7, 0xf6},
            {0xc3, 0x56, 0x5a, 0x35, 0xd9, 0x71, 0x02, 0xfb, 0xea, 0x69,
             0xe3, 0x24, 0x08, 0x6d, 0x5e, 0x33, 0xee, 0x2a, 0xb3, 0x4e},
            {0xf7, 0x65, 0x27, 0x4d, 0x0c, 0x94, 0x36, 0xbc, 0x13, 0x09,
             0x11, 0xab, 0xbd, 0x97, 0xe5, 0x2b, 0x16, 0x48, 0xd1, 0x3c},
            {0xff, 0xa1, 0xf2, 0x6d, 0x3d, 0xdf, 0x64, 0x16, 0x11, 0x9f,
             0x03, 0x54, 0xbd, 0x9a, 0xb3, 0x40, 0xdb, 0xdb, 0x16, 0x3e},
        };
        for (i = 0; i < bundle.nodes.slots_occupied; i++) {
            TEST(memcmp(
                HDAG_BUNDLE_NODE(&bundle, i)->hash, hashes[i], 20
            ) == 0);
        }
        uint32_t target_counts[8] = {4, 0, 0, 0, 0, 0, 2, 0};
        for (i = 0; i < 8; i++) {
            TEST(hdag_bundle_targets_count(&bundle, i) == target_counts[i]);
        }
        uint8_t node0_target_hashes[][20] = {
            {0x33, 0x9d, 0x9d, 0x87, 0x92, 0xae, 0xf5, 0xb4, 0x29, 0x09,
             0xc8, 0x73, 0x2e, 0xe7, 0xc2, 0x28, 0xd0, 0xec, 0xa3, 0x10},
            {0x52, 0xf1, 0x19, 0x28, 0x87, 0xf8, 0x25, 0xbd, 0x29, 0xc1,
             0xe7, 0x23, 0x03, 0xd9, 0xe6, 0x2f, 0x83, 0x82, 0xba, 0x20},
            {0xc3, 0x56, 0x5a, 0x35, 0xd9, 0x71, 0x02, 0xfb, 0xea, 0x69,
             0xe3, 0x24, 0x08, 0x6d, 0x5e, 0x33, 0xee, 0x2a, 0xb3, 0x4e},
            {0xff, 0xa1, 0xf2, 0x6d, 0x3d, 0xdf, 0x64, 0x16, 0x11, 0x9f,
             0x03, 0x54, 0xbd, 0x9a, 0xb3, 0x40, 0xdb, 0xdb, 0x16, 0x3e}
        };
        for (i = 0; i < hdag_bundle_targets_count(&bundle, 0); i++) {
            TEST(memcmp(
                HDAG_BUNDLE_TARGETS_NODE(&bundle, 0, i)->hash,
                node0_target_hashes[i],
                20
            ) == 0);
        }
        uint8_t node6_target_hashes[][20] = {
            {0x3c, 0x21, 0x7a, 0x18, 0x20, 0x18, 0xe6, 0xc6, 0xd3, 0x81,
             0xb3, 0xfd, 0xc3, 0x26, 0x26, 0x27, 0x5e, 0xef, 0xbf, 0xb0},
            {0x58, 0xff, 0x04, 0xe2, 0xe2, 0x23, 0x19, 0xe6, 0x3e, 0xa6,
             0x46, 0xd9, 0xa3, 0x88, 0x90, 0xc1, 0x78, 0x36, 0xa7, 0xf6},
        };
        for (i = 0; i < hdag_bundle_targets_count(&bundle, 6); i++) {
            TEST(memcmp(
                HDAG_BUNDLE_TARGETS_NODE(&bundle, 6, i)->hash,
                node6_target_hashes[i],
                20
            ) == 0);
        }

        /* Output */
        TEST(hdag_bundle_to_txt(output_file, &bundle) == HDAG_RES_OK);
        TEST(strcmp(
            output_buf,
            "0a06811d664b8695a7612d3e59c1defb4382f4e0 "
            "339d9d8792aef5b42909c8732ee7c228d0eca310 "
            "52f1192887f825bd29c1e72303d9e62f8382ba20 "
            "c3565a35d97102fbea69e324086d5e33ee2ab34e "
            "ffa1f26d3ddf6416119f0354bd9ab340dbdb163e\n"
            "f765274d0c9436bc130911abbd97e52b1648d13c "
            "3c217a182018e6c6d381b3fdc32626275eefbfb0 "
            "58ff04e2e22319e63ea646d9a38890c17836a7f6\n"
        ) == 0);
    }

    return failed;
}

#undef WITH_BUNDLES_AND_FILES

/** Test node sequence */
struct test_node_seq {
    /** The base abstract node sequence */
    struct hdag_node_seq    base;
    /**
     * The values of the test DAG hashes being iterated over.
     * A series of zero-terminated hash value sequences, terminated with an
     * empty sequence. Each sequence contains the value of the node's hash,
     * followed by the values of the target hashes.
     */
    const uint32_t         *hash_vals;
    /** The pointer to the next hash value to return */
    const uint32_t         *next_hash_val;
    /**
     * Location of the buffer to store generated and returned node hashes in.
     */
    uint8_t                *hash_buf;
    /**
     * Location of the buffer to store target hashes in.
     */
    uint8_t                *target_hash_buf;
    /** The returned node's target hash sequence */
    struct hdag_hash_seq    target_hash_seq;
};

static hdag_res
test_hash_seq_next(struct hdag_hash_seq *hash_seq,
                   const uint8_t **phash)
{
    assert(hdag_hash_seq_is_valid(hash_seq));
    struct test_node_seq *seq = HDAG_CONTAINER_OF(
        struct test_node_seq, target_hash_seq, hash_seq
    );
    uint32_t hash_val = *seq->next_hash_val++;
    if (hash_val == 0) {
        return 1;
    }
    *phash = hdag_hash_fill(seq->target_hash_buf, seq->base.hash_len,
                            hash_val);
    return 0;
}

static hdag_res
test_node_seq_next(struct hdag_node_seq   *base_seq,
                   const uint8_t         **phash,
                   struct hdag_hash_seq  **ptarget_hash_seq)
{
    assert(hdag_node_seq_is_valid(base_seq));
    struct test_node_seq *seq = HDAG_CONTAINER_OF(
        struct test_node_seq, base, base_seq
    );
    uint32_t hash_val = *seq->next_hash_val++;
    if (hash_val == 0) {
        return 1;
    }
    *phash = hdag_hash_fill(seq->hash_buf, base_seq->hash_len, hash_val);
    *ptarget_hash_seq = &seq->target_hash_seq;
    return 0;
}

static struct hdag_node_seq*
test_node_seq_init(struct test_node_seq *pseq,
                   uint16_t hash_len,
                   uint8_t *hash_buf,
                   uint8_t *target_hash_buf,
                   const uint32_t *hash_vals)
{
    assert(pseq != NULL);
    assert(hdag_hash_len_is_valid(hash_len));
    assert(hash_vals != NULL);
    assert(hash_buf != NULL);
    assert(target_hash_buf != NULL);

    *pseq = (struct test_node_seq){
        .base = {
            .hash_len = hash_len,
            .next_fn = test_node_seq_next,
        },
        .hash_vals = hash_vals,
        .next_hash_val = hash_vals,
        .hash_buf = hash_buf,
        .target_hash_buf = target_hash_buf,
        .target_hash_seq = {
            .hash_len = hash_len,
            .next_fn = test_hash_seq_next,
        },
    };

    assert(hdag_node_seq_is_valid(&pseq->base));
    assert(hdag_hash_seq_is_valid(&pseq->target_hash_seq));
    return &pseq->base;
}

#define TEST_NODE_SEQ(_hash_len, _hash_buf, _target_hash_buf, \
                      _hash_vals...)                                    \
    *test_node_seq_init(&(struct test_node_seq){0,},                    \
                        _hash_len,                                      \
                        _hash_buf,                                      \
                        _target_hash_buf,                               \
                        (const uint32_t []){0, ##_hash_vals, 0} + 1)

#define TEST_NODE(_hash_val, _target_hash_vals...) \
    _hash_val, ##_target_hash_vals, 0

static size_t
test_file(uint16_t hash_len)
{
    size_t failed = 0;
    hdag_res res;
    struct hdag_bundle bundle = HDAG_BUNDLE_EMPTY(0);
    assert(hdag_hash_len_is_valid(hash_len));
    uint8_t hash_buf[hash_len];
    uint8_t target_hash_buf[hash_len];
#define NODE_SEQ(_hash_vals...) \
    TEST_NODE_SEQ(hash_len, hash_buf, target_hash_buf, ##_hash_vals)

#define TEST_FILE(_node_seq) \
    do {                                                                \
        res = hdag_bundle_organized_from_node_seq(                      \
            &bundle, NULL, _node_seq                                    \
        );                                                              \
        TEST(res == HDAG_RES_OK);                                       \
        TEST(hdag_node_seq_cmp(&HDAG_BUNDLE_KNOWN_NODE_SEQ(&bundle),    \
                               _node_seq) - 2 == 0);                    \
        TEST(!hdag_bundle_is_filed(&bundle));                           \
        TEST(hdag_bundle_file(&bundle, NULL, 0, 0) == HDAG_RES_OK);     \
        TEST(hdag_bundle_is_filed(&bundle));                            \
        TEST(hdag_node_seq_cmp(&HDAG_BUNDLE_KNOWN_NODE_SEQ(&bundle),    \
                               _node_seq) - 2 == 0);                    \
        TEST(hdag_bundle_unfile(&bundle) == HDAG_RES_OK);               \
        TEST(!hdag_bundle_is_filed(&bundle));                           \
        TEST(hdag_node_seq_cmp(&HDAG_BUNDLE_KNOWN_NODE_SEQ(&bundle),    \
                               _node_seq) - 2 == 0);                    \
        hdag_bundle_cleanup(&bundle);                                   \
    } while (0)

    TEST_FILE(&NODE_SEQ());
    TEST_FILE(&NODE_SEQ(TEST_NODE(1)));
    TEST_FILE(&NODE_SEQ(TEST_NODE(1), TEST_NODE(2)));
    TEST_FILE(&NODE_SEQ(TEST_NODE(1, 2), TEST_NODE(2)));
    TEST_FILE(&NODE_SEQ(TEST_NODE(1, 2), TEST_NODE(2, 3)));
    TEST_FILE(&NODE_SEQ(TEST_NODE(1, 2, 3),
                        TEST_NODE(2, 3),
                        TEST_NODE(3, 4, 5)));

    return failed;
}

static size_t
test_fanout(uint16_t hash_len)
{
    size_t failed = 0;
    struct hdag_bundle bundle = HDAG_BUNDLE_EMPTY(hash_len);
    ssize_t idx;
    uint32_t *pcount;
    struct hdag_node *node;

    TEST(hdag_fanout_is_valid(NULL, 0));
    TEST(hdag_fanout_is_empty(NULL, 0));
    TEST(hdag_fanout_is_valid((uint32_t []){0}, 1));
    TEST(!hdag_fanout_is_empty((uint32_t []){0}, 1));
    TEST(hdag_fanout_is_zero((uint32_t []){0}, 1));
    TEST(hdag_fanout_is_valid((uint32_t []){UINT32_MAX}, 1));
    TEST(!hdag_fanout_is_zero((uint32_t []){UINT32_MAX}, 1));
    TEST(hdag_fanout_is_valid((uint32_t []){0, 0}, 2));
    TEST(hdag_fanout_is_valid((uint32_t []){0, 1}, 2));
    TEST(!hdag_fanout_is_valid((uint32_t []){1, 0}, 2));
    TEST(hdag_fanout_is_valid((uint32_t []){1, 1, 1}, 3));
    TEST(!hdag_fanout_is_valid((uint32_t []){1, 2, 1}, 3));

    /* Fill fanout in an empty bundle */
    hdag_bundle_fanout_fill(&bundle);
    hdag_bundle_cleanup(&bundle);

    /* Fill fanout in a bundle with one node */
    TEST(hdag_darr_cappend(&bundle.nodes, 1));
    hdag_node_hash_fill(HDAG_BUNDLE_NODE(&bundle, 0), hash_len, 0);
    hdag_bundle_fanout_fill(&bundle);

    HDAG_DARR_ITER_FORWARD(&bundle.nodes_fanout, idx, pcount,
                           (void)0, (void)0) {
        TEST(*pcount == 1);
    }
    hdag_bundle_cleanup(&bundle);

    /* Fill fanout in a bundle with 256 different hashes */
    TEST(hdag_darr_cappend(&bundle.nodes, 256));
    HDAG_DARR_ITER_FORWARD(&bundle.nodes, idx, node,
                           (void)0, (void)0) {
        memset(node->hash, idx, bundle.hash_len);
    }
    hdag_bundle_fanout_fill(&bundle);
    HDAG_DARR_ITER_FORWARD(&bundle.nodes_fanout, idx, pcount,
                           (void)0, (void)0) {
        TEST(*pcount == idx + 1);
    }
    hdag_bundle_cleanup(&bundle);

    /* Fill fanout in a bundle with 16 different hashes with gaps between */
    TEST(hdag_darr_cappend(&bundle.nodes, 16));
    HDAG_DARR_ITER_FORWARD(&bundle.nodes, idx, node,
                           (void)0, (void)0) {
        memset(node->hash, idx * 16, bundle.hash_len);
    }
    hdag_bundle_fanout_fill(&bundle);
    HDAG_DARR_ITER_FORWARD(&bundle.nodes_fanout, idx, pcount,
                           (void)0, (void)0) {
        TEST(*pcount == (idx / 16 + 1));
    }
    hdag_bundle_cleanup(&bundle);

    return failed;
}

static size_t
test(uint16_t hash_len)
{
    size_t failed = 0;
    const struct hdag_bundle empty_bundle = HDAG_BUNDLE_EMPTY(hash_len);
    struct hdag_bundle compacted_empty_bundle = HDAG_BUNDLE_EMPTY(hash_len);
    struct hdag_bundle bundle;

    TEST(hdag_bundle_is_valid(&empty_bundle));
    TEST(hdag_bundle_is_clean(&empty_bundle));
    TEST(hdag_bundle_is_sorted_and_deduped(&empty_bundle));
    TEST(!hdag_bundle_has_index_targets(&empty_bundle));
    TEST(!hdag_bundle_has_hash_targets(&empty_bundle));

    bundle = empty_bundle;
    TEST(hdag_bundle_sort_and_dedup(&bundle, false) == HDAG_RES_OK);
    TEST(memcmp(&bundle, &empty_bundle, sizeof(struct hdag_bundle)) == 0);

    bundle = empty_bundle;
    TEST(hdag_bundle_compact(&bundle) == HDAG_RES_OK);
    TEST(memcmp(&bundle, &compacted_empty_bundle,
                sizeof(struct hdag_bundle)) == 0);

    hdag_bundle_cleanup(&bundle);
    TEST(memcmp(&bundle, &empty_bundle, sizeof(struct hdag_bundle)) == 0);

    /*
     * Check deduplicating works.
     */
    failed += test_sorting_and_deduplicating(hash_len);

    /*
     * Check fanout generation works.
     */
    failed += test_fanout(hash_len);

    /*
     * Check compacting works.
     */
    failed += test_compacting(hash_len);

    /*
     * Check inverting works.
     */
    failed += test_inverting(hash_len);

    /*
     * Check generation and component enumeration works.
     */
    failed += test_enumerating(hash_len);

    /*
     * Check adjacency list text file processing works.
     */
    failed += test_txt(hash_len);

    /*
     * Check HDAG file operations work
     */
    failed += test_file(hash_len);

    /* Cleanup the bundle */
    hdag_bundle_cleanup(&bundle);

    return failed;
}

int
main(void)
{
    size_t failed = test(4) + test(32) + test(256) + test(1024);
    failed += test_txt_buggy_case();
    if (failed) {
        fprintf(stderr, "%zu tests failed.\n", failed);
    }
    return failed != 0;
}
