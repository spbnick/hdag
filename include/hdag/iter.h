/**
 * Hash DAG - abstract iterator.
 */
#ifndef _HDAG_ITER_H
#define _HDAG_ITER_H

#include <hdag/iter_prop.h>
#include <hdag/res.h>
#include <hdag/type.h>
#include <hdag/misc.h>
#include <stdint.h>
#include <stdbool.h>

/* The forward declaration of the iterator */
struct hdag_iter;

/**
 * The prototype for a function returning the next item from an iterator.
 *
 * @param iter      The iterator being traversed.
 * @param pitem     Location for the pointer to the retrieved iterator item.
 *                  The type of the item is defined by the iterator's
 *                  "item_type". The output pointer is only valid until the
 *                  next call.
 *
 * @return  One, if the item was retrieved successfully.
 *          Zero (HDAG_RES_OK), if there were no more items.
 *          A negative number (a failure result) if item retrieval has failed.
 *          The particular choice of values allows writing loop conditions
 *          like this: while (HDAG_RES_TRY(..._next(iter, &item))) {...}
 */
typedef hdag_res (*hdag_iter_next_fn)(const struct hdag_iter *iter,
                                      void **pitem);

/**
 * The prototype for a function returning the value of a property from an
 * iterator.
 *
 * @param iter      The iterator to retrieve the property from.
 * @param id        The ID of the property.
 * @param type      The type of the property.
 * @param pvalue    The location for the retrieved property value.
 *
 * @return True if the iterator had the property and did output its value.
 *         False if the property ID was unknown, or the type was incorrect.
 */
typedef bool (*hdag_iter_get_prop_fn)(const struct hdag_iter *iter,
                                      enum hdag_iter_prop_id id,
                                      hdag_type type,
                                      void *pvalue);

/** An abstract iterator */
struct hdag_iter {
    /** The function retrieving the next item from the iterator */
    hdag_iter_next_fn       next_fn;
    /**
     * The function retrieving a property from the iterator.
     * Can be NULL to signify iterator has no properties.
     */
    hdag_iter_get_prop_fn   get_prop_fn;
    /**
     * The type of the items, pointers to which are returned by the iterator.
     */
    hdag_type               item_type;
#ifndef NDEBUG
    /**
     * True if the items returned by iterator can be changed (are "mutable").
     * False if not (are "constant"). In the former case the iterator itself
     * can also be called "mutable", and in the latter - "constant". In the
     * latter case, only the hdag_iter_next_const() function can be used to
     * retrieve items.
     */
    bool                    item_mutable;
#endif
    /** The opaque private iterator instance data */
    void                   *data;
};

/**
 * Check if an iterator is valid.
 *
 * @param iter  The iterator to check.
 *
 * @return True if the iterator is valid, false otherwise.
 */
static inline bool
hdag_iter_is_valid(const struct hdag_iter *iter)
{
    return
        iter != NULL &&
        iter->next_fn != NULL &&
        hdag_type_is_valid(iter->item_type);
}

/**
 * Retrieve a property value from an iterator.
 *
 * @param iter      The iterator to retrieve the property from.
 * @param id        The ID of the property.
 * @param type      The type of the property.
 * @param pvalue    The location for the retrieved property value.
 *                  Can be NULL to not output the value.
 *
 * @return True if the iterator had the property (and did output its value
 *         given the location). False if the property ID was unknown or the
 *         property type was incorrect.
 */
[[nodiscard]]
static inline bool
hdag_iter_get_prop(const struct hdag_iter *iter,
                   enum hdag_iter_prop_id id,
                   hdag_type type,
                   void *pvalue)
{
    assert(hdag_iter_is_valid(iter));
    assert(hdag_iter_prop_id_is_valid(id));
    assert(hdag_type_is_valid(type));
    return iter->get_prop_fn && iter->get_prop_fn(iter, id, type, pvalue);
}

/**
 * Check if an iterator is "mutable", that is if the items returned from it
 * can be modified.
 *
 * @param iter The iterator to check.
 *
 * @return True if the iterator is "mutable", false otherwise.
 */
static inline bool
hdag_iter_is_mutable(const struct hdag_iter *iter)
{
    assert(hdag_iter_is_valid(iter));
    return iter->item_mutable;
}

/**
 * Create an abstract iterator
 *
 * @param next_fn       The function retrieving the next item from the
 *                      iterator.
 * @param get_prop_fn   The function retrieving the value of a property from
 *                      the iterator.
 * @param item_type     The type of the items (pointers to which are) returned
 *                      by the iterator.
 * @param item_mutable  True if the items returned by the iterator can be
 *                      modified. False if not, and the items can only be
 *                      retrieved using hdag_iter_next_const().
 * @param data          The opaque private iterator instance data.
 *
 * @return The created iterator.
 */
static inline struct hdag_iter
hdag_iter(hdag_iter_next_fn next_fn,
          hdag_iter_get_prop_fn get_prop_fn,
          hdag_type item_type,
          bool item_mutable,
          void *data)
{
    assert(next_fn != NULL);
    assert(hdag_type_is_valid(item_type));
#ifdef NDEBUG
    (void)item_mutable;
#endif
    return (struct hdag_iter){
        .next_fn = next_fn,
        .get_prop_fn = get_prop_fn,
        .item_type = item_type,
#ifndef NDEBUG
        .item_mutable = item_mutable,
#endif
        .data = data,
    };
}

/**
 * Get the next mutable item out of an iterator, if possible.
 *
 * @param iter  The iterator to retrieve the next item from.
 *              Must be "mutable".
 * @param pitem Location for the pointer to the retrieved item.
 *              Could be NULL to not have the pointer output.
 *              Can be modified. Only valid until the next call.
 *
 * @return  One, if the item was retrieved successfully.
 *          Zero (HDAG_RES_OK), if there were no more items.
 *          A negative number (a failure result) if item retrieval has failed.
 *          The particular choice of values allows writing loop conditions
 *          like this: while (HDAG_RES_TRY(..._next(iter, &item))) {...}
 */
static inline hdag_res
hdag_iter_next(const struct hdag_iter *iter, void **pitem)
{
    void *item;
    assert(hdag_iter_is_valid(iter));
    assert(hdag_iter_is_mutable(iter));
    return iter->next_fn(iter, pitem ? pitem : &item);
}

/**
 * Get the next constant item out of an iterator, if possible.
 *
 * @param iter  The iterator to retrieve the next item from.
 * @param pitem Location for the pointer to the retrieved item.
 *              Could be NULL to not have the pointer output.
 *              Cannot be modified. Only valid until the next call.
 *
 * @return  One, if the item was retrieved successfully.
 *          Zero (HDAG_RES_OK), if there were no more items.
 *          A negative number (a failure result) if item retrieval has failed.
 *          The particular choice of values allows writing loop conditions
 *          like this: while (HDAG_RES_TRY(..._next(iter, &item))) {...}
 */
static inline hdag_res
hdag_iter_next_const(const struct hdag_iter *iter, const void **pitem)
{
    void *item;
    assert(hdag_iter_is_valid(iter));
    return iter->next_fn(iter, pitem ? (void **)pitem : &item);
}

/** A next-item retrieval function which never returns anything */
[[nodiscard]]
extern hdag_res hdag_iter_empty_next(const struct hdag_iter *iter,
                                     void **pitem);

/** A property value retrieval function which never returns anything */
[[nodiscard]]
extern bool hdag_iter_empty_get_prop(const struct hdag_iter *iter,
                                     enum hdag_iter_prop_id id,
                                     hdag_type type,
                                     void *pvalue);

/**
 * Compare items of two iterators.
 *
 * @param iter_a    The first iterator to compare.
 * @param iter_b    The second iterator to compare.
 * @param cmp_fn    The item comparison function.
 * @param cmp_data  The private data to pass to the item comparison
 *                  function.
 *
 * @return A universal comparison result code.
 */
extern hdag_res hdag_iter_cmp(const struct hdag_iter *iter_a,
                              const struct hdag_iter *iter_b,
                              hdag_res_cmp_fn cmp_fn,
                              void *cmp_data);

/**
 * Check if two iterators have any items in common.
 * The iterators must be sorted in ascending order, according to the
 * comparison function.
 *
 * @param iter_a    The first iterator to check.
 * @param iter_b    The second iterator to check.
 * @param cmp_fn    The item comparison function.
 * @param cmp_data  The private data to pass to the item comparison
 *                  function.
 *
 * @return A universal result code:
 *         0 - the iterators have no common items,
 *         1 - the iterators *do* have common items,
 *         or a fault.
 *         The particular choice of values allows writing conditions like
 *         this: if (HDAG_RES_TRY(..._intersecting(iter_a, iter_b))) {...}
 */
extern hdag_res hdag_iter_are_intersecting(const struct hdag_iter *iter_a,
                                           const struct hdag_iter *iter_b,
                                           hdag_res_cmp_fn cmp_fn,
                                           void *cmp_data);

#endif /* _HDAG_ITER_H */
