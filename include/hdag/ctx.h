/**
 * Hash DAG context.
 */

#ifndef _HDAG_CTX_H
#define _HDAG_CTX_H

#include <hdag/hash_seq.h>

/* The forward declaration of a context */
struct hdag_ctx;

/**
 * A node retrieved from a context.
 * Valid only until the next get_node_fn()
 */
struct hdag_ctx_node {
    /** The node hash with the length specified by the context */
    const uint8_t          *hash;
    /** The node's generation number */
    uint32_t                generation;
    /**
     * The (resettable) sequence of the node's target hashes (ordered
     * lexicographically), or NULL, if the node is "unknown".
     */
    struct hdag_hash_seq   *target_hash_seq;
};

/**
 * The prototype for a function retrieving a node from a context.
 * Non-reentrant.
 *
 * @param ctx   The context to retrieve the node from.
 * @param hash  The hash of the node to retrieve.
 *
 * @return The pointer to a node description allocated within the context,
 *         if found. NULL if the node is not found in the context.
 */
typedef const struct hdag_ctx_node * (*hdag_ctx_get_node_fn)(
                                            struct hdag_ctx *ctx,
                                            const uint8_t *hash);

/** An abstract hash DAG context (a supergraph) */
struct hdag_ctx {
    /** The length of the DAG hashes, bytes */
    uint16_t                hash_len;
    /** The function finding a node in the context */
    hdag_ctx_get_node_fn    get_node_fn;
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
        ctx->get_node_fn != NULL;
}

/**
 * Retrieve the description of a node from a context. Non-reentrant.
 *
 * @param ctx   The context to retrieve the node from.
 * @param hash  The hash of the node to retrieve.
 *
 * @return The pointer to a node description allocated within the context,
 *         if found. NULL if the node is not found in the context.
 */
static inline const struct hdag_ctx_node *
hdag_ctx_get_node(struct hdag_ctx *ctx, const uint8_t *hash)
{
    assert(hdag_ctx_is_valid(ctx));
    assert(hash != NULL);
    return ctx->get_node_fn(ctx, hash);
}

/** Node retrieval function for an empty context, never returning nodes */
extern const struct hdag_ctx_node *hdag_ctx_empty_get_node(
                                            struct hdag_ctx *ctx,
                                            const uint8_t *hash);

/**
 * An initializer for an empty hash DAG context
 *
 * @param _hash_len The length of the hashes in the context
 */
#define HDAG_CTX_EMPTY(_hash_len) (struct hdag_ctx){ \
    .hash_len = hdag_hash_len_validate(_hash_len),      \
    .get_node_fn = hdag_ctx_empty_get_node,             \
}

#endif /* _HDAG_CTX_H */
