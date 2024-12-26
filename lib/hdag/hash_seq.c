/*
 * Hash DAG hash sequence
 */

#include <hdag/hash_seq.h>

hdag_res
hdag_hash_seq_empty_next_fn(const struct hdag_hash_seq *hash_seq,
                            uint8_t *phash)
{
    (void)hash_seq;
    (void)phash;
    return 1;
}

void
hdag_hash_seq_empty_reset_fn(const struct hdag_hash_seq *hash_seq)
{
    (void)hash_seq;
}
