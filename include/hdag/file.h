/**
 * Hash DAG file
 *
 * NOTE: Files use host order.
 */

#ifndef _HDAG_FILE_H
#define _HDAG_FILE_H

#include <hdag/hash.h>
#include <hdag/fanout.h>
#include <hdag/node.h>
#include <hdag/edge.h>
#include <hdag/res.h>
#include <hdag/misc.h>
#include <fcntl.h>
#include <linux/limits.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

/** The starting signature of the file */
#define HDAG_FILE_SIGNATURE (uint32_t)('H' | 'D' << 8 | 'A' << 16 | 'G'<< 24 )

/** The file header */
struct hdag_file_header {
    /** The initial file signature (must be HDAG_FILE_SIGNATURE) */
    uint32_t    signature;
    /** The major and the minor version numbers */
    struct {
        uint8_t major;
        uint8_t minor;
    } version;
    /** Hash length, bytes, must be divisible by four */
    uint16_t    hash_len;
    union {
        /**
         * Node hash fanout - an array where each element
         * corresponds to the number of nodes having the first byte of their
         * hash equal to or lower than the element index.
         */
        uint32_t    node_fanout[256];
        struct {
            uint32_t    _padding[255];
            /** Number of all nodes (with hash[0] <= 255) */
            uint32_t    node_num;
        };
    };
    /** Number of extra edges */
    uint32_t    extra_edge_num;
    /** Number of indexes of unknown nodes */
    uint32_t    unknown_index_num;
};

HDAG_ASSERT_STRUCT_MEMBERS_PACKED(
    hdag_file_header,
    signature,
    version.major,
    version.minor,
    hash_len,
    node_fanout,
    extra_edge_num,
    unknown_index_num
);

/**
 * Check that a file header is valid.
 *
 * @param header    The file header to check.
 *
 * @return True if the header is valid, false otherwise.
 */
static inline bool
hdag_file_header_is_valid(const struct hdag_file_header *header)
{
    return header != NULL &&
        header->signature == HDAG_FILE_SIGNATURE &&
        header->version.major == 0 &&
        header->version.minor == 0 &&
        hdag_hash_len_is_valid(header->hash_len) &&
        hdag_fanout_is_valid(header->node_fanout,
                             HDAG_ARR_LEN(header->node_fanout)) &&
        ffs(header->node_num) <= header->hash_len * 8 &&
        /* A file has to have less unknown nodes than all nodes */
        (header->node_num == 0
         ? (header->unknown_index_num == 0)
         : (header->unknown_index_num < header->node_num));
}

/**
 * The file state.
 * Considered closed if initialized to zeroes.
 */
struct hdag_file {
    /** The file pathname. NULL, if there's no backing file */
    char   *pathname;
    /** The mapped file contents, NULL if there's no contents */
    void   *contents;
    /** The size of file contents, only valid when `contents` != NULL */
    size_t  size;

    /* Pointers to pieces of the contents, only valid, if contents != NULL */

    /** The file header */
    struct hdag_file_header    *header;

    /**
     * The node array.
     *
     * The node's target's direct indexes point into this array.
     * The indirect ones point into the extra_edges array.
     */
    struct hdag_node           *nodes;

    /** The edge array */
    struct hdag_edge           *extra_edges;

    /** The array of indexes of unknown nodes (duplicating "nodes" info) */
    uint32_t                   *unknown_indexes;
};

/** An initializer for a closed file */
#define HDAG_FILE_CLOSED (struct hdag_file){0, }

/**
 * Calculate the file size from the contents parameters.
 *
 * @param hash_len          The length of the node ID hash.
 * @param node_num          Number of nodes.
 * @param extra_edge_num    Number of extra edges (the sum of number of
 *                          outgoing edges of all nodes, which have more than
 *                          two of them).
 * @param unknown_index_num Number of indexes of unknown nodes (must be less
 *                          than number of nodes, if there are any).
 *
 * @return The size of the file, in bytes.
 */
static size_t
hdag_file_size(uint16_t hash_len,
               uint32_t node_num,
               uint32_t extra_edge_num,
               uint32_t unknown_index_num)
{
    assert(hdag_hash_len_is_valid(hash_len));
    assert(node_num == 0
           ? (unknown_index_num == 0)
           : (unknown_index_num < node_num));
    return sizeof(struct hdag_file_header) +
           hdag_node_size(hash_len) * node_num +
           sizeof(struct hdag_edge) * extra_edge_num +
           sizeof(uint32_t) * unknown_index_num;
}

/**
 * Create a hash DAG file, filling it with the supplied data.
 *
 * @param pfile             Location for the state of the opened file.
 *                          Not modified in case of failure.
 *                          Can be NULL to have the file closed after
 *                          creation.
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
 * @param hash_len          The length of all the hashes in the data.
 * @param nodes             The node array. Size of each node depends on the
 *                          "hash_len", and can be calculated using
 *                          hdag_node_size(). Sorted lexicographically by
 *                          hashes.
 * @param node_fanout       Node hash fanout - an array where each element
 *                          corresponds to the number of nodes having the
 *                          first byte of their hash equal to or lower than
 *                          the element index. Thus the array has 256
 *                          elements, and its last element contains the total
 *                          number of nodes.
 *                          The node's target's direct indexes point into this
 *                          array. The indirect ones point into the
 *                          extra_edges array.
 * @param extra_edges       The array of edges which didn't fit into the
 *                          nodes' targets, referenced by nodes.
 * @param extra_edge_num    The number of edges in the "extra_edges" array.
 * @param unknown_indexes   The array of (uint32_t) indexes of "unknown" nodes
 *                          in the "nodes" array, sorted by component ID.
 * @param unknown_index_num The number of indexes in the "unknown_indexes"
 *                          array.
 *
 * @return A void universal result.
 */
[[nodiscard]]
extern hdag_res hdag_file_create(struct hdag_file *pfile,
                                 const char *pathname,
                                 int template_sfxlen,
                                 mode_t open_mode,
                                 uint16_t hash_len,
                                 const struct hdag_node *nodes,
                                 const uint32_t *node_fanout,
                                 const struct hdag_edge *extra_edges,
                                 uint32_t extra_edge_num,
                                 const uint32_t *unknown_indexes,
                                 uint32_t unknown_index_num);

/**
 * Open a previously-created hash DAG file.
 *
 * @param pfile         Location for the state of the opened file.
 *                      Not modified in case of failure.
 *                      Can be NULL to have the file closed after opening.
 * @param pathname      The file's pathname. Cannot be NULL.
 *
 * @return A void universal result.
 */
[[nodiscard]]
extern hdag_res hdag_file_open(struct hdag_file *pfile,
                               const char *pathname);

/**
 * Check if an opened or closed file is valid.
 *
 * @param file  Location of the file to check.
 *
 * @return True if the file is valid. False otherwise.
 */
static inline bool
hdag_file_is_valid(const struct hdag_file *file)
{
    return
        file != NULL &&
        file->contents == file->header &&
        (file->contents == NULL) == (file->nodes == NULL) &&
        (file->contents == NULL) == (file->extra_edges == NULL) &&
        (file->contents == NULL) == (file->unknown_indexes == NULL) &&
        (
            file->contents == NULL ||
            (
                hdag_file_header_is_valid(file->header) &&
                file->size == hdag_file_size(
                    file->header->hash_len,
                    file->header->node_num,
                    file->header->extra_edge_num,
                    file->header->unknown_index_num
                )
            )
        );
}

/**
 * Check if a file is open.
 *
 * @param file  Location of the file to check. Must be valid.
 *
 * @return True if the file is open. False otherwise.
 */
static inline bool
hdag_file_is_open(const struct hdag_file *file)
{
    assert(hdag_file_is_valid(file));
    return file->contents != NULL;
}

/**
 * Check if an HDAG file is backed by a file on-disc.
 *
 * @param file  The file to check. Must be open.
 *
 * @return True if the file has a file on disc, false otherwise.
 */
static inline bool
hdag_file_is_backed(const struct hdag_file *file)
{
    assert(hdag_file_is_valid(file));
    assert(hdag_file_is_open(file));
    return file->pathname != NULL;
}

/**
 * Sync the contents of an HDAG file to the on-disc file, if backed by one.
 * Do nothing, if not.
 *
 * @param file  The file to sync. Must be open.
 *
 * @return A void universal result.
 */
[[nodiscard]]
static inline hdag_res
hdag_file_sync(struct hdag_file *file)
{
    assert(hdag_file_is_valid(file));
    assert(hdag_file_is_open(file));
    if (hdag_file_is_backed(file) &&
        msync(file->contents, file->size, MS_SYNC) != 0) {
        return HDAG_RES_ERRNO;
    }
    return HDAG_RES_OK;
}

/**
 * Change the name of a "backed" file.
 *
 * @param file      The backed file to change the name of.
 *                  Not modified in case of failure. Must be "backed".
 * @param pathname  The pathname to rename the file to.
 *                  If NULL, the file is removed ("unlinked").
 *
 * @return A void universal result.
 */
[[nodiscard]]
extern hdag_res hdag_file_rename(struct hdag_file *file,
                                 const char *pathname);

/**
 * Unlink (delete) the file behind the "backed" file.
 * The file stops being "backed" as a result.
 *
 * @param file  The file to unlink the file of.
 *              Not modified in case of failure. Must be "backed".
 *
 * @return A void universal result.
 */
[[nodiscard]]
extern hdag_res hdag_file_unlink(struct hdag_file *file);

/**
 * Close a previously-opened hash DAG file.
 *
 * @param pfile Location of the opened file.
 *              Not modified on failure.
 *              Must be valid.
 *
 * @return A void universal result.
 */
[[nodiscard]]
extern hdag_res hdag_file_close(struct hdag_file *pfile);

#endif /* _HDAG_FILE_H */
