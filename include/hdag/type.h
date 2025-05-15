/**
 * Hash DAG - general type-related definitions
 */
#ifndef _HDAG_TYPE_H
#define _HDAG_TYPE_H

#include <hdag/res.h>
#include <stdbool.h>

/** IDs of various types which we need to identify dynamically */
enum hdag_type_id {
    /** no type (void) */
    HDAG_TYPE_ID_VOID,
    /** pointer */
    HDAG_TYPE_ID_PTR,
    /** uint8_t */
    HDAG_TYPE_ID_UINT8,
    /** uint16_t */
    HDAG_TYPE_ID_UINT16,
    /** uint32_t */
    HDAG_TYPE_ID_UINT32,
    /** uint64_t */
    HDAG_TYPE_ID_UINT64,
    /** size_t */
    HDAG_TYPE_ID_SIZE,
    /** struct hdag_node_iter_item */
    HDAG_TYPE_ID_STRUCT_NODE_ITER_ITEM,
    /** The number of type IDs, not an ID itself */
    HDAG_TYPE_ID_NUM
};

/** Number of bits required to represent any type ID */
#define HDAG_TYPE_ID_BITS   8
static_assert(HDAG_TYPE_ID_NUM <= (1 << HDAG_TYPE_ID_BITS));

/** A bitmask covering all possible values of a type ID */
#define HDAG_TYPE_ID_MASK   ((1 << HDAG_TYPE_ID_BITS) - 1)

/**
 * Check if a type ID is valid.
 *
 * @param id    The ID to check.
 *
 * @return True if the ID is valid, false otherwise.
 */
static inline bool
hdag_type_id_is_valid(enum hdag_type_id id)
{
    return id >= 0 && id < HDAG_TYPE_ID_NUM;
}

/**
 * Validate a type ID.
 *
 * @param id    The ID to validate.
 *
 * @return The validated ID.
 */
static inline enum hdag_type_id
hdag_type_id_validate(enum hdag_type_id id)
{
    assert(hdag_type_id_is_valid(id));
    return id;
}

/**
 * Get the type ID from a type definition layer via a constant expression
 * (without validation).
 */
#define HDAG_TYPE_LAYER_GET_ID(_layer)  ((_layer) & HDAG_TYPE_ID_MASK)

/**
 * Get the repetition count from a type definition layer via a constant
 * expression (without validation).
 */
#define HDAG_TYPE_LAYER_GET_REP(_layer)  (((_layer) >> HDAG_TYPE_ID_BITS) + 1)

/**
 * A (composite) type definition layer.
 *
 * The lowest HDAG_TYPE_ID_BITS specify the type ID (enum hdag_type_id).
 * The remaining bits specify the number of elements in an array, minus one.
 * Zero thus signifying that it's a single instance.
 */
typedef uint32_t hdag_type_layer;

/** Number of bits in a type definition layer */
#define HDAG_TYPE_LAYER_BITS    (sizeof(hdag_type_layer) * 8)

/**
 * Check a type definition layer is valid.
 *
 * @param layer The layer to check.
 *
 * @return True if the layer is valid, false otherwise.
 */
static inline bool
hdag_type_layer_is_valid(hdag_type_layer layer)
{
    return hdag_type_id_is_valid(HDAG_TYPE_LAYER_GET_ID(layer));
}

/**
 * Get the type ID from a type definition layer.
 *
 * @param layer The layer to get the type ID from.
 *
 * @return The retrieved type ID.
 */
static inline enum hdag_type_id
hdag_type_layer_get_id(hdag_type_layer layer)
{
    assert(hdag_type_layer_is_valid(layer));
    return hdag_type_id_validate(HDAG_TYPE_LAYER_GET_ID(layer));
}

/**
 * Get the size of a type layer.
 *
 * @param layer The type layer to retrieve the size of.
 *
 * @return The size of the layer, bytes.
 */
extern size_t hdag_type_layer_get_size(hdag_type_layer layer);

/**
 * A (composite) type definition.
 *
 * A series of type definition layers, terminated by a layer with type ID
 * other than HDAG_TYPE_ID_PTR. A type can also be treated as its top layer.
 */
typedef uint64_t hdag_type;

/**
 * Define a non-aggregate type via a constant expression (without validation).
 *
 * @param _id   The ID of the type to define.
 */
#define HDAG_TYPE_DEF(_id)  (_id)

/**
 * Get the first layer of a type via a constant expression
 * (without validation).
 */
#define HDAG_TYPE_GET_LAYER(_type) \
    ((hdag_type_layer)(_type))

/**
 * Get the ID of a type's first layer via a constant expression
 * (without validation).
 */
#define HDAG_TYPE_GET_ID(_type) \
    HDAG_TYPE_LAYER_GET_ID(HDAG_TYPE_GET_LAYER(_type))

/**
 * Define an array type via a constant expression (without validation).
 *
 * @param _id   The ID of the type to define.
 * @param _size The non-zero size of the defined array.
 */
#define HDAG_TYPE_ARR(_id, _size)  ( \
    (_id) | (((_size) - 1) << HDAG_TYPE_ID_BITS)    \
)

/**
 * Define of a pointer type referencing another type via a constant expression
 * (without validation).
 *
 * @param _type The type being referenced.
 */
#define HDAG_TYPE_REF(_type) ( \
    ((_type) << HDAG_TYPE_LAYER_BITS) | \
    HDAG_TYPE_DEF(HDAG_TYPE_ID_PTR)         \
)

/** Dereference a type via a constant expression (without validation) */
#define HDAG_TYPE_DEREF(_type) (_type >> HDAG_TYPE_LAYER_BITS)

/**
 * Define a pointer array type referencing another type via a constant
 * expression (without validation).
 *
 * @param _type The type being referenced.
 * @param _size The non-zero size of the defined array.
 */
#define HDAG_TYPE_REF_ARR(_type, _size) ( \
    ((_type) << HDAG_TYPE_LAYER_BITS) |     \
    HDAG_TYPE_ARR(HDAG_TYPE_ID_PTR, _size)  \
)

/**
 * Check if a type definition is valid.
 *
 * @param type  The type to check.
 *
 * @return True if the type definition is valid. False otherwise.
 */
static inline bool
hdag_type_is_valid(hdag_type type)
{
    hdag_type_layer layer = HDAG_TYPE_GET_LAYER(type);
    return hdag_type_layer_is_valid(layer) && (
        HDAG_TYPE_LAYER_GET_ID(layer) != HDAG_TYPE_ID_PTR ||
        hdag_type_is_valid(HDAG_TYPE_DEREF(type))
    );
}

/**
 * Validate a type definition.
 *
 * @param type  The type definition to validate.
 *
 * @return The validated type definition.
 */
static inline hdag_type
hdag_type_validate(hdag_type type)
{
    assert(hdag_type_is_valid(type));
    return type;
}

/**
 * Create a non-aggregate type definition.
 *
 * @param id    The ID of the type to define.
 *
 * @return The created type definition.
 */
static inline hdag_type
hdag_type_def(enum hdag_type_id id)
{
    return hdag_type_validate(HDAG_TYPE_DEF(hdag_type_id_is_valid(id)));
}

/**
 * Get the size of all layers of a type.
 *
 * @param type  The type to retrieve the size of.
 *
 * @return The size of the type, bytes.
 */
extern size_t hdag_type_get_size(hdag_type type);

/** The void type */
#define HDAG_TYPE_VOID HDAG_TYPE_DEF(HDAG_TYPE_ID_VOID)
/** The uint8_t type */
#define HDAG_TYPE_UINT8 HDAG_TYPE_DEF(HDAG_TYPE_ID_UINT8)
/** The uint16_t type */
#define HDAG_TYPE_UINT16 HDAG_TYPE_DEF(HDAG_TYPE_ID_UINT16)
/** The uint32_t type */
#define HDAG_TYPE_UINT32 HDAG_TYPE_DEF(HDAG_TYPE_ID_UINT32)
/** The uint64_t type */
#define HDAG_TYPE_UINT64 HDAG_TYPE_DEF(HDAG_TYPE_ID_UINT64)
/** The size_t type */
#define HDAG_TYPE_SIZE HDAG_TYPE_DEF(HDAG_TYPE_ID_SIZE)

#endif /* _HDAG_TYPE_H */
