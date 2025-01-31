/**
 * Hash DAG node sequence
 */

#include <hdag/node_seq.h>
#include <string.h>
#include <sys/param.h>

void
hdag_node_seq_empty_reset(struct hdag_node_seq *node_seq)
{
    (void)node_seq;
}

hdag_res
hdag_node_seq_empty_next(struct hdag_node_seq *node_seq,
                         const uint8_t **phash,
                         struct hdag_hash_seq **ptarget_hash_seq)
{
    (void)node_seq;
    (void)phash;
    (void)ptarget_hash_seq;
    return 1;
}

hdag_res
hdag_node_seq_cmp(struct hdag_node_seq *seq_a, struct hdag_node_seq *seq_b)
{
    hdag_res res;
    hdag_res res_a;
    hdag_res res_b;

    assert(hdag_node_seq_is_valid(seq_a));
    assert(hdag_node_seq_is_valid(seq_b));
    assert(seq_a->hash_len == seq_b->hash_len);

    uint16_t hash_len = MIN(seq_a->hash_len, seq_b->hash_len);
    const uint8_t *hash_a;
    const uint8_t *hash_b;
    struct hdag_hash_seq *target_hash_seq_a;
    struct hdag_hash_seq *target_hash_seq_b;

    do {
        res_a = HDAG_RES_TRY(hdag_node_seq_next(seq_a, &hash_a,
                                                &target_hash_seq_a));
        res_b = HDAG_RES_TRY(hdag_node_seq_next(seq_b, &hash_b,
                                                &target_hash_seq_b));

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
    } while (!(res = memcmp(hash_a, hash_b, hash_len)) &&
             !(res = HDAG_RES_TRY(
                hdag_hash_seq_cmp(target_hash_seq_a, target_hash_seq_b)
               ) - 2));

    /* Map the comparison result to the expected OK range */
    res += 2;

cleanup:
    return res;
}
