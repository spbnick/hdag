/*
 * Hash DAG abstract node iterator
 */

#ifndef _HDAG_NODE_ITER_H
#define _HDAG_NODE_ITER_H

#include <hdag/hash.h>
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

/**
 * Create a type ID for a single node iterator item via a constant expression
 * (without validation).
 *
 * @param _hash_len The length of the hashes referenced by the item.
 */
#define HDAG_NODE_ITER_ITEM_TYPE(_hash_len) \
    HDAG_TYPE_PRM(HDAG_TYPE_ID_STRUCT_NODE_ITER_ITEM, _hash_len)

/**
 * Check if a node iterator item type is valid.
 *
 * @param type  The type to check.
 *
 * @return True if the type is a valid node iterator item type, false
 *         otherwise.
 */
static inline bool
hdag_node_iter_item_type_is_valid(hdag_type type)
{
    return hdag_type_is_valid(type) &&
        HDAG_TYPE_GET_ID(type) == HDAG_TYPE_ID_STRUCT_NODE_ITER_ITEM &&
        hdag_hash_len_is_valid(HDAG_TYPE_GET_PRM(type));
}

/**
 * Create a type ID for a single node iterator item.
 *
 * @param hash_len  The length of the hashes referenced by the item.
 *
 * @return The created type ID.
 */
static inline hdag_type
hdag_node_iter_item_type(size_t hash_len)
{
    assert(hdag_hash_len_is_valid(hash_len));
    return HDAG_NODE_ITER_ITEM_TYPE(hash_len);
}

/**
 * Get the length of hashes referenced by a node iterator item type via a
 * constant expression (without validation).
 */
#define HDAG_NODE_ITER_ITEM_TYPE_GET_HASH_LEN(_type) \
    HDAG_TYPE_GET_PRM(_type)

/**
 * Get the length of hashes referenced by a node iterator item type.
 *
 * @param type  The node iter item type to get the hash length from.
 *
 * @return The retrieved hash length.
 */
static inline size_t
hdag_node_iter_item_type_get_hash_len(hdag_type type)
{
    assert(hdag_type_is_valid(type));
    return hdag_hash_len_validate(
        HDAG_NODE_ITER_ITEM_TYPE_GET_HASH_LEN(type)
    );
}

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
    assert(hdag_node_iter_item_type_is_valid(iter_a->item_type));
    assert(hdag_iter_is_valid(iter_b));
    assert(hdag_node_iter_item_type_is_valid(iter_b->item_type));

    size_t hash_len_a =
        hdag_node_iter_item_type_get_hash_len(iter_a->item_type);
    size_t hash_len_b =
        hdag_node_iter_item_type_get_hash_len(iter_b->item_type);

    assert(hash_len_a == hash_len_b);

    return hdag_iter_cmp(iter_a, iter_b,
                         hdag_node_iter_item_cmp,
                         (void *)(uintptr_t)hash_len_a);
}

#endif /* _HDAG_NODE_ITER_H */
