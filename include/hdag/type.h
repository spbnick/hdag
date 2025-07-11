/**
 * Hash DAG - general type-related definitions
 */
#ifndef _HDAG_TYPE_H
#define _HDAG_TYPE_H

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
#define HDAG_TYPE_ID_MASK   ((1ULL << HDAG_TYPE_ID_BITS) - 1)

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
 * The sizes of non-composite types indexed by type ID.
 * If SIZE_MAX, then the corresponding function from
 * hdag_type_layer_get_inst_size_fns should be used instead.
 */
extern const size_t hdag_type_sizes[HDAG_TYPE_ID_NUM];

/** Number of bits in a repetition count (array size) */
#define HDAG_TYPE_REP_BITS  16

/** A bitmask covering all possible values of a repetition count */
#define HDAG_TYPE_REP_MASK  ((1ULL << HDAG_TYPE_REP_BITS) - 1)

/**
 * Check a repetition count is valid.
 *
 * @param rep   The repetition count to check.
 *
 * @return True if the count is valid.
 */
static inline bool
hdag_type_rep_is_valid(uint64_t rep)
{
    return rep <= HDAG_TYPE_REP_MASK;
}

/**
 * Validate a repetition count.
 *
 * @param rep   The repetition count to validate.
 *
 * @return The validated repetition count.
 */
static inline uint64_t
hdag_type_rep_validate(uint64_t rep)
{
    assert(hdag_type_rep_is_valid(rep));
    return rep;
}

/** Number of bits in a type parameter */
#define HDAG_TYPE_PRM_BITS  40

/** A bitmask covering all possible values of a parameter */
#define HDAG_TYPE_PRM_MASK  ((1ULL << HDAG_TYPE_PRM_BITS) - 1)

/**
 * Check a parameter is valid.
 *
 * @param prm   The parameter to check.
 *
 * @return True if the parameter is valid.
 */
static inline bool
hdag_type_prm_is_valid(uint64_t prm)
{
    return prm <= HDAG_TYPE_PRM_MASK;
}

/**
 * Validate a parameter.
 *
 * @param prm   The parameter to validate.
 *
 * @return The validated parameter.
 */
static inline uint64_t
hdag_type_prm_validate(uint64_t prm)
{
    assert(hdag_type_prm_is_valid(prm));
    return prm;
}

/**
 * A (composite) type definition layer.
 *
 * The lowest HDAG_TYPE_ID_BITS specify the type ID (enum hdag_type_id),
 * followed by HDAG_TYPE_PRM_BITS of the type-specific parameter, and
 * HDAG_TYPE_REP_BITS specifying the number of elements in the array.
 * One thus signifying a single instance (not really an array).
 */
typedef uint64_t hdag_type_layer;

/** Number of bits in a type definition layer */
#define HDAG_TYPE_LAYER_BITS    (sizeof(hdag_type_layer) * 8)

/** The first bit of the type ID in a type layer */
#define HDAG_TYPE_LAYER_ID_LSB  0

/** The first bit of the type parameter in a type layer */
#define HDAG_TYPE_LAYER_PRM_LSB  HDAG_TYPE_ID_BITS

/** The first bit of the repetition count in a type layer */
#define HDAG_TYPE_LAYER_REP_LSB  (HDAG_TYPE_ID_BITS + HDAG_TYPE_PRM_BITS)

/*
 * Get the type ID from a type definition layer via a constant expression
 * (without validation).
 */
#define HDAG_TYPE_LAYER_GET_ID(_layer) \
     (((_layer) >> HDAG_TYPE_LAYER_ID_LSB) & HDAG_TYPE_ID_MASK)

/**
 * Get the parameter from a type definition layer via a constant
 * expression (without validation).
 */
#define HDAG_TYPE_LAYER_GET_PRM(_layer) \
    (((_layer) >> HDAG_TYPE_LAYER_PRM_LSB) & HDAG_TYPE_PRM_MASK)

/**
 * Get the repetition count from a type definition layer via a constant
 * expression (without validation).
 */
#define HDAG_TYPE_LAYER_GET_REP(_layer) \
    (((_layer) >> HDAG_TYPE_LAYER_REP_LSB) & HDAG_TYPE_REP_MASK)

/**
 * Create a type layer via a constant expression.
 *
 * @param _id   The type ID (enum hdag_type_id).
 * @param _prm  The type parameter.
 * @param _rep  The repetition count (minimum one).
 */
#define HDAG_TYPE_LAYER(_id, _prm, _rep) ( \
    (((_id) & HDAG_TYPE_ID_MASK) << HDAG_TYPE_LAYER_ID_LSB) |       \
    (((_prm) & HDAG_TYPE_PRM_MASK) << HDAG_TYPE_LAYER_PRM_LSB) |    \
    (((_rep) & HDAG_TYPE_REP_MASK) << HDAG_TYPE_LAYER_REP_LSB)      \
)

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
typedef __uint128_t hdag_type;

/**
 * Define a type via a constant expression (without validation).
 *
 * @param _id   The ID of the type to define.
 * @param _prm  The parameter to specify for the type.
 * @param _rep  The number of times the instance is repeated.
 */
#define HDAG_TYPE_ANY(_id, _prm, _rep)  HDAG_TYPE_LAYER(_id, _prm, _rep)

/**
 * Define a parameterless, non-aggregate (simple) type via a constant
 * expression (without validation).
 *
 * @param _id   The ID of the type to define.
 */
#define HDAG_TYPE_BASIC(_id)  HDAG_TYPE_ANY(_id, 0, 1)

/**
 * Define a non-aggregate parametrized type via a constant expression (without
 * validation).
 *
 * @param _id   The ID of the type to define.
 * @param _prm  The parameter to specify for the type.
 */
#define HDAG_TYPE_PRM(_id, _prm)  HDAG_TYPE_ANY(_id, _prm, 1)

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
 * Get the parameter of a type's first layer via a constant expression
 * (without validation).
 */
#define HDAG_TYPE_GET_PRM(_type) \
    HDAG_TYPE_LAYER_GET_PRM(HDAG_TYPE_GET_LAYER(_type))

/**
 * Get the repetition count of a type's first layer via a constant expression
 * (without validation).
 */
#define HDAG_TYPE_GET_REP(_type) \
    HDAG_TYPE_LAYER_GET_REP(HDAG_TYPE_GET_LAYER(_type))

/**
 * Define an array type via a constant expression (without validation).
 *
 * @param _id   The ID of the type to define.
 * @param _rep  The size of the defined array.
 */
#define HDAG_TYPE_ARR(_id, _rep)  HDAG_TYPE_ANY(_id, 0, _rep)

/**
 * Define of a pointer type referencing another type via a constant expression
 * (without validation).
 *
 * @param _type The type being referenced.
 */
#define HDAG_TYPE_REF(_type) ( \
    ((_type) << HDAG_TYPE_LAYER_BITS) | HDAG_TYPE_ANY(HDAG_TYPE_ID_PTR) \
)

/** Dereference a type via a constant expression (without validation) */
#define HDAG_TYPE_DEREF(_type) (_type >> HDAG_TYPE_LAYER_BITS)

/**
 * Define a pointer array type referencing another type via a constant
 * expression (without validation).
 *
 * @param _type The type being referenced.
 * @param _rep  The non-zero size of the defined array.
 */
#define HDAG_TYPE_REF_ARR(_type, _rep) ( \
    ((_type) << HDAG_TYPE_LAYER_BITS) | HDAG_TYPE_ARR(HDAG_TYPE_ID_PTR, _rep)   \
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
 * Create a type definition.
 *
 * @param id    The ID of the type to define.
 *
 * @return The created type definition.
 */
static inline hdag_type
hdag_type_any(enum hdag_type_id id, uint64_t prm, uint16_t rep)
{
    assert(hdag_type_id_is_valid(id));
    assert(hdag_type_prm_is_valid(prm));
    assert(hdag_type_rep_is_valid(rep));
    return hdag_type_validate(HDAG_TYPE_ANY(id, prm, rep));
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
#define HDAG_TYPE_VOID HDAG_TYPE_BASIC(HDAG_TYPE_ID_VOID)
/** The uint8_t type */
#define HDAG_TYPE_UINT8 HDAG_TYPE_BASIC(HDAG_TYPE_ID_UINT8)
/** The uint16_t type */
#define HDAG_TYPE_UINT16 HDAG_TYPE_BASIC(HDAG_TYPE_ID_UINT16)
/** The uint32_t type */
#define HDAG_TYPE_UINT32 HDAG_TYPE_BASIC(HDAG_TYPE_ID_UINT32)
/** The uint64_t type */
#define HDAG_TYPE_UINT64 HDAG_TYPE_BASIC(HDAG_TYPE_ID_UINT64)
/** The size_t type */
#define HDAG_TYPE_SIZE HDAG_TYPE_BASIC(HDAG_TYPE_ID_SIZE)

#endif /* _HDAG_TYPE_H */
