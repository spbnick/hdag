/**
 * Hash DAG - abstract iterator.
 */

#include <hdag/iter.h>
#include <hdag/misc.h>
#include <sys/param.h>
#include <string.h>

hdag_res
hdag_iter_empty_next(const struct hdag_iter *iter, void **pitem)
{
    assert(hdag_iter_is_valid(iter));
    (void)iter;
    (void)pitem;
    return HDAG_RES_OK;
}

hdag_res
hdag_iter_cmp(const struct hdag_iter *iter_a, const struct hdag_iter *iter_b,
              hdag_res_cmp_fn cmp_fn, void *cmp_data)
{
    hdag_res res = HDAG_RES_INVALID;
    const void *a;
    const void *b;
    bool got_a;
    bool got_b;

    assert(hdag_iter_is_valid(iter_a));
    assert(hdag_iter_is_valid(iter_b));
    assert(iter_a->item_type == iter_b->item_type);
    assert(cmp_fn != NULL);

    do {
        got_a = HDAG_RES_TRY(hdag_iter_next_const(iter_a, &a));
        got_b = HDAG_RES_TRY(hdag_iter_next_const(iter_b, &b));

        /* If iterators are in sync */
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
    } while (
        HDAG_RES_TRY(hdag_res_cmp_validate(cmp_fn(a, b, cmp_data))) ==
        HDAG_RES_CMP_EQ
    );

cleanup:
    return res;
}

hdag_res
hdag_iter_are_intersecting(const struct hdag_iter *iter_a,
                           const struct hdag_iter *iter_b,
                           hdag_res_cmp_fn cmp_fn,
                           void *cmp_data)
{
    hdag_res res;
    const void *a;
    const void *b;
    bool got_a;
    bool got_b;

    assert(hdag_iter_is_valid(iter_a));
    assert(hdag_iter_is_valid(iter_b));
    assert(iter_a->item_type == iter_b->item_type);
    assert(cmp_fn != NULL);

    /* Get the first hashes */
    got_a = HDAG_RES_TRY(hdag_iter_next_const(iter_a, &a));
    got_b = HDAG_RES_TRY(hdag_iter_next_const(iter_b, &b));

    /* Until either iterator ends */
    while (got_a && got_b) {
        res = HDAG_RES_TRY(hdag_res_cmp_validate(
            cmp_fn(a, b, cmp_data)
        ));
        /* If iter_a is behind iter_b */
        if (res == HDAG_RES_CMP_LT) {
            /* Move iter_a forward */
            got_a = HDAG_RES_TRY(hdag_iter_next_const(iter_a, &a));
        /* Else, if iter_b is behind iter_a */
        } else if (res == HDAG_RES_CMP_GT) {
            /* Move iter_b forward */
            got_b = HDAG_RES_TRY(hdag_iter_next_const(iter_b, &b));
        /* Else, items match, and therefore iterators intersect */
        } else {
            res = 1;
            goto cleanup;
        }
    }

    /* No matches found, iterators do not intersect */
    res = 0;

cleanup:
    return res;
}
