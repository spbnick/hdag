/*
 * Hash DAG - an abstract sequence of nodes
 */

#include <hdag/node_seq.h>
#include <string.h>

hdag_res
hdag_node_seq_item_cmp(const void *first, const void *second, void *data)
{
    hdag_res res;
    const struct hdag_node_seq_item *first_item = first;
    const struct hdag_node_seq_item *second_item = second;
    size_t hash_len = (uintptr_t)data;
    assert(hdag_node_seq_item_is_valid(first_item));
    assert(!hdag_node_seq_item_is_void(first_item));
    assert(hdag_node_seq_item_is_valid(second_item));
    assert(!hdag_node_seq_item_is_void(second_item));
    assert(hdag_hash_len_is_valid(hash_len));

    /* If node hashes are equal */
    res = HDAG_RES_TRY(
        hdag_res_cmp_mem(first_item->hash, second_item->hash, data)
    );
    if (res == HDAG_RES_CMP_EQ) {
        /* Compare target hash sequences */
        res = HDAG_RES_TRY(hdag_seq_cmp(
            first_item->target_hash_seq,
            second_item->target_hash_seq,
            hdag_res_cmp_mem,
            data
        ));
    }

cleanup:
    return res;
}
