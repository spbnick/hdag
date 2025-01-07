/*
 * Hash DAG (lexicographically) sorted hash array
 */

#include <hdag/hashes.h>
#include <hdag/misc.h>
#include <sys/param.h>
#include <string.h>

hdag_res
hdag_hashes_seq_next(struct hdag_hash_seq *base_seq, uint8_t *phash)
{
    struct hdag_hashes_seq *seq = HDAG_CONTAINER_OF(
        struct hdag_hashes_seq, base, base_seq
    );
    if (seq->next_idx >= seq->hash_num) {
        return 1;
    }
    memcpy(phash,
           seq->hashes + base_seq->hash_len * seq->next_idx,
           base_seq->hash_len);
    seq->next_idx++;
    return 0;
}

void
hdag_hashes_seq_reset(struct hdag_hash_seq *base_seq)
{
    struct hdag_hashes_seq *seq = HDAG_CONTAINER_OF(
        struct hdag_hashes_seq, base, base_seq
    );
    seq->next_idx = 0;
}
