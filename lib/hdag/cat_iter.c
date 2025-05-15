/*
 * Hash DAG - concatenated iterator
 */

#include <hdag/cat_iter.h>

hdag_res
hdag_cat_iter_next(const struct hdag_iter *iter, void **pitem)
{
    hdag_res res;
    struct hdag_cat_iter_data *data = hdag_cat_iter_data_validate(iter->data);

    while (true) {
        const struct hdag_iter *cur_iter = data->cur_iter;
        /* If an iterator has already been retrieved */
        if (cur_iter != NULL) {
            assert(hdag_iter_is_valid(cur_iter));
            assert(iter->item_type == HDAG_TYPE_VOID ||
                   cur_iter->item_type == HDAG_TYPE_VOID ||
                   cur_iter->item_type == iter->item_type);
            assert(!iter->item_mutable > !cur_iter->item_mutable);
            /* If we got an item from the current iterator */
            if (HDAG_RES_TRY(cur_iter->next_fn(cur_iter, pitem)) == 1) {
                /* Report we got one */
                return 1;
            }
            /* Else, the current iterator has ran out */
        }

        /* If we ran out of iterators while trying to get another */
        if (HDAG_RES_TRY(hdag_iter_next(data->iter_iter,
                                        (void **)&data->cur_iter)) == 0) {
            /* Report we have no more items */
            return 0;
        }
    }

cleanup:
    return res;
}
