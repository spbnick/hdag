/*
 * Hash DAG file operations
 *
 * NOTE: Files use host order.
 */

#ifndef _HDAG_FILE_H
#define _HDAG_FILE_H

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
    /** Hash length, bytes */
    uint16_t    hash_len;
    /** Number of nodes */
    uint32_t    node_num;
    /** Number of (extra) edges */
    uint32_t    edge_num;
} __attribute__((packed));

static_assert(
    sizeof(struct hdag_file_header) ==
    sizeof(((struct hdag_file_header *)0)->signature) +
    sizeof(((struct hdag_file_header *)0)->version.major) +
    sizeof(((struct hdag_file_header *)0)->version.minor) +
    sizeof(((struct hdag_file_header *)0)->hash_len) +
    sizeof(((struct hdag_file_header *)0)->node_num) +
    sizeof(((struct hdag_file_header *)0)->edge_num),
    "The hdag_file_header structure is not packed"
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
        ffs(header->node_num) <= header->hash_len * 8;
}

/** An node's (outgoing edge's) target value */
typedef uint32_t    hdag_file_target;

/** A node's invalid (absent) target (outgoing edge's target) */
#define HDAG_FILE_TARGET_INVALID    (hdag_file_target)0

/** A node's unknown target (outgoing edge's target) */
#define HDAG_FILE_TARGET_UNKNOWN    (hdag_file_target)UINT32_MAX

/** Minimum target value for a node index */
#define HDAG_FILE_TARGET_NODE_IDX_MIN   (HDAG_FILE_TARGET_INVALID + 1)
/** Maximum target value for a node index */
#define HDAG_FILE_TARGET_NODE_IDX_MAX   (hdag_file_target)INT32_MAX

/** Minimum target value for an edge index */
#define HDAG_FILE_TARGET_EDGE_IDX_MIN   (HDAG_FILE_TARGET_NODE_IDX_MAX + 1)
/** Maximum target value for an edge index */
#define HDAG_FILE_TARGET_EDGE_IDX_MAX   (HDAG_FILE_TARGET_UNKNOWN - 1)

/**
 * Check if a node's target is a node index.
 *
 * @param target    The target value to check.
 *
 * @return True if the target is a node index, false otherwise.
 */
static inline bool
hdag_file_target_is_node_idx(hdag_file_target target)
{
    return target >= HDAG_FILE_TARGET_NODE_IDX_MIN &&
        target <= HDAG_FILE_TARGET_NODE_IDX_MAX;
}

/**
 * Check if a node's target is an outgoing edge list boundary index.
 *
 * @param target    The target value to check.
 *
 * @return True if the target is an outgoing edge list boundary index,
 *         false otherwise.
 */
static inline bool
hdag_file_target_is_edge_idx(hdag_file_target target)
{
    return target >= HDAG_FILE_TARGET_EDGE_IDX_MIN &&
        target <= HDAG_FILE_TARGET_EDGE_IDX_MAX;
}

/**
 * Extract the target node's index from a target value.
 *
 * @param target    The target value to extract the node index from.
 *
 * @return The node index.
 */
static inline size_t
hdag_file_target_get_node_idx(hdag_file_target target)
{
    assert(hdag_file_target_is_node_idx(target));
    return target - HDAG_FILE_TARGET_NODE_IDX_MIN;
}

/**
 * Extract the outgoing edge list boundary from a target value.
 *
 * @param target    The target value to extract outgoing edge list boundary
 *                  index from.
 *
 * @return The edge index.
 */
static inline size_t
hdag_file_target_get_edge_idx(hdag_file_target target)
{
    assert(hdag_file_target_is_edge_idx(target));
    return target - HDAG_FILE_TARGET_EDGE_IDX_MIN;
}

/** A reference to targets of a node */
struct hdag_file_targets {
    /** The first target */
    hdag_file_target    first;
    /** The last target */
    hdag_file_target    last;
};

/** A node */
struct hdag_file_node {
    /** The node's generation number */
    uint32_t generation;
    /** The targets of outgoing edges */
    struct hdag_file_targets targets;
    /** The hash with length of struct hdag_file_header.hash_len */
    uint8_t hash[];
};

/**
 * Calculate the size of a node based on the ID hash length.
 *
 * @param hash_len  The length of the node ID hash, bytes.
 */
static inline size_t
hdag_file_node_size(uint16_t hash_len)
{
    return sizeof(struct hdag_file_node) + hash_len;
}

/** An edge */
struct hdag_file_edge {
    /** The edge's target node index */
    uint32_t    node_idx;
};

/**
 * The file state.
 * Considered closed if initialized to zeroes.
 */
struct hdag_file {
    /** The file pathname. An empty string, if there's no backing file */
    char    pathname[PATH_MAX];
    /** The mapped file contents, NULL if there's no contents */
    void   *contents;
    /** The size of file contents, only valid when `contents` != NULL */
    size_t  size;

    /* Pointers to pieces of the contents, only valid, if contents != NULL */

    /** The file header */
    struct hdag_file_header    *header;

    /** The node array */
    struct hdag_file_node      *nodes;

    /** The edge array */
    struct hdag_file_edge      *edges;
};

/** An initializer for a closed file */
#define HDAG_FILE_CLOSED (struct hdag_file){0, }

/**
 * Calculate the file size from the contents parameters.
 *
 * @param hash_len  The length of the node ID hash.
 * @param node_num  Number of nodes.
 * @param edge_num  Number of extra edges (the sum of number of outgoing edges
 *                  of all nodes, which have more than two of them).
 *
 * @return The size of the file, in bytes.
 */
static size_t
hdag_file_size(uint16_t hash_len,
               uint32_t node_num,
               uint32_t edge_num)
{
    return sizeof(struct hdag_file_header) +
           hdag_file_node_size(hash_len) * node_num +
           sizeof(struct hdag_file_edge) * edge_num;
}

/**
 * Create and open an empty hash DAG file with specified parameters.
 *
 * @param pfile             Location for the state of the opened file.
 *                          Not modified in case of failure.
 *                          Can be NULL to have the file closed after opening.
 * @param pathname          The file's pathname (template), or empty string to
 *                          open an in-memory file. Cannot be longer than
 *                          PATH_MAX, including the terminating '\0'.
 * @param template_sfxlen   The (non-negative) number of suffix characters
 *                          following the "XXXXXX" at the end of "pathname",
 *                          if it contains the template for a temporary file
 *                          to be created. Or a negative number to treat
 *                          "pathname" literally. Ignored, if "pathname" is
 *                          empty.
 * @param open_mode         The mode bitmap to supply to open(2).
 *                          Ignored, if pathname is empty.
 * @param hash_len          The length of the hashes, bytes.
 * @param node_num          Number of blank nodes to allocate.
 * @param edge_num          Number of blank edges to allocate.
 *
 * @return True if the file was successfully created and open, false if not.
 *         The errno is set in case of failure.
 */
extern bool hdag_file_create(struct hdag_file *pfile,
                             const char *pathname,
                             int template_sfxlen,
                             mode_t open_mode,
                             uint16_t hash_len,
                             uint32_t node_num,
                             uint32_t edge_num);

/**
 * Open a previously-created hash DAG file.
 *
 * @param pfile         Location for the state of the opened file.
 *                      Not modified in case of failure.
 *                      Can be NULL to have the file closed after opening.
 * @param pathname      The file's pathname. Cannot be empty. Cannot be longer
 *                      than PATH_MAX, including the terminating '\0'.
 *
 * @return True if the file was successfully open, false if not.
 *         The errno is set in case of failure.
 */
extern bool hdag_file_open(struct hdag_file *pfile,
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
        memchr(file->pathname, 0, sizeof(file->pathname)) != NULL &&
        file->contents == file->header &&
        (file->contents == NULL) == (file->nodes == NULL) &&
        (file->contents == NULL) == (file->edges == NULL) &&
        (
            file->contents == NULL ||
            file->size == hdag_file_size(
                file->header->hash_len,
                file->header->node_num,
                file->header->edge_num
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
    return *file->pathname != 0;
}

/**
 * Sync the contents of an HDAG file to the on-disc file, if backed by one.
 * Do nothing, if not.
 *
 * @param file  The file to sync. Must be open.
 *
 * @return True if the file synced successfully, false otherwise.
 */
static inline bool
hdag_file_sync(struct hdag_file *file)
{
    assert(hdag_file_is_valid(file));
    assert(hdag_file_is_open(file));
    if (hdag_file_is_backed(file)) {
        return msync(file->contents, file->size, MS_SYNC) == 0;
    }
    return true;
}

/**
 * Close a previously-opened hash DAG file.
 *
 * @param pfile Location of the opened file.
 *              Not modified on failure.
 *              Must be valid.
 *
 * @return True if the file closed successfully. False otherwise.
 */
extern bool hdag_file_close(struct hdag_file *pfile);

#endif /* _HDAG_FILE_H */
