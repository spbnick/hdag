/**
 * Hash DAG database - a collection of hash DAG files
 */

#ifndef _HDAG_DB_H
#define _HDAG_DB_H

#include <hdag/file.h>
#include <hdag/bundle.h>
#include <hdag/ctx.h>
#include <hdag/res.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

/**
 * The format string for the pathname template for a new database file.
 * Accepts the database directory path as the single format argument. The
 * resulting template has a suffix with length specified in
 * HDAG_DB_NEW_FILE_PATHNAME_TMPL_SFX_LEN.
 */
extern const char *HDAG_DB_NEW_FILE_PATHNAME_TMPL_FMT;

/** The length of the suffix of the new file's pathname template */
extern const size_t HDAG_DB_NEW_FILE_PATHNAME_TMPL_SFX_LEN;

/**
 * The length of the "extension" on the end of a file pathname, signifying
 * it's a new one, and shouldn't be considered a part of the database yet.
 * This suffix is removed to add the file to the database.
 */
extern const size_t HDAG_DB_NEW_FILE_PATHNAME_EXT_SFX_LEN;

/** An HDAG database */
struct hdag_db {
    /** The pathname of the database directory. NULL if it's in-memory. */
    char *pathname;
    /**
     * The pathname template for new bundle files, if filesystem-backed. Has
     * HDAG_DB_NEW_FILE_PATHNAME_TMPL_SFX_LEN-long suffix on the end. NULL, if
     * the database is in-memory (pathname == NULL).
     */
    char *new_file_pathname_tmpl;
    /** The length of the node hash used throughout the database */
    uint16_t hash_len;
    /** The dynamic array of hash DAG bundles */
    struct hdag_darr bundles;

    /**
     * An HDAG representing component connections across bundle boundaries.
     * Each node is a piece of a component that is spread over more than one
     * bundle. Edge directions represent the connecting edge directions.
     * The node hash is a 64-bit "hash" generated from the following 32 bits
     * of bundle index, followed by 32 bits of target node index, giving a
     * total of 128 bits (16 bytes).
     */
    struct hdag_bundle components;
};

/**
 * The structure of the (pseudo)hash of a "components" node (component piece).
 */
struct hdag_db_components_pseudohash {
    /** A SplitMix64 hash over (bundle_idx << 32) | node_idx */
    uint64_t    hash;
    /** The index of the bundle the component piece belongs to */
    uint32_t    bundle_idx;
    /** The index of the node component connects to, in the bundle */
    uint32_t    node_idx;
};

HDAG_ASSERT_STRUCT_MEMBERS_PACKED(
    hdag_db_components_pseudohash,
    hash,
    bundle_idx,
    node_idx
);

/** An initializer for a component piece pseudohash */
#define HDAG_DB_COMPONENTS_PSEUDOHASH(_bundle_idx, _node_idx) \
    (struct hdag_db_components_pseudohash) {                            \
        .hash = hdag_splitmix64_hash((((uint64_t)_bundle_idx) << 32) |  \
                                     (_node_idx)),                      \
        .bundle_idx = bundle_idx,                                       \
        .node_idx = node_idx,                                           \
    }

/**
 * Initialize a component piece pseudohash
 *
 * @param pseudohash    The pseudohash to initialize.
 * @param bundle_idx    The index of the bundle of the component piece.
 * @param node_idx      The index of the component source/target node in the
 *                      bundle.
 *
 * @return The initialized pseudohash.
 */
static inline struct hdag_db_components_pseudohash *
hdag_db_components_pseudohash_init(
                        struct hdag_db_components_pseudohash *pseudohash,
                        uint32_t bundle_idx, uint32_t node_idx)
{
    assert(pseudohash != NULL);
    *pseudohash = HDAG_DB_COMPONENTS_PSEUDOHASH(_bundle_idx, _node_idx);
    return pseudohash;
}

/** An initializer for a component piece pseudohash */
#define HDAG_DB_COMPONENTS_PSEUDOHASH(_bundle_idx, _node_idx) \
    (*hdag_db_components_pseudohash_init(
        &(struct

/**
 * An initializer for a closed HDAG database
 */
#define HDAG_DB_CLOSED ((struct hdag_db){ \
    .bundles = HDAG_DARR_EMPTY(sizeof(struct hdag_bundle), 16), \
    .components = HDAG_BUNDLE_EMPTY(                            \
        sizeof(struct hdag_db_components_pseudohash)            \
    ),                                                          \
})

/**
 * Check if a database is valid.
 *
 * @param db    The database to check.
 *
 * @return True if the database is valid, false otherwise.
 */
static inline bool
hdag_db_is_valid(const struct hdag_db *db)
{
    ssize_t idx;
    const struct hdag_bundle *bundle;

    if (!(
        db != NULL &&
        (db->hash_len == 0 || hdag_hash_len_is_valid(db->hash_len)) &&
        (db->pathname == NULL) == (db->new_file_pathname_tmpl == NULL) &&
        hdag_darr_is_valid(&db->bundles) &&
        hdag_bundle_is_valid(&db->components) &&
        db->components.hash_len ==
            sizeof(struct hdag_db_components_pseudohash) &&
        hdag_bundle_is_organized(&db->components)
    )) {
        return false;
    }

    HDAG_DARR_ITER_FORWARD(&db->bundles, idx, bundle, (void)0, (void)0) {
        if (!hdag_bundle_is_valid(bundle) ||
            !hdag_bundle_is_filed(bundle)) {
            return false;
        }
    }

    return true;
}

/**
 * Check if a database is open.
 *
 * @param db    The database to check.
 *
 * @return True if the database is open, false otherwise.
 */
static inline bool
hdag_db_is_open(const struct hdag_db *db)
{
    assert(hdag_db_is_valid(db));
    return db->hash_len != 0;
}

/**
 * Create and open an HDAG database.
 *
 * @param pdb       Location to store the state of the created database.
 *                  Can be NULL to have the database closed after creation.
 *                  Not modified in case of failure.
 * @param pathname  The path to the database directory, which should not exist.
 *                  Or NULL to create in-memory database, not backed by a
 *                  filesystem.
 * @param template  True if the pathname should be considered the template for
 *                  a randomly-generated name of the database directory,
 *                  ending with "XXXXXX", to be replaced with random
 *                  characters. False if the pathname should be treated
 *                  literally. Ignored if pathname is NULL.
 * @param mode      The mode bitmap to supply to mkdir(2)/chmod(2) to set the
 *                  created directory's permissions.
 *                  Ignored, if pathname is NULL.
 * @param hash_len  The length of the node hash to use and expect.
 *
 * @return A void universal result.
 */
[[nodiscard]]
extern hdag_res hdag_db_create(struct hdag_db *pdb,
                               const char *pathname,
                               bool template,
                               mode_t mode,
                               uint16_t hash_len);

/**
 * Open an existing HDAG database.
 *
 * @param pdb       Location to store the state of the opened database.
 *                  Can be NULL to have the database closed after opening.
 *                  Not modified in case of failure.
 * @param pathname  The path to the database directory.
 *
 * @return A void universal result.
 */
[[nodiscard]]
extern hdag_res hdag_db_open(struct hdag_db *pdb, const char *pathname);

/** An HDAG context of (a subset of) a database */
struct hdag_db_ctx {
    /** The base abstract HDAG context */
    struct hdag_ctx                         base;
    /** The database of the context */
    const struct hdag_db                   *db;
    /** The list of bundle indexes to include into the context */
    const size_t                           *bundle_idx_list;
    /** The number of bundle indexes to include into the context */
    size_t                                  bundle_idx_num;
    /** A returned node */
    struct hdag_ctx_node                    node;
    /** A returned node's target hash sequence */
    struct hdag_bundle_targets_hash_seq     target_hash_seq;
};

/** Node retrieval function for a database context */
extern const struct hdag_ctx_node *hdag_db_ctx_get_node(
                                            struct hdag_ctx *base_ctx,
                                            const uint8_t *hash);

/**
 * Initialize an HDAG context corresponding to (a subset of) a database.
 *
 * @param pctx              The database context to initialize.
 * @param db                The database to base the context on. Must be open.
 * @param bundle_idx_list   An array of indexes of the database's bundles 
 *                          to include into the context.
 *                          Must be sorted and contain no duplicates.
 * @param bundle_idx_num    The number of bundle indexes in the array above.
 *
 * @return The pointer to the abstract base context.
 */
extern struct hdag_ctx *hdag_db_ctx_init(struct hdag_db_ctx *pctx,
                                         const struct hdag_db *db,
                                         const size_t *bundle_idx_list,
                                         size_t bundle_idx_num);

/**
 * Merge a node sequence (adjacency list) into a database.
 *
 * @param db        The database to add the bundle into.
 * @param node_seq  The sequence of nodes (and optionally their targets)
 *                  to merge into the database.
 *
 * @return A void universal result.
 */
[[nodiscard]]
extern hdag_res hdag_db_merge_node_seq(struct hdag_db *db,
                                       struct hdag_node_seq *node_seq);

/**
 * Close a database.
 *
 * @param pdf   Location of the opened database to close.
 *
 * @return A void universal result.
 */
[[nodiscard]]
extern hdag_res hdag_db_close(struct hdag_db *pdb);

#endif /* _HDAG_DB_H */
