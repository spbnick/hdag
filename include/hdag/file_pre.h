/*
 * Hash DAG file preprocessing
 */

#ifndef _HDAG_FILE_PRE_H
#define _HDAG_FILE_PRE_H

#include <hdag/edge.h>
#include <hdag/node.h>
#include <hdag/node_seq.h>

/** A preprocessed file */
struct hdag_file_pre {
    /** The length of the node hash */
    uint16_t            hash_len;

    /** Number of collected nodes */
    size_t              nodes_num;
    /** Number of allocated node slots */
    size_t              nodes_allocated;
    /**
     * Nodes.
     * Before compacting, their targets are either unknown, or are indirect
     * indices pointing into the target_hashes array. After compacting their
     * targets are either unknown, direct indices pointing into the same
     * array, or indirect indices pointing into the extra_edges array.
     */
    struct hdag_node   *nodes;

    /** Number of collected target hashes */
    size_t              target_hashes_num;
    /** Number of allocated target hash slots */
    size_t              target_hashes_allocated;
    /** Target hashes */
    uint8_t            *target_hashes;

    /** Number of collected extra edges */
    size_t              extra_edges_num;
    /** Number of allocated extra edge slots */
    size_t              extra_edges_allocated;
    /** The array of extra edges, which didn't fit into nodes themselves */
    struct hdag_edge   *extra_edges;
};

/** An empty preprocessed file */
#define HDAG_FILE_PRE_EMPTY (struct hdag_file_pre){0, }

/**
 * Cleanup a preprocessed file, freeing associated resources (but not the
 * file structure itself).
 *
 * @param file_pre  The preprocessed file to cleanup.
 */
extern void hdag_file_pre_cleanup(struct hdag_file_pre *file_pre);

/**
 * Load a node sequence (adjacency list) into a preprocessed file.
 *
 * @param pfile_pre Location for the preprocessed file.
 *                  Not modified in case of error.
 *                  Can be NULL to have the preprocessed file discarded.
 * @param hash_len  The length of the node hashes.
 * @param node_seq  The sequence of nodes (and optionally their
 *                  targets) to load.
 *
 * @return True if the data was loaded and preprocessed successfully,
 *         false if not. The errno is set in case of failure.
 */
extern bool hdag_file_pre_create(struct hdag_file_pre *pfile_pre,
                                 uint16_t hash_len,
                                 struct hdag_node_seq node_seq);

#endif /* _HDAG_FILE_PRE_H */
