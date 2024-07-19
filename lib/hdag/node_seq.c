/*
 * Hash DAG node sequence
 */

#include <hdag/node_seq.h>

int
hdag_node_seq_next_none(void *node_seq_data, uint8_t *phash,
                        struct hdag_hash_seq *ptarget_hash_seq)
{
    (void)node_seq_data;
    (void)phash;
    (void)ptarget_hash_seq;
    return 1;
}
