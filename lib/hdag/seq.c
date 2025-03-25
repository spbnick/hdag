/**
 * Hash DAG - abstract sequence.
 */

#include <hdag/seq.h>
#include <hdag/misc.h>
#include <sys/param.h>
#include <string.h>

hdag_res
hdag_seq_empty_next(struct hdag_seq *seq, const void **pitem)
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
             hdag_cmp_fn cmp_fn, void *cmp_data)
{
    hdag_res res = HDAG_RES_INVALID;
    int relation;
    const void *a;
    const void *b;
    bool got_a;
    bool got_b;

    assert(hdag_seq_is_valid(seq_a));
    assert(hdag_seq_is_valid(seq_b));
    assert(cmp_fn != NULL);

    do {
        got_a = HDAG_RES_TRY(hdag_seq_next(seq_a, &a));
        got_b = HDAG_RES_TRY(hdag_seq_next(seq_b, &b));

        /* If sequences are in sync */
        if (got_a == got_b) {
            /* If they ended */
            if (!got_a) {
                relation = 0;
                break;
            }
        /* Else, one of them has ended */
        } else {
            relation = got_a < got_b ? -1 : 1;
            break;
        }
    } while ((relation = cmp_fn(a, b, cmp_data)) == 0);

    /* Map the relation to the expected OK range */
    res = hdag_cmp_validate(relation) + 2;

cleanup:
    return res;
}

hdag_res
hdag_seq_are_intersecting(struct hdag_seq *seq_a,
                          struct hdag_seq *seq_b,
                          hdag_cmp_fn cmp_fn,
                          void *cmp_data)
{
    hdag_res res;
    const void *a;
    const void *b;
    bool got_a;
    bool got_b;
    int relation;

    assert(hdag_seq_is_valid(seq_a));
    assert(hdag_seq_is_valid(seq_b));
    assert(cmp_fn != NULL);

    /* Get the first hashes */
    got_a = HDAG_RES_TRY(hdag_seq_next(seq_a, &a));
    got_b = HDAG_RES_TRY(hdag_seq_next(seq_b, &b));

    /* Until either sequence ends */
    while (got_a && got_b) {
        relation = cmp_fn(a, b, cmp_data);
        /* If seq_a is behind seq_b */
        if (relation < 0) {
            /* Move seq_a forward */
            got_a = HDAG_RES_TRY(hdag_seq_next(seq_a, &a));
        /* Else, if seq_b is behind seq_a */
        } else if (relation > 0) {
            /* Move seq_b forward */
            got_b = HDAG_RES_TRY(hdag_seq_next(seq_b, &b));
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
