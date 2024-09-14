/*
 * Hash DAG database file operations test
 */

#include <hdag/file.h>
#include <hdag/rc.h>
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

/** The length of test hashes, bytes */
#define TEST_HASH_LEN   32
/** Number of each kind of test objects in a test graph */
#define TEST_OBJ_NUM    32

struct test_node {
    /** The node's hash */
    uint8_t hash[TEST_HASH_LEN];
    /** The node's target hashes, terminated by all-zero hash */
    uint8_t target_hashes[TEST_OBJ_NUM][TEST_HASH_LEN];
};

#define TEST_HASH(_byte) {_byte,},

#define TEST_NODE(_hash_byte, ...) ((struct test_node){ \
    .hash = {_hash_byte},                                               \
    .target_hashes = {                                                  \
        HDAG_EXPAND_FOR_EACH(TEST_HASH, ##__VA_ARGS__) TEST_HASH(0)    \
    },                                                                  \
})

struct test_graph {
    /**
     * The nodes and targets of the graph.
     * Terminated by a node with all-zero hash.
     */
    struct test_node nodes[TEST_OBJ_NUM];
};

#define TEST_GRAPH(...) ((struct test_graph){ \
    .nodes = {__VA_ARGS__, TEST_NODE(0)}        \
})

struct test_node_seq {
    /** The graph being traversed, cannot be NULL */
    const struct test_graph *graph;
    /** The index of the next node to return */
    size_t node_idx;
    /** The index of the next node target to return */
    size_t target_idx;
};

#define TEST_NODE_SEQ(...) ((struct hdag_node_seq){ \
    .next = test_node_seq_next,                     \
    .data = &(struct test_node_seq){                \
        .graph = &TEST_GRAPH(__VA_ARGS__)           \
    }                                               \
})

static int
test_hash_seq_next(void *hash_seq_data, uint8_t *phash)
{
    assert(hash_seq_data != NULL);
    assert(phash != NULL);
    struct test_node_seq *seq = hash_seq_data;
    const struct test_node *node = &seq->graph->nodes[seq->node_idx];
    const uint8_t *target_hash = node->target_hashes[seq->target_idx];
    assert(seq->graph != NULL);
    assert(seq->node_idx < TEST_OBJ_NUM);
    assert(seq->target_idx < TEST_OBJ_NUM);
    /* If the target hash is invalid */
    if (hdag_hash_is_filled(target_hash, TEST_HASH_LEN, 0)) {
        /* No more hashes */
        seq->node_idx++;
        return 1;
    }
    memcpy(phash, target_hash, TEST_HASH_LEN);
    seq->target_idx++;
    /* Hash retrieved */
    return 0;
}

static int
test_node_seq_next(void *node_seq_data,
                   uint8_t *phash,
                   struct hdag_hash_seq *ptarget_hash_seq)
{
    assert(node_seq_data != NULL);
    assert(phash != NULL);
    assert(ptarget_hash_seq != NULL);
    struct test_node_seq *seq = node_seq_data;
    const struct test_node *node = &seq->graph->nodes[seq->node_idx];
    assert(seq->graph != NULL);
    assert(seq->node_idx < TEST_OBJ_NUM);
    assert(seq->target_idx < TEST_OBJ_NUM);

    /* If the next node is invalid */
    if (hdag_hash_is_filled(node->hash, TEST_HASH_LEN, 0)) {
        /* No more nodes */
        return 1;
    }
    memcpy(phash, node->hash, TEST_HASH_LEN);
    seq->target_idx = 0;
    *ptarget_hash_seq = (struct hdag_hash_seq){test_hash_seq_next, seq};
    /* Node retrieved */
    return 0;
}

static size_t
test_empty(void)
{
    size_t failed = 0;
    struct hdag_file file = HDAG_FILE_CLOSED;
    char pathname[sizeof(file.pathname)];
    uint8_t expected_contents[] = {
        0x48, 0x44, 0x41, 0x47, 0x00, 0x00, 0x20, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    };

    /*
     * Empty in-memory file.
     */
    TEST(!hdag_file_create(&file, "", -1, 0,
                           TEST_HASH_LEN, HDAG_NODE_SEQ_EMPTY));
    TEST(file.size = sizeof(expected_contents));
    TEST(memcmp(file.contents, expected_contents, file.size) == 0);
    TEST(!hdag_file_close(&file));

    /*
     * Create empty on-disk file.
     */
    TEST(!hdag_file_create(&file, "test.XXXXXX.hdag", 5,
                           S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP,
                           TEST_HASH_LEN, HDAG_NODE_SEQ_EMPTY));
    TEST(hdag_file_is_open(&file));
    /* Remember created file pathname */
    memcpy(pathname, file.pathname, sizeof(pathname));
    TEST(!hdag_file_sync(&file));
    TEST(file.size == sizeof(expected_contents));
    TEST(memcmp(file.contents, expected_contents, file.size) == 0);
    TEST(!hdag_file_close(&file));
    TEST(!hdag_file_is_open(&file));

    /*
     * Open (the created) empty on-disk file.
     */
    TEST(!hdag_file_open(&file, pathname));
    TEST(hdag_file_is_open(&file));
    TEST(file.size == sizeof(expected_contents));
    TEST(memcmp(file.contents, expected_contents, file.size) == 0);
    TEST(!hdag_file_close(&file));
    TEST(unlink(pathname) == 0);
    TEST(sizeof(struct hdag_file_header) == 4 + 2 + 2 + 4 + 4);

    return failed;
}

static size_t
test_basic(void)
{
    size_t failed = 0;
    struct hdag_file file = HDAG_FILE_CLOSED;
    struct hdag_node *node;

    /*
     * Single-node in-memory file.
     */
    TEST(!hdag_file_create(
            &file, "", -1, 0, TEST_HASH_LEN,
            TEST_NODE_SEQ(TEST_NODE(1))
    ));
    TEST(file.pathname[0] == 0);
    TEST(file.contents != NULL);
    TEST(file.header != NULL);
    TEST(file.header->signature == HDAG_FILE_SIGNATURE);
    TEST(file.header->version.major == 0);
    TEST(file.header->version.minor == 0);
    TEST(file.header->hash_len == TEST_HASH_LEN);
    TEST(file.header->node_num == 1);
    TEST(file.header->extra_edge_num == 0);
    TEST(file.nodes != NULL);
    node = hdag_node_off(file.nodes, TEST_HASH_LEN, 0);
    TEST(node->component == 1);
    TEST(node->generation == 1);
    TEST(node->targets.first == HDAG_TARGET_ABSENT);
    TEST(node->targets.last == HDAG_TARGET_ABSENT);
    TEST(node->hash[0] == 1);
    TEST(file.extra_edges != NULL);
    hdag_file_close(&file);

    /*
     * Two-node in-memory file.
     */
    TEST(!hdag_file_create(
            &file, "", -1, 0, TEST_HASH_LEN,
            TEST_NODE_SEQ(TEST_NODE(1), TEST_NODE(2))
    ));
    TEST(file.header->node_num == 2);
    TEST(file.header->extra_edge_num == 0);
    TEST(file.nodes != NULL);
    node = hdag_node_off(file.nodes, TEST_HASH_LEN, 0);
    TEST(node->hash[0] == 1);
    TEST(node->component == 1);
    TEST(node->generation == 1);
    TEST(node->targets.first == HDAG_TARGET_ABSENT);
    TEST(node->targets.last == HDAG_TARGET_ABSENT);
    node = hdag_node_off(file.nodes, TEST_HASH_LEN, 1);
    TEST(node->hash[0] == 2);
    TEST(node->component == 2);
    TEST(node->generation == 1);
    TEST(node->targets.first == HDAG_TARGET_ABSENT);
    TEST(node->targets.last == HDAG_TARGET_ABSENT);
    TEST(file.extra_edges != NULL);
    hdag_file_close(&file);

    /*
     * N1->N2 in-memory file.
     */
    TEST(!hdag_file_create(
            &file, "", -1, 0, TEST_HASH_LEN,
            TEST_NODE_SEQ(TEST_NODE(1, 2), TEST_NODE(2))
    ));
    TEST(file.header->node_num == 2);
    TEST(file.header->extra_edge_num == 0);
    TEST(file.nodes != NULL);
    node = hdag_node_off(file.nodes, TEST_HASH_LEN, 0);
    TEST(node->hash[0] == 1);
    TEST(node->component == 1);
    TEST(node->generation == 2);
    TEST(node->targets.first == hdag_target_from_dir_idx(1));
    TEST(node->targets.last == HDAG_TARGET_ABSENT);
    node = hdag_node_off(file.nodes, TEST_HASH_LEN, 1);
    TEST(node->hash[0] == 2);
    TEST(node->component == 1);
    TEST(node->generation == 1);
    TEST(node->targets.first == HDAG_TARGET_ABSENT);
    TEST(node->targets.last == HDAG_TARGET_ABSENT);
    TEST(file.extra_edges != NULL);
    hdag_file_close(&file);

    /*
     * N1<-N2 in-memory file.
     */
    TEST(!hdag_file_create(
            &file, "", -1, 0, TEST_HASH_LEN,
            TEST_NODE_SEQ(TEST_NODE(1), TEST_NODE(2, 1))
    ));
    TEST(file.header->node_num == 2);
    TEST(file.header->extra_edge_num == 0);
    TEST(file.nodes != NULL);
    node = hdag_node_off(file.nodes, TEST_HASH_LEN, 0);
    TEST(node->hash[0] == 1);
    TEST(node->component == 1);
    TEST(node->generation == 1);
    TEST(node->targets.first == HDAG_TARGET_ABSENT);
    TEST(node->targets.last == HDAG_TARGET_ABSENT);
    node = hdag_node_off(file.nodes, TEST_HASH_LEN, 1);
    TEST(node->hash[0] == 2);
    TEST(node->component == 1);
    TEST(node->generation == 2);
    TEST(node->targets.first == hdag_target_from_dir_idx(0));
    TEST(node->targets.last == HDAG_TARGET_ABSENT);
    TEST(file.extra_edges != NULL);
    hdag_file_close(&file);

    return failed;
}

static size_t
test(void)
{
    size_t failed = 0;

    failed += test_empty();
    failed += test_basic();

    return failed;
}

int
main(void)
{
    size_t failed = test();
    if (failed) {
        fprintf(stderr, "%zu tests failed.\n", failed);
    }
    return failed != 0;
}
