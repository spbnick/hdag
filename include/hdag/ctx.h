/*
 * Hash DAG context.
 */

#ifndef _HDAG_CTX_H
#define _HDAG_CTX_H

#include <hdag/hash_seq.h>

/* The forward declaration of a context */
struct hdag_ctx;

/** The result of matching a node in a context */
enum hdag_ctx_node_matched {
    /** Node not found */
    HDAG_CTX_NODE_MATCHED_NONE,
    /** Matched a node with the same hash */
    HDAG_CTX_NODE_MATCHED_HASH,
    /**
     * Matched a node fully, hash and targets.
     * Can be returned only if no hash-only matches are encountered.
     */
    HDAG_CTX_NODE_MATCHED_FULL,
};

/**
 * The prototype for a function a node in a context.
 *
 * @param ctx               The context to match the node in.
 * @param hash              The hash of the node to match.
 * @param target_hash_seq   The (resettable) sequence of the node's target
 *                          hashes to match, or NULL to not match targets.
 *
 * @return The result matching code.
 */
typedef enum hdag_ctx_node_matched (*hdag_ctx_node_match_fn)(
                            const struct hdag_ctx *ctx,
                            const uint8_t *hash,
                            const struct hdag_hash_seq *target_hash_seq);

/** A hash DAG context */
struct hdag_ctx {
    /** The length of the DAG hashes, bytes */
    uint16_t                hash_len;
    /** The function finding a node in the context */
    hdag_ctx_node_match_fn  node_match_fn;
    /**
     * True, if added nodes with the same hash should have the same content.
     * False, if node content should not be verified.
     */
    bool                    match_content;
    /**
     * True, if any added duplicate nodes/edges should be rejected with an
     * HDAG_RES_NODE_DUPLICATE/HDAG_RES_EDGE_DUPLICATE.
     * False, if they should be silently dropped.
     */
    bool                    reject_dups;
    /** The context's private data */
    void                   *data;
};

/**
 * Check if a context is valid.
 *
 * @param ctx   The context to check.
 *
 * @return True if the context is valid, false if not.
 */
static inline bool
hdag_ctx_is_valid(const struct hdag_ctx *ctx)
{
    return ctx != NULL &&
        hdag_hash_len_is_valid(ctx->hash_len) &&
        ctx->node_match_fn != NULL;
}

/**
 * Check if a node with specified hash and target hashes can be added to a
 * hash DAG context.
 *
 * @param ctx               The context to validate addition to.
 * @param hash              The hash of the node being added.
 * @param target_hash_seq   The (resettable) sequence of the node's target
 *                          hashes.
 *
 * @return A universal result, which is (positive) true if the node can be
 *         added to the context, false if the node should not be added (should
 *         be dropped), or a failure result, including:
 *         * HDAG_RES_NODE_CONFLICT, if the context's "match_content" is true,
 *           and nodes with matching hashes had different targets.
 *         * HDAG_RES_NODE_DUPLICATE, if the context's "reject_dups" is true,
 *           and a duplicate node was detected (according to "match_content").
 */
static inline hdag_res
hdag_ctx_can_add_node(const struct hdag_ctx *ctx,
                      const uint8_t *hash,
                      const struct hdag_hash_seq *target_hash_seq)
{
    assert(hdag_ctx_is_valid(ctx));
    assert(hash != NULL);
    assert(hdag_hash_seq_is_valid(target_hash_seq));
    assert(hdag_hash_seq_is_resettable(target_hash_seq));

    switch (ctx->node_match_fn(ctx, hash,
                               ctx->match_content ? target_hash_seq : NULL)) {
    case HDAG_CTX_NODE_MATCHED_HASH:
        if (ctx->match_content) {
            return HDAG_RES_NODE_CONFLICT;
        }
        /* Fall through */
    case HDAG_CTX_NODE_MATCHED_FULL:
        if (ctx->reject_dups) {
            return HDAG_RES_NODE_DUPLICATE;
        }
        /* Fall through */
    default:
        return true;
    }
}

/** Node finding function which never matches anything */
extern enum hdag_ctx_node_matched hdag_ctx_empty_node_match_fn(
                            const struct hdag_ctx *ctx,
                            const uint8_t *hash,
                            const struct hdag_hash_seq *target_hash_seq);

/**
 * An initializer for an empty hash DAG context
 *
 * @param _hash_len The length of the hashes in the context
 */
#define HDAG_CTX_EMPTY(_hash_len) (struct hdag_ctx){ \
    .hash_len = hdag_hash_len_validate(_hash_len),      \
    .node_match_fn = hdag_ctx_empty_node_match_fn,      \
}

#endif /* _HDAG_CTX_H */
