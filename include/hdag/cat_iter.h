/*
 * Hash DAG - concatenated iterator
 */

#ifndef _HDAG_CAT_ITER_H
#define _HDAG_CAT_ITER_H

#include <hdag/iter.h>
#include <hdag/res.h>
#include <stdint.h>
#include <stdbool.h>

/** Concatenated iterator private data */
struct hdag_cat_iter_data {
    /** The iterator returning iterators to concatenate */
    const struct hdag_iter *iter_iter;
    /** The current iterator being traversed */
    const struct hdag_iter *cur_iter;
};

/** The next-item retrieval function for a concatenated iterator */
[[nodiscard]]
extern hdag_res hdag_cat_iter_next(const struct hdag_iter *iter,
                                   void **pitem);

/**
 * Check if private concatenated iterator data is valid.
 *
 * @param data   The concatenated iterator data to check.
 *
 * @return True if the iterator data is valid, false otherwise.
 */
static inline bool
hdag_cat_iter_data_is_valid(const struct hdag_cat_iter_data *data)
{
    return
        data != NULL &&
        hdag_iter_is_valid(data->iter_iter) &&
        (data->cur_iter == NULL || hdag_iter_is_valid(data->cur_iter));
}

/**
 * Validate private concatenated iterator data.
 *
 * @param data  The concatenated iterator data to validate.
 *
 * @return The validated iterator data.
 */
static inline struct hdag_cat_iter_data*
hdag_cat_iter_data_validate(struct hdag_cat_iter_data *data)
{
    assert(hdag_cat_iter_data_is_valid(data));
    return data;
}

/**
 * Create a concat iterator.
 *
 * @param item_type     The type of the items returned by the created
 *                      iterator. HDAG_TYPE_VOID if undefined. In the
 *                      former case each iterator returned by iter_iter should
 *                      have items of this type, or HDAG_TYPE_VOID.
 * @param item_mutable  True if the items returned by the created iterator
 *                      should be mutable. False if they should be constant.
 *                      In the former case each iterator returned by iter_iter
 *                      should be mutable.
 * @param data          The location for the private iterator data.
 * @param iter_iter     The iterator returning iterators to concatenate.
 */
static inline struct hdag_iter
hdag_cat_iter(hdag_type item_type,
              bool item_mutable,
              struct hdag_cat_iter_data *data,
              const struct hdag_iter *iter_iter)
{
    assert(hdag_type_is_valid(item_type));
    assert(data != NULL);
    assert(hdag_iter_is_valid(iter_iter));
    *data = (struct hdag_cat_iter_data){.iter_iter = iter_iter};
    return hdag_iter(hdag_cat_iter_next,
                     item_type,
                     item_mutable,
                     hdag_cat_iter_data_validate(data));
}

#endif /* _HDAG_CAT_ITER_H */
