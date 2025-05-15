/*
 * Hash DAG - an abstract iterator of nodes
 */

#include <hdag/node_iter.h>
#include <hdag/hash.h>
#include <string.h>

hdag_res
hdag_node_iter_item_cmp(const void *first, const void *second, void *data)
{
    hdag_res res;
    const struct hdag_node_iter_item *first_item = first;
    const struct hdag_node_iter_item *second_item = second;
    size_t hash_len = (uintptr_t)data;
    assert(hdag_node_iter_item_is_valid(first_item));
    assert(!hdag_node_iter_item_is_void(first_item));
    assert(hdag_node_iter_item_is_valid(second_item));
    assert(!hdag_node_iter_item_is_void(second_item));
    assert(hdag_hash_len_is_valid(hash_len));
    assert(first_item->target_hash_iter.item_type ==
           HDAG_TYPE_ARR(HDAG_TYPE_ID_UINT8, hash_len));
    assert(second_item->target_hash_iter.item_type ==
           HDAG_TYPE_ARR(HDAG_TYPE_ID_UINT8, hash_len));

    /* If node hashes are equal */
    res = hdag_res_cmp_mem(first_item->hash, second_item->hash, data);
    if (res == HDAG_RES_CMP_EQ) {
        /* Compare target hash iterators */
        res = hdag_iter_cmp(
            &first_item->target_hash_iter,
            &second_item->target_hash_iter,
            hdag_res_cmp_mem,
            data
        );
    }

    return res;
}
