/*
 * Hash DAG context.
 */

#include <hdag/ctx.h>

enum hdag_ctx_node_matched
hdag_ctx_empty_node_match_fn(const struct hdag_ctx *ctx,
                             const uint8_t *hash,
                             const struct hdag_hash_seq *target_hash_seq)
{
    (void)ctx;
    (void)hash;
    (void)target_hash_seq;
    return HDAG_CTX_NODE_MATCHED_NONE;
}
