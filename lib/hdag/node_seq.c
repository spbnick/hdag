/**
 * Hash DAG node sequence
 */

#include <hdag/node_seq.h>
#include <hdag/misc.h>
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
hdag_node_seq_cat_next(struct hdag_node_seq *base_seq,
                       const uint8_t **phash,
                       struct hdag_hash_seq **ptarget_hash_seq)
{
    hdag_res res;
    struct hdag_node_seq_cat *seq = HDAG_CONTAINER_OF(
        struct hdag_node_seq_cat, base, base_seq
    );
    assert(hdag_node_seq_is_valid(base_seq));

    /* For each unfinished sequence */
    for (; seq->pcat_seq_idx < seq->pcat_seq_num; seq->pcat_seq_idx++) {
        res = hdag_node_seq_next(seq->pcat_seq_list[seq->pcat_seq_idx],
                                 phash, ptarget_hash_seq);
        /* If the node retrieval succeeded or failed */
        if (res <= HDAG_RES_OK) {
            return res;
        }
        /* Else the current sequence ended */
    }
    /* Signal we have no more nodes */
    return 1;
}

void
hdag_node_seq_cat_reset(struct hdag_node_seq *base_seq)
{
    size_t i;
    struct hdag_node_seq_cat *seq = HDAG_CONTAINER_OF(
        struct hdag_node_seq_cat, base, base_seq
    );
    assert(hdag_node_seq_is_valid(base_seq));
#ifndef NDEBUG
    for (i = 0; i < seq->pcat_seq_num; i++) {
        if (!hdag_node_seq_is_resettable(seq->pcat_seq_list[i])) {
            break;
        }
    }
    assert(i == seq->pcat_seq_num &&
           "All concatenated sequences are resettable");
#endif
    for (i = 0; i < seq->pcat_seq_num; i++) {
        hdag_node_seq_reset(seq->pcat_seq_list[i]);
    }
    seq->pcat_seq_idx = 0;
}

struct hdag_node_seq *
hdag_node_seq_cat_init(struct hdag_node_seq_cat *pseq,
                       uint16_t hash_len,
                       struct hdag_node_seq **pcat_seq_list,
                       size_t pcat_seq_num)
{
    size_t i;
    hdag_node_seq_reset_fn reset = hdag_node_seq_cat_reset;

    assert(pseq != NULL);
    assert(hdag_hash_len_is_valid(hash_len));
    assert(pcat_seq_list != NULL || pcat_seq_num == 0);

    for (i = 0; i < pcat_seq_num; i++) {
        assert(hdag_node_seq_is_valid(pcat_seq_list[i]));
        assert(pcat_seq_list[i]->hash_len == hash_len);
        if (!hdag_node_seq_is_resettable(pcat_seq_list[i])) {
            reset = NULL;
        }
    }

    *pseq = (struct hdag_node_seq_cat){
        .base = {
            .hash_len = hash_len,
            .reset_fn = reset,
            .next_fn = hdag_node_seq_cat_next,
        },
        .pcat_seq_list = pcat_seq_list,
        .pcat_seq_num = pcat_seq_num,
        .pcat_seq_idx = 0,
    };

    assert(hdag_node_seq_is_valid(&pseq->base));
    return &pseq->base;
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
