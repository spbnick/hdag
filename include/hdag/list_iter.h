/**
 * Hash DAG - a pointer list iterator.
 */
#ifndef _HDAG_LIST_ITER_H
#define _HDAG_LIST_ITER_H

#include <hdag/iter.h>
#include <hdag/res.h>
#include <stdint.h>
#include <stdbool.h>

/** Pointer list iterator private data */
struct hdag_list_iter_data {
    /**
     * The list of pointers to iterate over.
     * Can be NULL if len is zero.
     */
    void    **list;
    /** The number of pointers in the list */
    size_t    len;
    /** The index of the currently-traversed pointer */
    size_t    idx;
};

/** The next-item retrieval function for pointer list iterator */
[[nodiscard]]
extern hdag_res hdag_list_iter_next(const struct hdag_iter *iter,
                                    void **pitem);

/**
 * Check if private list iterator data is valid.
 *
 * @param data  The list iterator data to check.
 *
 * @return True if the iterator data is valid, false otherwise.
 */
static inline bool
hdag_list_iter_data_is_valid(const struct hdag_list_iter_data *data)
{
    return
        data != NULL &&
        (data->list != NULL || data->len == 0) &&
        data->idx <= data->len;
}

/**
 * Validate private list iterator data.
 *
 * @param data  The list iterator data to validate.
 *
 * @return The validated list iterator data.
 */
static inline struct hdag_list_iter_data*
hdag_list_iter_data_validate(struct hdag_list_iter_data *data)
{
    assert(hdag_list_iter_data_is_valid(data));
    return data;
}

/**
 * Create a list iterator.
 *
 * @param item_type     The type of the items pointers to which are returned
 *                      by the created iterator. HDAG_TYPE_VOID if undefined.
 * @param item_mutable  True if the items returned by the created iterator
 *                      should be mutable. False if they should be constant.
 * @param data          The location for the private iterator data.
 * @param list          Pointer to the pointer list.
 * @param len           The length of the pointer list.
 */
static inline struct hdag_iter
hdag_list_iter(hdag_type item_type,
               bool item_mutable,
               struct hdag_list_iter_data *data,
               void **list,
               size_t len)
{
    assert(hdag_type_is_valid(item_type));
    assert(data != NULL);
    assert(list != NULL || len == 0);
    *data = (struct hdag_list_iter_data){.list = list, .len = len};
    return hdag_iter(hdag_list_iter_next,
                     NULL,
                     item_type,
                     item_mutable,
                     hdag_list_iter_data_validate(data));
}

#endif /* _HDAG_LIST_ITER_H */
