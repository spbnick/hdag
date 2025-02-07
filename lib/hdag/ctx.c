/**
 * Hash DAG context.
 */

#include <hdag/ctx.h>

const struct hdag_ctx_node *
hdag_ctx_empty_get_node(struct hdag_ctx *ctx, const uint8_t *hash)
{
    (void)ctx;
    (void)hash;
    return NULL;
}
