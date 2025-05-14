/**
 * Hash DAG - abstract sequence.
 */

#include <hdag/seq.h>
#include <hdag/misc.h>
#include <sys/param.h>
#include <string.h>

hdag_res
hdag_seq_empty_next(struct hdag_seq *seq, void **pitem)
{
    assert(hdag_seq_is_valid(seq));
    (void)seq;
    (void)pitem;
    return HDAG_RES_OK;
}

hdag_res
hdag_seq_empty_reset(struct hdag_seq *seq)
{
    assert(hdag_seq_is_valid(seq));
    return HDAG_RES_OK;
}

hdag_res
hdag_seq_cmp(struct hdag_seq *seq_a, struct hdag_seq *seq_b,
             hdag_res_cmp_fn res_cmp_fn, void *res_cmp_data)
{
    hdag_res res = HDAG_RES_INVALID;
    const void *a;
    const void *b;
    bool got_a;
    bool got_b;

    assert(hdag_seq_is_valid(seq_a));
    assert(hdag_seq_is_valid(seq_b));
    assert(res_cmp_fn != NULL);

    do {
        got_a = HDAG_RES_TRY(hdag_seq_next_const(seq_a, &a));
        got_b = HDAG_RES_TRY(hdag_seq_next_const(seq_b, &b));

        /* If sequences are in sync */
        if (got_a == got_b) {
            /* If they ended */
            if (!got_a) {
                res = HDAG_RES_CMP_EQ;
                break;
            }
        /* Else, one of them has ended */
        } else {
            res = got_a < got_b ? HDAG_RES_CMP_LT : HDAG_RES_CMP_GT;
            break;
        }
    } while ((res = res_cmp_fn(a, b, res_cmp_data)) == HDAG_RES_CMP_EQ);

cleanup:
    return res;
}

hdag_res
hdag_seq_are_intersecting(struct hdag_seq *seq_a,
                          struct hdag_seq *seq_b,
                          hdag_res_cmp_fn res_cmp_fn,
                          void *res_cmp_data)
{
    hdag_res res;
    const void *a;
    const void *b;
    bool got_a;
    bool got_b;

    assert(hdag_seq_is_valid(seq_a));
    assert(hdag_seq_is_valid(seq_b));
    assert(res_cmp_fn != NULL);

    /* Get the first hashes */
    got_a = HDAG_RES_TRY(hdag_seq_next_const(seq_a, &a));
    got_b = HDAG_RES_TRY(hdag_seq_next_const(seq_b, &b));

    /* Until either sequence ends */
    while (got_a && got_b) {
        res = HDAG_RES_TRY(hdag_res_cmp_validate(
            res_cmp_fn(a, b, res_cmp_data)
        ));
        /* If seq_a is behind seq_b */
        if (res == HDAG_RES_CMP_LT) {
            /* Move seq_a forward */
            got_a = HDAG_RES_TRY(hdag_seq_next_const(seq_a, &a));
        /* Else, if seq_b is behind seq_a */
        } else if (res == HDAG_RES_CMP_GT) {
            /* Move seq_b forward */
            got_b = HDAG_RES_TRY(hdag_seq_next_const(seq_b, &b));
        /* Else, items match, and therefore sequences intersect */
        } else {
            res = 1;
            goto cleanup;
        }
    }

    /* No matches found, sequences do not intersect */
    res = 0;

cleanup:
    return res;
}
