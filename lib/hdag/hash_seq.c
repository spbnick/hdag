/**
 * Hash DAG hash sequence
 */

#include <hdag/hash_seq.h>
#include <hdag/misc.h>
#include <sys/param.h>
#include <string.h>

hdag_res
hdag_hash_seq_empty_next(struct hdag_hash_seq *hash_seq,
                         const uint8_t **phash)
{
    (void)hash_seq;
    (void)phash;
    return 1;
}

void
hdag_hash_seq_empty_reset(struct hdag_hash_seq *hash_seq)
{
    (void)hash_seq;
}

hdag_res
hdag_hash_seq_cmp(struct hdag_hash_seq *seq_a,
                  struct hdag_hash_seq *seq_b)
{
    hdag_res res;
    hdag_res res_a;
    hdag_res res_b;

    assert(hdag_hash_seq_is_valid(seq_a));
    assert(hdag_hash_seq_is_valid(seq_b));
    assert(seq_a->hash_len == seq_b->hash_len);

    uint16_t hash_len = MIN(seq_a->hash_len, seq_b->hash_len);
    const uint8_t *hash_a;
    const uint8_t *hash_b;

    do {
        res_a = HDAG_RES_TRY(hdag_hash_seq_next(seq_a, &hash_a));
        res_b = HDAG_RES_TRY(hdag_hash_seq_next(seq_b, &hash_b));

        /* If seq a has ended, but seq b didn't */
        if (res_a > res_b) {
            /* Report seq_a < seq_b */
            res = -1;
            break;
        }
        /* If seq b has ended, but seq a didn't */
        if (res_a < res_b) {
            /* Report seq_a > seq_b */
            res = 1;
            break;
        }
        /* If both sequences have ended at the same time */
        if (res_a == 1) {
            /* Report seq_a == seq_b (adjusted for hash length diff) */
            res = seq_a->hash_len == seq_b->hash_len ? 0 : (
                seq_a->hash_len < seq_b->hash_len ? -1 : 1
            );
            break;
        }
        /* Sequences continue */
    } while(!(res = memcmp(hash_a, hash_b, hash_len)));

    /* Map the comparison result to the expected OK range */
    res += 2;

cleanup:
    return res;
}

extern hdag_res
hdag_hash_seq_are_intersecting(struct hdag_hash_seq *seq_a,
                               struct hdag_hash_seq *seq_b)
{
    hdag_res res = HDAG_RES_INVALID;
    hdag_res res_a;
    hdag_res res_b;
    int relation;

    assert(hdag_hash_seq_is_valid(seq_a));
    assert(hdag_hash_seq_is_valid(seq_b));
    assert(seq_a->hash_len == seq_b->hash_len);

    if (seq_a->hash_len != seq_b->hash_len) {
        return 0;
    }

    const uint8_t *hash_a;
    const uint8_t *hash_b;

    /* Get the first hashes */
    res_a = HDAG_RES_TRY(hdag_hash_seq_next(seq_a, &hash_a));
    res_b = HDAG_RES_TRY(hdag_hash_seq_next(seq_b, &hash_b));

    /* Until either sequence ends */
    while (res_a == 0 && res_b == 0) {
        relation = memcmp(hash_a, hash_b, seq_a->hash_len);
        /* If seq_a is behind seq_b */
        if (relation < 0) {
            /* Move seq_a forward */
            res_a = HDAG_RES_TRY(hdag_hash_seq_next(seq_a, &hash_a));
        /* Else, if seq_b is behind seq_a */
        } else if (relation > 0) {
            /* Move seq_b forward */
            res_b = HDAG_RES_TRY(hdag_hash_seq_next(seq_b, &hash_b));
        /* Else, hashes match, and therefore sequences intersect */
        } else {
            res = 1;
            goto cleanup;
        }
    }

    /* No matches found */
    res = 0;

cleanup:
    return HDAG_RES_ERRNO_IF_INVALID(res);
}
