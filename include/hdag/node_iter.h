/*
 * Hash DAG abstract node iterator
 */

#ifndef _HDAG_NODE_ITER_H
#define _HDAG_NODE_ITER_H

#include <hdag/iter.h>
#include <hdag/res.h>
#include <stdint.h>
#include <stdbool.h>

/** Type of items (nodes) returned from a node iterator */
struct hdag_node_iter_item {
    /** The node hash (length specified in the iterator) */
    const uint8_t      *hash;
    /**
     * The iterator of the node's target hashes
     * (item size, being hash length, must match the node iterator's)
     */
    struct hdag_iter    target_hash_iter;
};

/** The type ID of a single node iterator item */
#define HDAG_NODE_ITER_ITEM_TYPE \
    HDAG_TYPE_BASIC(HDAG_TYPE_ID_STRUCT_NODE_ITER_ITEM)

/**
 * Check if a node iterator item is valid.
 *
 * @param item  The item to check.
 *
 * @return True if the item is valid, false otherwise.
 */
static inline bool
hdag_node_iter_item_is_valid(const struct hdag_node_iter_item *item)
{
    return item != NULL && (
        item->hash == NULL ||
        hdag_iter_is_valid(&item->target_hash_iter)
    );
}

/**
 * Validate a node iterator item.
 *
 * @param item  The item to validate.
 *
 * @return The validated item.
 */
static inline struct hdag_node_iter_item*
hdag_node_iter_item_validate(struct hdag_node_iter_item *item)
{
    assert(hdag_node_iter_item_is_valid(item));
    return item;
}

/**
 * Check if a node iterator item is void.
 *
 * @param item  The item to check.
 *
 * @return True if the item is void, false otherwise.
 */
static inline bool
hdag_node_iter_item_is_void(const struct hdag_node_iter_item *item)
{
    assert(hdag_node_iter_item_is_valid(item));
    return item->hash == NULL;
}

/**
 * Compare two node iterator items.
 *
 * @param first     The pointer to the first value to compare.
 * @param second    The pointer to the second value to compare.
 * @param data      The length of each value's hash (uintptr_t).
 *
 * @return Universal comparison result.
 */
extern hdag_res hdag_node_iter_item_cmp(const void *first_value,
                                        const void *second_value,
                                        void *data);

/**
 * Compare items of two node iterators.
 *
 * @param iter_a    The first node iterator to compare.
 * @param iter_b    The second node iterator to compare.
 *
 * @return A universal comparison result code.
 */
static inline hdag_res
hdag_node_iter_cmp(const struct hdag_iter *iter_a,
                   const struct hdag_iter *iter_b)
{
    assert(hdag_iter_is_valid(iter_a));
    assert(iter_a->item_type == HDAG_NODE_ITER_ITEM_TYPE);
    assert(hdag_iter_is_valid(iter_b));
    assert(iter_b->item_type == HDAG_NODE_ITER_ITEM_TYPE);

    bool                hash_len_present;
    size_t              hash_len_a;
    size_t              hash_len_b;
    hash_len_present = hdag_iter_get_prop(
        iter_a, HDAG_ITER_PROP_ID_HASH_LEN, HDAG_TYPE_SIZE, &hash_len_a
    );
    assert(hash_len_present);
    (void)hash_len_present;
    hash_len_present = hdag_iter_get_prop(
        iter_b, HDAG_ITER_PROP_ID_HASH_LEN, HDAG_TYPE_SIZE, &hash_len_b
    );
    assert(hash_len_present);
    (void)hash_len_present;

    assert(hash_len_a == hash_len_b);

    return hdag_iter_cmp(iter_a, iter_b,
                         hdag_node_iter_item_cmp,
                         (void *)(uintptr_t)hash_len_a);
}

#endif /* _HDAG_NODE_ITER_H */
