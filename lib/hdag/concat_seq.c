/*
 * Hash DAG - concatenated sequence
 */

#include <hdag/concat_seq.h>

hdag_res
hdag_concat_seq_next(struct hdag_seq *base_seq, void **pitem)
{
    hdag_res res;
    struct hdag_concat_seq *seq = HDAG_CONCAT_SEQ_FROM_SEQ(base_seq);

    while (true) {
        /* If a sequence has already been retrieved */
        if (seq->seq != NULL) {
            /* If we got an item from the current sequence */
            if (HDAG_RES_TRY(
                    hdag_seq_is_variable(base_seq)
                        ? hdag_seq_next(seq->seq, pitem)
                        : hdag_seq_next_const(seq->seq, (const void **)pitem)
                ) == 1) {
                /* Report we got one */
                return 1;
            }
            /* Else, the current sequence has ran out */
        }

        /* If we ran out of sequences while trying to get another */
        if (HDAG_RES_TRY(hdag_seq_next(seq->seq_seq,
                                       (void **)&seq->seq)) == 0) {
            /* Report we have no more items */
            return 0;
        }
        /* Reset the retrieved sequence, if all of them support it */
        if (hdag_seq_is_resettable(base_seq)) {
            HDAG_RES_TRY(hdag_seq_reset(seq->seq));
        }
    }

cleanup:
    return res;
}

hdag_res
hdag_concat_seq_reset(struct hdag_seq *base_seq)
{
    struct hdag_concat_seq *seq = HDAG_CONCAT_SEQ_FROM_SEQ(base_seq);
    return hdag_seq_reset(seq->seq_seq);
}
