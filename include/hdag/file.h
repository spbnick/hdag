/*
 * Hash DAG file
 *
 * NOTE: Files use host order.
 */

#ifndef _HDAG_FILE_H
#define _HDAG_FILE_H

#include <hdag/bundle.h>
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
};

HDAG_ASSERT_STRUCT_MEMBERS_PACKED(
    hdag_file_header,
    signature,
    version.major,
    version.minor,
    hash_len,
    node_fanout,
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
        hdag_fanout_is_valid(header->node_fanout,
                             HDAG_ARR_LEN(header->node_fanout)) &&
        ffs(header->node_num) <= header->hash_len * 8;
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
 * Create and open a hash DAG file, filling it with the contents of a bundle.
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
 * @param bundle            The bundle to get the file contents from.
 *                          Must be fully optimized (have generations and
 *                          components enumerated).
 *
 * @return A void universal result.
 */
[[nodiscard]]
extern hdag_res hdag_file_from_bundle(struct hdag_file *pfile,
                                      const char *pathname,
                                      int template_sfxlen,
                                      mode_t open_mode,
                                      const struct hdag_bundle *bundle);

/**
 * Create a bundle from the contents of a file.
 *
 * @param pbundle           The location for the output bundle.
 *                          Not modified in case of failure.
 *                          Can be NULL to have bundle discarded.
 * @param file              The opened file to create the bundle from.
 *
 * @return A void universal result.
 */
[[nodiscard]]
extern hdag_res hdag_file_to_bundle(struct hdag_bundle *pbundle,
                                    const struct hdag_file *file);

/**
 * Create and open a hash DAG file with specified parameters and a node
 * sequence (adjacency list).
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
 * @param node_seq          The sequence of nodes (and optionally their
 *                          targets, constituting an adjacency list) to store
 *                          in the created file. Specifies the node hash
 *                          length.
 *
 * @return A void universal result.
 */
[[nodiscard]]
extern hdag_res hdag_file_from_node_seq(struct hdag_file *pfile,
                                        const char *pathname,
                                        int template_sfxlen,
                                        mode_t open_mode,
                                        struct hdag_node_seq *node_seq);

/**
 * Create and open a hash DAG file with specified parameters and a text
 * adjacency list file stream.
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
 * @param stream            The FILE stream containing the text to parse and
 *                          load. Each line of the stream is expected to
 *                          contain a node's hash followed by hashes of its
 *                          targets, if any. Each hash is represented by a
 *                          hexadecimal number, separated by (non-linebreak)
 *                          whitespace. Hashes are assumed to be
 *                          right-aligned.
 * @param hash_len          The length of hashes expected to be contained in
 *                          the stream. Must be a valid hash length.
 *
 * @return A void universal result.
 */
[[nodiscard]]
extern hdag_res hdag_file_from_txt(struct hdag_file *pfile,
                                   const char *pathname,
                                   int template_sfxlen,
                                   mode_t open_mode,
                                   FILE *stream,
                                   uint16_t hash_len);

/**
 * Write the contents of a file to an adjacency list text stream.
 *
 * @param stream            The FILE stream to output to.
 * @param file              The file to output.
 *
 * @return A void universal result.
 */
[[nodiscard]]
extern hdag_res hdag_file_to_txt(FILE *stream, const struct hdag_file *file);

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

/**
 * Lookup the index of a node within a file, using its hash.
 *
 * @param file      The bundle to look up the node in.
 * @param hash_ptr  The hash the node must have.
 *                  The hash length must match the file's hash length.
 *
 * @return The index of the found node (< INT32_MAX),
 *         or INT32_MAX, if not found.
 */
static inline uint32_t
hdag_file_find_node_idx(const struct hdag_file *file,
                        const uint8_t *hash_ptr)
{
    assert(hdag_file_is_valid(file));
    assert(hdag_file_is_open(file));
    assert(hash_ptr != NULL);

    const uint32_t *fanout = file->header->node_fanout;

    return hdag_nodes_slice_find(
        file->nodes,
        (*hash_ptr == 0 ? 0 : fanout[*hash_ptr - 1]),
        fanout[*hash_ptr],
        file->header->hash_len,
        hash_ptr
    );
}

#endif /* _HDAG_FILE_H */
