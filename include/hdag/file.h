/*
 * Hash DAG file operations
 *
 * NOTE: Files use host order.
 */

#ifndef _HDAG_FILE_H
#define _HDAG_FILE_H

#include <hdag/edge.h>
#include <hdag/node_seq.h>
#include <hdag/node.h>
#include <hdag/targets.h>
#include <hdag/hash.h>
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
    /** Number of nodes */
    uint32_t    node_num;
    /** Number of extra edges */
    uint32_t    extra_edge_num;
};

HDAG_ASSERT_STRUCT_MEMBERS_PACKED(
    hdag_file_header,
    signature,
    version.major,
    version.minor,
    hash_len,
    node_num,
    extra_edge_num
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
        ffs(header->node_num) <= header->hash_len * 8;
}

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

    /**
     * The node array.
     *
     * The node's target's direct indexes point into this array.
     * The indirect ones point into the extra_edges array.
     */
    struct hdag_node           *nodes;

    /** The edge array */
    struct hdag_edge           *extra_edges;
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
 *
 * @return The size of the file, in bytes.
 */
static size_t
hdag_file_size(uint16_t hash_len,
               uint32_t node_num,
               uint32_t extra_edge_num)
{
    assert(hdag_hash_len_is_valid(hash_len));
    return sizeof(struct hdag_file_header) +
           hdag_node_size(hash_len) * node_num +
           sizeof(struct hdag_edge) * extra_edge_num;
}

/**
 * Create and open a hash DAG file with specified parameters.
 *
 * @param pfile             Location for the state of the opened file.
 *                          Not modified in case of failure.
 *                          Can be NULL to have the file closed after
 *                          creation.
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
 * @param hash_len          The length of the node hashes, bytes.
 * @param node_seq          The sequence of nodes (and optionally their
 *                          targets) to store in the created file.
 *
 * @return True if the file was successfully created and opened, false if not.
 *         The errno is set in case of failure.
 */
extern bool hdag_file_create(struct hdag_file *pfile,
                             const char *pathname,
                             int template_sfxlen,
                             mode_t open_mode,
                             uint16_t hash_len,
                             struct hdag_node_seq node_seq);

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
        (file->contents == NULL) == (file->extra_edges == NULL) &&
        (
            file->contents == NULL ||
            (
                hdag_file_header_is_valid(file->header) &&
                file->size == hdag_file_size(
                    file->header->hash_len,
                    file->header->node_num,
                    file->header->extra_edge_num
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
