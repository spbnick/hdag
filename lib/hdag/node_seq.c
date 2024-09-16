/*
 * Hash DAG node sequence
 */

#include <hdag/node_seq.h>

hdag_res
hdag_node_seq_empty_next_fn(const struct hdag_node_seq *node_seq,
                            uint8_t *phash,
                            struct hdag_hash_seq *ptarget_hash_seq)
{
    (void)node_seq;
    (void)phash;
    (void)ptarget_hash_seq;
    return 1;
}
