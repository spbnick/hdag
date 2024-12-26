/**
 * Hash DAG database - a collection of hash DAG files
 */

#include <hdag/db.h>
#include <hdag/hashes.h>
#include <sys/stat.h>
#include <limits.h>

const char *HDAG_DB_NEW_FILE_PATHNAME_TMPL_FMT = "%s/XXXXXX.hdag.new";
const size_t HDAG_DB_NEW_FILE_PATHNAME_TMPL_SFX_LEN = 9;
const size_t HDAG_DB_NEW_FILE_PATHNAME_EXT_SFX_LEN = 4;

hdag_res
hdag_db_create(struct hdag_db *pdb, const char *pathname, bool template,
               mode_t mode, uint16_t hash_len)
{
    hdag_res res = HDAG_RES_INVALID;
    int orig_errno;
    struct hdag_db db = HDAG_DB_CLOSED;

    if (pathname != NULL) {
        db.pathname = strdup(pathname);
        if (template) {
            if (mkdtemp(db.pathname) == NULL) {
                goto cleanup;
            }
            if (chmod(db.pathname, mode) != 0) {
                goto cleanup;
            }
        } else {
            if (mkdir(db.pathname, mode) != 0) {
                goto cleanup;
            }
        }
        #define SNPRINTF_NEW_FILE_PATHNAME_TMPL(_buf, _len) \
            snprintf(_buf, _len, HDAG_DB_NEW_FILE_PATHNAME_TMPL_FMT, \
                     db.pathname)
        const size_t new_file_pathname_tmpl_len =
            SNPRINTF_NEW_FILE_PATHNAME_TMPL(NULL, 0) + 1;
        /* Generate filename template */
        db.new_file_pathname_tmpl = calloc(1, new_file_pathname_tmpl_len);
        if (db.new_file_pathname_tmpl == NULL) {
            goto cleanup;
        }
        SNPRINTF_NEW_FILE_PATHNAME_TMPL(db.new_file_pathname_tmpl,
                                   new_file_pathname_tmpl_len);
        #undef SNPRINTF_NEW_FILE_PATHNAME_TMPL
    }

    db.hash_len = hash_len;

    if (pdb != NULL) {
        *pdb = db;
        db = HDAG_DB_CLOSED;
    }

    res = HDAG_RES_OK;
cleanup:
    orig_errno = errno;
    hdag_db_close(&db);
    errno = orig_errno;
    return HDAG_RES_ERRNO_IF_INVALID(res);
}

const struct hdag_ctx_node *
hdag_db_ctx_get_node(struct hdag_ctx *base_ctx, const uint8_t *hash)
{
    struct hdag_db_ctx *ctx = HDAG_CONTAINER_OF(
        struct hdag_db_ctx, base, base_ctx
    );
    size_t idx_idx;
    uint32_t node_idx;
    const struct hdag_bundle *bundle;

    for (idx_idx = 0; idx_idx < ctx->bundle_idx_num; idx_idx++) {
        bundle = hdag_darr_element_const(
            &ctx->db->bundles, ctx->bundle_idx_list[idx_idx]
        );
        node_idx = hdag_bundle_find_node_idx(bundle, hash);
        if (node_idx < INT32_MAX) {
            const struct hdag_node *node = HDAG_BUNDLE_NODE(bundle, node_idx);
            ctx->node = (struct hdag_ctx_node){
                .hash = node->hash,
                .generation = node->generation,
                .target_hash_seq = hdag_bundle_targets_hash_seq_init(
                    &ctx->target_hash_seq, bundle, node_idx
                ),

            };
            return &ctx->node;
        }
    }
    return NULL;
}

struct hdag_ctx *
hdag_db_ctx_init(struct hdag_db_ctx *pctx, const struct hdag_db *db,
                 const size_t *bundle_idx_list, size_t bundle_idx_num)
{
    assert(pctx != NULL);
    assert(hdag_db_is_valid(db));
    assert(hdag_db_is_open(db));
    assert(bundle_idx_list != NULL || bundle_idx_num == 0);

#ifndef NDEBUG
    size_t idx_idx;
    size_t prev_idx;
    size_t idx;
    size_t bundle_num = hdag_darr_occupied_slots(&db->bundles);

    for (idx_idx = 0; idx_idx < bundle_idx_num; prev_idx = idx, idx_idx++) {
        idx = bundle_idx_list[idx_idx];
        assert(idx_idx == 0 || idx > prev_idx);
        assert(idx < bundle_num);
    }
#endif

    *pctx = (struct hdag_db_ctx){
        .base = {
            .hash_len = db->hash_len,
            .get_node_fn = hdag_db_ctx_get_node,
        },
        .db = db,
        .bundle_idx_list = bundle_idx_list,
        .bundle_idx_num = bundle_idx_num,
    };

    assert(hdag_ctx_is_valid(&pctx->base));
    return &pctx->base;
}


struct hdag_db_components_pseudohash {
    uint64_t    hash;
    uint32_t    bundle_idx;
    uint32_t    component_idx;
};

HDAG_ASSERT_STRUCT_MEMBERS_PACKED(
    hdag_db_components_pseudohash,
    hash,
    bundle_idx,
    component_idx
);

struct hdag_db_components_targets_hash_seq {
    /** The base abstract hash sequence */
    struct hdag_hash_seq                    base;
    /** The pseudohash of the target returned last */
    struct hdag_db_components_pseudohash    pseudohash;
};

struct hdag_db_components_node_seq {
    /** The base abstract node sequence */
    struct hdag_node_seq                        base;
    /** The array of pointers to bundles to build the component DAG from */
    struct hdag_bundle                        **pbundle_list;
    /** The number of pointers to bundles to build the component DAG from */
    size_t                                      pbundle_num;
    /** The index of the next bundle to extract component pieces from */
    size_t                                      pbundle_idx;
    /** The hash of the node returned last */
    struct hdag_db_components_pseudohash        pseudohash;
    /** The sequence of the returned node's target hashes */
    struct hdag_db_components_targets_hash_seq  targets_hash_seq;
};

static hdag_res
hdag_db_components_node_seq_next(struct hdag_node_seq *node_seq,
                                 const uint8_t **phash,
                                 struct hdag_hash_seq **ptarget_hash_seq)
{
}

static hdag_res
hdag_db_components_node_seq_init(struct hdag_db_components_node_seq *pseq,
                                 struct hdag_bundle **pbundle_list,
                                 size_t pbundle_num)
{
    assert(pseq != NULL);
    assert(pbundle_list != NULL || pbundle_num == 0);

    *pseq = (struct hdag_db_components_node_seq){
        .base = {
            .hash_len = sizeof(pseq->pseudohash),
            .next_fn = hdag_db_components_node_seq_next,
        },
        .pbundle_list = pbundle_list,
        .pbundle_num = pbundle_num,
    };

    assert(hdag_node_seq_is_valid(&pseq->base));
    return &pseq->base;
}

/**
 * Build a component connection DAG from a list of bundles.
 *
 * @param pcomponents   The location for the created component DAG bundle.
 * @param pbundle_list  The array of pointers to bundles to build the
 *                      component DAG from.
 * @param pbundle_num   The number of pointers to bundles to build the
 *                      component DAG from.
 */
[[nodiscard]]
static hdag_res
hdag_db_components_collect(struct hdag_bundle *pcomponents,
                           struct hdag_bundle **pbundle_list,
                           size_t pbundle_num)
{
    hdag_res res = HDAG_RES_INVALID;
}

hdag_res
hdag_db_merge_node_seq(struct hdag_db *db, struct hdag_node_seq *node_seq)
{
    assert(hdag_db_is_valid(db));
    assert(hdag_node_seq_is_valid(node_seq));
    assert(node_seq->hash_len == db->hash_len);

    hdag_res res = HDAG_RES_INVALID;
    /* The bundle receiving the node sequence */
    struct hdag_bundle new_bundle = HDAG_BUNDLE_EMPTY(db->hash_len);
    /* Index of a node inside the new bundle */
    ssize_t new_node_idx;
    /* A node inside the new bundle */
    struct hdag_node *new_node;

    /* Number of existing bundles */
    size_t bundle_num;
    /* The array of existing bundle indexes (bundle_num long) */
    size_t *bundle_idx_list = NULL;
    /* Index of an index of an existing bundle */
    size_t bundle_idx_idx;
    /* Index of an existing bundle */
    ssize_t bundle_idx;
    /* An existing bundle */
    struct hdag_bundle *bundle;
    /* Index of a node inside an existing bundle */
    uint32_t node_idx;
    /* A node inside an existing bundle */
    const struct hdag_node *node;

    /*
     * Number of indexes of bundles to rebuild
     * at the front of bundle_idx_list
     */
    size_t rebuilt_bundle_num;
    /* Original rebuilt_bundle_num value */
    size_t orig_rebuilt_bundle_num;
    /* Index of a bundle to be rebuilt */
    ssize_t rebuilt_bundle_idx;
    /* Index of a rebuilt bundle index */
    size_t rebuilt_bundle_idx_idx;
    /* A bundle to be rebuilt */
    const struct hdag_bundle *rebuilt_bundle;

    /* An array of node sequences of the new and all the rebuilt bundles */
    struct hdag_bundle_node_seq *node_seq_list = NULL;

    /* An array of pointers to the (abstract) node sequences */
    struct hdag_node_seq **pnode_seq_list = NULL;

    /* The bundle containing merged new and rebuilt bundles */
    struct hdag_bundle merged_bundle = HDAG_BUNDLE_EMPTY(db->hash_len);

    /* The final name of the merged bundle's file */
    char *merged_pathname = NULL;

    /*
     * Create a bundle from the node sequence, without context
     */
    HDAG_RES_TRY(
        hdag_bundle_organized_from_node_seq(&new_bundle, NULL, node_seq)
    );

    /* TODO: Lock the database for writing */

    /*
     * Create an array of indexes of existing bundles.
     */
    bundle_num = db->bundles.slots_occupied;
    if (bundle_num != 0) {
        bundle_idx_list = malloc(sizeof(*bundle_idx_list) * bundle_num);
        if (bundle_idx_list == NULL) {
            goto cleanup;
        }
        for (bundle_idx_idx = 0;
             bundle_idx_idx < bundle_num;
             bundle_idx_idx++) {
            bundle_idx_list[bundle_idx_idx] = bundle_idx_idx;
        }
    }

    /*
     * Eliminate nodes already "known" by the DB from the new bundle,
     * and find directly-linked bundles to rebuild.
     */
    /* For each known node in the bundle */
    HDAG_DARR_ITER_FORWARD(&new_bundle.nodes, new_node_idx, new_node,
                           (void)0, (void)0) {
        if (!hdag_node_is_known(new_node)) {
            continue;
        }
        orig_rebuilt_bundle_num = rebuilt_bundle_num;
        /* For each index of an existing bundle */
        for (bundle_idx_idx = 0;
             bundle_idx_idx < bundle_num;
             bundle_idx_idx++) {
            bundle_idx = bundle_idx_list[bundle_idx_idx];
            bundle = hdag_darr_element(&db->bundles, bundle_idx);
            node_idx = hdag_bundle_find_node_idx(bundle, new_node->hash);
            /* If not found */
            if (node_idx >= INT_MAX) {
                continue;
            }
            node = HDAG_BUNDLE_NODE(bundle, node_idx);
            /* If the existing bundle's node is KNOWN */
            if (hdag_node_is_known(node)) {
                /* If node contents mismatch */
                if (hdag_hash_seq_cmp(
                    &HDAG_BUNDLE_TARGETS_HASH_SEQ(&new_bundle, new_node_idx),
                    &HDAG_BUNDLE_TARGETS_HASH_SEQ(bundle, node_idx)
                )) {
                    res = HDAG_RES_NODE_CONFLICT;
                    goto cleanup;
                }
                /* Forget the node in the new bundle (i.e. remove it) */
                /* NOTE: we skip updating unknown_hashes */
                new_node->targets = HDAG_TARGETS_UNKNOWN;
                /* Drop all files to be rebuilt for this new node */
                rebuilt_bundle_num = orig_rebuilt_bundle_num;
                /* Continue with the next node */
                break;
            /*
             * Else the existing bundle's node is UNKNOWN,
             * the new bundle is defining it, and so the existing bundle
             * should have its generations re-enumerated.
             */
            } else {
                /* Make sure the current index is below rebuilt_bundle_num */
                if (bundle_idx_idx == rebuilt_bundle_num) {
                    rebuilt_bundle_num++;
                } else if (bundle_idx_idx > rebuilt_bundle_num) {
                    bundle_idx_list[bundle_idx_idx] =
                        bundle_idx_list[rebuilt_bundle_num];
                    bundle_idx_list[rebuilt_bundle_num] = bundle_idx;
                    rebuilt_bundle_num++;
                }
            }
        }
    }

    /*
     * Collect all downstream (child) bundles to rebuild
     */
    /* Until no new bundles to rebuild are found */
    do {
        orig_rebuilt_bundle_num = rebuilt_bundle_num;
        /* For each non-rebuilt bundle index */
        for (bundle_idx_idx = rebuilt_bundle_num;
             bundle_idx_idx < bundle_num;
             bundle_idx_idx++) {
            bundle_idx = bundle_idx_list[bundle_idx_idx];
            bundle = hdag_darr_element(&db->bundles, bundle_idx);
            /* For each index of a bundle to be rebuilt */
            for (rebuilt_bundle_idx_idx = 0;
                 rebuilt_bundle_idx_idx < rebuilt_bundle_num;
                 rebuilt_bundle_idx_idx++) {
                rebuilt_bundle_idx = bundle_idx_list[rebuilt_bundle_idx_idx];
                rebuilt_bundle = hdag_darr_element(
                    &db->bundles, rebuilt_bundle_idx
                );
                /*
                 * If any hashes unknown to the non-rebuilt bundle are found
                 * in the bundle to rebuild.
                 */
                if (HDAG_RES_TRY(hdag_hash_seq_are_intersecting(
                    &HDAG_HASHES_DARR_SEQ(&bundle->unknown_hashes),
                    &HDAG_BUNDLE_KNOWN_NODE_HASH_SEQ(rebuilt_bundle)
                ))) {
                    /* Make sure the index is below rebuilt_bundle_num */
                    if (bundle_idx_idx == rebuilt_bundle_num) {
                        rebuilt_bundle_num++;
                    } else if (bundle_idx_idx > rebuilt_bundle_num) {
                        bundle_idx_list[bundle_idx_idx] =
                            bundle_idx_list[rebuilt_bundle_num];
                        bundle_idx_list[rebuilt_bundle_num] = bundle_idx;
                        rebuilt_bundle_num++;
                    }
                    /* Continue with next non-rebuilt bundle */
                    break;
                }
            }
        }
    } while (rebuilt_bundle_num != orig_rebuilt_bundle_num);

    /* Reverse-sort the indexes of rebuilt bundles to speed up removal */
    qsort(bundle_idx_list, rebuilt_bundle_num, sizeof(size_t),
          hdag_size_t_rcmp);

    /*
     * Merge the new bundle and the bundles to be rebuilt into one bundle
     */

    /* Allocate and initialize the node sequence and their pointer lists */
    node_seq_list = malloc((rebuilt_bundle_num + 1) *
                           sizeof(*node_seq_list));
    if (node_seq_list == NULL) {
        goto cleanup;
    }
    pnode_seq_list = malloc((rebuilt_bundle_num + 1) *
                            sizeof(*pnode_seq_list));
    if (pnode_seq_list == NULL) {
        goto cleanup;
    }

    pnode_seq_list[0] = hdag_bundle_node_seq_init(&node_seq_list[0],
                                                  &new_bundle,
                                                  /* Without unknown nodes */
                                                  false);
    for (bundle_idx_idx = 0;
         bundle_idx_idx < rebuilt_bundle_num;
         bundle_idx_idx++) {
        pnode_seq_list[bundle_idx_idx + 1] = hdag_bundle_node_seq_init(
            &node_seq_list[bundle_idx_idx + 1],
            hdag_darr_element(&db->bundles, bundle_idx_list[bundle_idx_idx]),
            /* Without unknown nodes */
            false
        );
    }

    /* Create a bundle from the concatenated node sequence, *with* context */
    HDAG_RES_TRY(hdag_bundle_organized_from_node_seq(
        &merged_bundle,
        /* TODO ADD CONTEXT! */
        /* The context should exclude the rebuilt bundles */
        NULL,
        &HDAG_NODE_SEQ_CAT(db->hash_len,
                           pnode_seq_list,
                           rebuilt_bundle_num + 1)
    ));

    /* Free the used-up sequence (pointer) list */
    free(pnode_seq_list);
    pnode_seq_list = NULL;
    free(node_seq_list);
    node_seq_list = NULL;
    /* Cleanup the now-unneded new bundle */
    hdag_bundle_cleanup(&new_bundle);

    /* File the merged bundle under a name with the "new" extension */
    HDAG_RES_TRY(hdag_bundle_file(&merged_bundle, db->new_file_pathname_tmpl,
                                  HDAG_DB_NEW_FILE_PATHNAME_TMPL_SFX_LEN,
                                  S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP));

    /*
     * Replace the rebuilt bundles with the merged bundle
     */
    /* TODO: Lock the database for reading */

    /* For each rebuilt bundle index from last to first */
    for (bundle_idx_idx = 0;
         bundle_idx_idx < rebuilt_bundle_num; bundle_idx_idx++) {
        bundle_idx = bundle_idx_list[bundle_idx_idx];
        bundle = hdag_darr_element(&db->bundles, bundle_idx);
        /* Remove the bundle's file */
        HDAG_RES_TRY(hdag_bundle_unlink(bundle));
        hdag_bundle_cleanup(bundle);
        hdag_darr_remove_one(&db->bundles, bundle_idx);
    }
    /* Remove the "new" extension from the now absorbed merged bundle file */
    /* FIXME: Renaming a temporary file is unsafe, create a new one instead */
    assert(strlen(merged_bundle.file.pathname) <
           HDAG_DB_NEW_FILE_PATHNAME_EXT_SFX_LEN);
    merged_pathname = strndup(merged_bundle.file.pathname,
                              strlen(merged_bundle.file.pathname) -
                              HDAG_DB_NEW_FILE_PATHNAME_EXT_SFX_LEN);
    if (merged_pathname == NULL) {
        goto cleanup;
    }
    HDAG_RES_TRY(hdag_bundle_rename(&merged_bundle, merged_pathname));
    free(merged_pathname);
    merged_pathname = NULL;
    /* Move the merged bundle into the bundle array */
    if (hdag_darr_append_one(&db->bundles, &merged_bundle) == NULL) {
        goto cleanup;
    }
    merged_bundle = HDAG_BUNDLE_EMPTY(db->hash_len);

    res = HDAG_RES_OK;
cleanup:
    /* TODO: Unlock the database for reading */
    /* TODO: Unlock the database for writing */

    free(merged_pathname);
    hdag_bundle_cleanup(&merged_bundle);
    free(pnode_seq_list);
    free(node_seq_list);
    free(bundle_idx_list);
    hdag_bundle_cleanup(&new_bundle);
    return HDAG_RES_ERRNO_IF_INVALID(res);
}

hdag_res
hdag_db_open(struct hdag_db *pdb, const char *pathname)
{
    struct hdag_db db = HDAG_DB_CLOSED;

    assert(pathname == NULL && "Filesystem backing not supported yet");
    assert(false && "Not implemented");

    if (pdb != NULL) {
        *pdb = db;
        db = HDAG_DB_CLOSED;
    }

    hdag_db_close(&db);
    return HDAG_RES_OK;
}

hdag_res
hdag_db_close(struct hdag_db *db)
{
    ssize_t idx;
    struct hdag_bundle *bundle;

    assert(hdag_db_is_valid(db));

    HDAG_DARR_ITER_FORWARD(&db->bundles, idx, bundle, (void)0, (void)0) {
        hdag_bundle_cleanup(bundle);
    }
    hdag_darr_cleanup(&db->bundles);
    if (db->pathname != NULL) {
        free(db->pathname);
        db->pathname = NULL;
    }
    if (db->new_file_pathname_tmpl != NULL) {
        free(db->new_file_pathname_tmpl);
        db->new_file_pathname_tmpl = NULL;
    }
    return HDAG_RES_OK;
}
