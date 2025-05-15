/**
 * Hash DAG - general type-related definitions
 */

#include <hdag/node_iter.h>
#include <hdag/type.h>
#include <hdag/misc.h>

/**
 * The sizes of non-composite types indexed by type ID.
 */
static size_t hdag_type_sizes[] = {
    [HDAG_TYPE_ID_VOID]                     = 0,
    [HDAG_TYPE_ID_PTR]                      = sizeof(void *),
    [HDAG_TYPE_ID_UINT8]                    = sizeof(uint8_t),
    [HDAG_TYPE_ID_UINT16]                   = sizeof(uint16_t),
    [HDAG_TYPE_ID_UINT32]                   = sizeof(uint32_t),
    [HDAG_TYPE_ID_UINT64]                   = sizeof(uint64_t),
    [HDAG_TYPE_ID_SIZE]                     = sizeof(size_t),
    [HDAG_TYPE_ID_STRUCT_NODE_ITER_ITEM]    = sizeof(struct hdag_node_iter_item),
};

static_assert(HDAG_ARR_LEN(hdag_type_sizes) == HDAG_TYPE_ID_NUM);

size_t
hdag_type_layer_get_size(hdag_type_layer layer)
{
    assert(hdag_type_layer_is_valid(layer));
    return hdag_type_sizes[HDAG_TYPE_LAYER_GET_ID(layer)] *
           HDAG_TYPE_LAYER_GET_REP(layer);
}

size_t
hdag_type_get_size(hdag_type type)
{
    assert(hdag_type_is_valid(type));
    hdag_type_layer layer = HDAG_TYPE_GET_LAYER(type);
    enum hdag_type_id type_id = HDAG_TYPE_LAYER_GET_ID(layer);
    size_t rep = HDAG_TYPE_LAYER_GET_REP(layer);
    size_t size = hdag_type_sizes[type_id] * rep;
    if (type_id == HDAG_TYPE_ID_PTR) {
        size += hdag_type_get_size(HDAG_TYPE_DEREF(type)) * rep;
    }
    return size;
}
