/*
 * Hash DAG hash sequence
 */

#include <hdag/hash_seq.h>
#include <sys/param.h>
#include <string.h>

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

hdag_res
hdag_hash_seq_cmp(const struct hdag_hash_seq *hash_seq_a,
                  const struct hdag_hash_seq *hash_seq_b)
{
    hdag_res res;
    hdag_res res_a;
    hdag_res res_b;

    assert(hdag_hash_seq_is_valid(hash_seq_a));
    assert(hdag_hash_seq_is_valid(hash_seq_b));
    assert(hash_seq_a->hash_len == hash_seq_b->hash_len);

    size_t hash_len = MAX(hash_seq_a->hash_len, hash_seq_b->hash_len);
    /* Let's keep this relatively safe */
    if (hash_len > 4096) {
        return HDAG_RES_ERRNO_ARG(ENOBUFS);
    }

    uint8_t hash_a[hash_len] = {};
    uint8_t hash_b[hash_len] = {};

    do {
        res_a = HDAG_RES_TRY(hdag_hash_seq_next(hash_seq_a, hash_a));
        res_b = HDAG_RES_TRY(hdag_hash_seq_next(hash_seq_b, hash_b));

        /* If seq a has ended, but seq b didn't */
        if (res_a > res_b) {
            /* Report hash_seq_a < hash_seq_b */
            res = -1;
            break;
        }
        /* If seq b has ended, but seq a didn't */
        if (res_a < res_b) {
            /* Report hash_seq_a > hash_seq_b */
            res = 1;
            break;
        }
        /* If both sequences have ended at the same time */
        if (res_a == 1) {
            /* Report hash_seq_a == hash_seq_b */
            res = 0;
            break;
        }
        /* Sequences continue */
    } while(!(res = memcmp(hash_a, hash_b, hash_len)));

    /* Map the comparison result to the expected OK range */
    res += 2;

cleanup:
    return res;
}
