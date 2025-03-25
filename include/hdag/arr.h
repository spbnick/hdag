/**
 * Hash DAG - array
 */

#ifndef _HDAG_ARR_H
#define _HDAG_ARR_H

#include <hdag/misc.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>

/** An array */
struct hdag_arr {
    /**
     * The size of each array element slot.
     * If zero, the array is considered "void".
     * A void array cannot be changed and its elements cannot be accessed
     * (without assertions NULL pointers will be returned).
     * However, allocating/appending zero elements will return NULL as usual,
     * and removing zero elements, emptying and cleanup will succeed.
     */
    size_t slot_size;
    /** The memory slots allocated for the elements */
    void *slots;
    /**
     * "Constant" flag in the topmost bit.
     * If set, the array slot values cannot be changed.
     * Otherwise the array is considered "variable".
     *
     * "Static" flag in the following bit.
     * If set, the array occupied size cannot be changed.
     * Otherwise the array is considered "dynamic".
     *
     * "Pinned" flag in the following bit.
     * If set, the array cannot be reallocated.
     * Otherwise the array is considered "movable".
     *
     * The array is considered "immutable", if all flag bits are set,
     * and "mutable", if all flag bits are unset.
     *
     * Number of slots to preallocate initially in the remaining bits.
     * Only has meaning if "pinned" flag is not set (the array is "movable").
     */
    size_t flags_slots_preallocate;
    /** Number of allocated element slots */
    size_t slots_allocated;
    /** Number of occupied element slots */
    size_t slots_occupied;
};

/** The bitmask of the "constant" flag in "flags_slots_preallocate" */
#define HDAG_ARR_FSP_CONSTANT_MASK \
    ((size_t)1 << (sizeof(size_t) * 8 - 1))
/** The bitmask of the "static" flag in "flags_slots_preallocate" */
#define HDAG_ARR_FSP_STATIC_MASK \
    (HDAG_ARR_FSP_CONSTANT_MASK >> 1)
/** The bitmask of the "pinned" flag in "flags_slots_preallocate" */
#define HDAG_ARR_FSP_PINNED_MASK \
    (HDAG_ARR_FSP_STATIC_MASK >> 1)

/** The bitmask of the flags in "flags_slots_preallocate" */
#define HDAG_ARR_FSP_FLAGS_MASK \
    (HDAG_ARR_FSP_CONSTANT_MASK |  \
     HDAG_ARR_FSP_STATIC_MASK |    \
     HDAG_ARR_FSP_PINNED_MASK)

/**
 * The bitmask of the flags representing the completely "immutable" state in
 * "flags_slots_preallocate", when all its bits are set.
 */
#define HDAG_ARR_FSP_IMMUTABLE_MASK HDAG_ARR_FSP_FLAGS_MASK

/** The bitmask of the preallocate slots value in "flags_slots_preallocate" */
#define HDAG_ARR_FSP_SLOTS_PREALLOCATE_MASK (~HDAG_ARR_FSP_FLAGS_MASK)

/**
 * Check if a slots preallocate value is valid for an array.
 *
 * @param slots_preallocate The value to check.
 *
 * @return True if the value is valid, false otherwise.
 */
static inline bool
hdag_arr_slots_preallocate_is_valid(size_t slots_preallocate)
{
    return slots_preallocate <= HDAG_ARR_FSP_SLOTS_PREALLOCATE_MASK;
}

/**
 * Validate a slots preallocate value for an array.
 *
 * @param slots_preallocate The value to validate.
 *
 * @return The validated value.
 */
static inline size_t
hdag_arr_slots_preallocate_validate(size_t slots_preallocate)
{
    assert(hdag_arr_slots_preallocate_is_valid(slots_preallocate));
    return slots_preallocate;
}

/**
 * An initializer for an empty array.
 *
 * @param _slot_size            The size of each element slot.
 *                              The array is considered "void", if zero.
 * @param _slots_preallocate    Number of slots to preallocate.
 */
#define HDAG_ARR_EMPTY(_slot_size, _slots_preallocate) (struct hdag_arr){ \
    .slot_size = (_slot_size),                                              \
    .flags_slots_preallocate =                                              \
        hdag_arr_slots_preallocate_validate(_slots_preallocate),            \
}

/** An initializer for a void array */
#define HDAG_ARR_VOID  HDAG_ARR_EMPTY(0, 0)

/**
 * Check if an array is valid.
 *
 * @param arr   The array to check.
 *
 * @return True if the array is valid, false otherwise.
 */
static inline bool
hdag_arr_is_valid(const struct hdag_arr *arr)
{
    return arr != NULL &&
        arr->slots_occupied <= arr->slots_allocated &&
        (arr->slots != NULL ||
         arr->slots_allocated == 0 ||
         (arr->slots_allocated == SIZE_MAX && arr->slots_occupied == 0) ||
         arr->slot_size == 0);
}

/**
 * Validate an array.
 *
 * @param arr   The array to validate
 *
 * @return The validated array.
 */
static inline struct hdag_arr *
hdag_arr_validate(struct hdag_arr *arr)
{
    assert(hdag_arr_is_valid(arr));
    return arr;
}

/**
 * Check if array slice parameters are valid.
 *
 * @param arr   The array containing the slice to check.
 * @param start The start index of the slice.
 * @param end   The end index of the slice (pointing right after the last
 *              element of the slice).
 *              Must be greater than or equal to start, and less than
 *              or equal to arr->slots_occupied.
 *
 * @return True if the array slice is valid, false otherwise.
 */
static inline bool
hdag_arr_slice_is_valid(const struct hdag_arr *arr,
                        size_t start, size_t end)
{
    return
        hdag_arr_is_valid(arr) &&
        start <= end &&
        end <= arr->slots_occupied;
}

/**
 * Validate the parameters of an array slice, return the array.
 *
 * @param arr   The array containing the slice to check.
 * @param start The start index of the slice.
 * @param end   The end index of the slice (pointing right after the last
 *              element of the slice).
 *              Must be greater than or equal to start, and less than
 *              or equal to arr->slots_occupied.
 *
 * @return True if the array slice is valid, false otherwise.
 */
static inline const struct hdag_arr *
hdag_arr_slice_validate(const struct hdag_arr *arr,
                        size_t start, size_t end)
{
    assert(hdag_arr_slice_is_valid(arr, start, end));
    return arr;
}

/**
 * An initializer for a pinned (non- movable/reallocatable) array.
 *
 * @param _slots            The pointer to the array slots.
 * @param _slot_size        The size of each element slot.
 *                          The array is considered "void", if zero.
 * @param _slots_occupied   Number of occupied slots (number of elements).
 * @param _slots_allocated  Number of available slots (must be greater than or
 *                          equal to _slots_occupied).
 */
#define HDAG_ARR_PINNED(_slots, _slot_size, \
                        _slots_occupied, _slots_allocated)      \
    (struct hdag_arr){                                          \
        .slots = (_slots),                                      \
        .slot_size = (_slot_size),                              \
        .flags_slots_preallocate = HDAG_ARR_FSP_PINNED_MASK,    \
        .slots_occupied = (_slots_occupied),                    \
        .slots_allocated = (_slots_allocated),                  \
    }

/**
 * An initializer for a static (non-resizeable) and pinned (non-
 * movable/reallocatable), but variable (changeable-element) array.
 *
 * @param _slots            The pointer to the array slots.
 * @param _slot_size        The size of each element slot.
 *                          The array is considered "void", if zero.
 * @param _slots_occupied   Number of occupied slots (number of elements).
 */
#define HDAG_ARR_PINNED_STATIC(_slots, _slot_size, _slots_occupied) \
    (struct hdag_arr){                                              \
        .slots = (_slots),                                          \
        .slot_size = (_slot_size),                                  \
        .flags_slots_preallocate =                                  \
            HDAG_ARR_FSP_STATIC_MASK | HDAG_ARR_FSP_PINNED_MASK,    \
        .slots_occupied = (_slots_occupied),                        \
        .slots_allocated = (_slots_occupied),                       \
    }

/**
 * An initializer for an immutable array.
 *
 * @param _slots            The pointer to the array slots.
 * @param _slot_size        The size of each element slot.
 *                          The array is considered "void", if zero.
 * @param _slots_occupied   Number of occupied slots (number of elements).
 */
#define HDAG_ARR_IMMUTABLE(_slots, _slot_size, _slots_occupied) \
    (struct hdag_arr){                                          \
        .slots = (_slots),                                      \
        .slot_size = (_slot_size),                              \
        .flags_slots_preallocate = HDAG_ARR_FSP_IMMUTABLE_MASK, \
        .slots_occupied = (_slots_occupied),                    \
        .slots_allocated = (_slots_occupied),                   \
    }

/**
 * Check if an array is completely immutable, that is if both its
 * occupied size and elements cannot be changed.
 *
 * @param arr   The array to check.
 *
 * @return True if the array is immutable, false otherwise.
 */
static inline bool
hdag_arr_is_immutable(const struct hdag_arr *arr)
{
    assert(hdag_arr_is_valid(arr));
    return (arr->flags_slots_preallocate & HDAG_ARR_FSP_FLAGS_MASK) ==
            HDAG_ARR_FSP_FLAGS_MASK;
}

/**
 * Check if an array is completely mutable, that is if its elements,
 * occupied, and allocated sizes can be changed.
 *
 * @param arr   The array to check.
 *
 * @return True if the array is mutable, false otherwise.
 */
static inline bool
hdag_arr_is_mutable(const struct hdag_arr *arr)
{
    assert(hdag_arr_is_valid(arr));
    return (arr->flags_slots_preallocate & HDAG_ARR_FSP_FLAGS_MASK) == 0;
}

/**
 * Check if an array is "static", that is if its occupied size cannot
 * be changed.
 *
 * @param arr   The array to check.
 *
 * @return True if the array is static, false otherwise.
 */
static inline bool
hdag_arr_is_static(const struct hdag_arr *arr)
{
    assert(hdag_arr_is_valid(arr));
    return (arr->flags_slots_preallocate & HDAG_ARR_FSP_STATIC_MASK) != 0;
}

/**
 * Check if an array is actually "dynamic" (not "static"), that is if
 * its occupied size can be changed.
 *
 * @param arr   The array to check.
 *
 * @return True if the array is actually "dynamic", false otherwise.
 */
static inline bool
hdag_arr_is_dynamic(const struct hdag_arr *arr)
{
    assert(hdag_arr_is_valid(arr));
    return (arr->flags_slots_preallocate & HDAG_ARR_FSP_STATIC_MASK) == 0;
}

/**
 * Check if an array is "constant", that is if its elements cannot be
 * changed.
 *
 * @param arr   The variable array to check.
 *
 * @return True if the array is constant, false otherwise.
 */
static inline bool
hdag_arr_is_constant(const struct hdag_arr *arr)
{
    assert(hdag_arr_is_valid(arr));
    return (arr->flags_slots_preallocate & HDAG_ARR_FSP_CONSTANT_MASK) != 0;
}

/**
 * Check if an array is "variable", that is if its elements can be
 * changed.
 *
 * @param arr   The array to check.
 *
 * @return True if the array is "variable", false otherwise.
 */
static inline bool
hdag_arr_is_variable(const struct hdag_arr *arr)
{
    assert(hdag_arr_is_valid(arr));
    return (arr->flags_slots_preallocate & HDAG_ARR_FSP_CONSTANT_MASK) == 0;
}

/**
 * Check if an array is "pinned", that is if its allocated size cannot be
 * changed.
 *
 * @param arr   The array to check.
 *
 * @return True if the array is pinned, false otherwise.
 */
static inline bool
hdag_arr_is_pinned(const struct hdag_arr *arr)
{
    assert(hdag_arr_is_valid(arr));
    return (arr->flags_slots_preallocate & HDAG_ARR_FSP_PINNED_MASK) != 0;
}

/**
 * Check if an array is "movable", that is if its allocated size can be
 * changed.
 *
 * @param arr   The array to check.
 *
 * @return True if the array is movable, false otherwise.
 */
static inline bool
hdag_arr_is_movable(const struct hdag_arr *arr)
{
    assert(hdag_arr_is_valid(arr));
    return (arr->flags_slots_preallocate & HDAG_ARR_FSP_PINNED_MASK) == 0;
}

/**
 * Check if an array is void (has slot size zero), cannot be
 * accessed or modified, and always has size zero.
 *
 * @param arr   The array to check.
 *
 * @return True if the array is void, false otherwise.
 */
static inline bool
hdag_arr_is_void(const struct hdag_arr *arr)
{
    assert(hdag_arr_is_valid(arr));
    return arr->slot_size == 0;
}

/**
 * Get the number of occupied slots.
 *
 * @param arr   The array to get the number of occupied slots for.
 *
 * @return The number of occupied slots, bytes.
 */
static inline size_t
hdag_arr_slots_occupied(const struct hdag_arr *arr)
{
    assert(hdag_arr_is_valid(arr));
    return arr->slots_occupied;
}

/**
 * Get the size of occupied slots, in bytes.
 *
 * @param arr   The array to get the size of occupied slots.
 *
 * @return The size of occupied slots, bytes.
 */
static inline size_t
hdag_arr_size_occupied(const struct hdag_arr *arr)
{
    assert(hdag_arr_is_valid(arr));
    return arr->slot_size * arr->slots_occupied;
}

/**
 * Get the number of allocated (occupied, if immutable) slots.
 *
 * @param arr   The array to get the number of allocated slots for.
 *
 * @return The number of allocated (occupied, if immutable) slots.
 */
static inline size_t
hdag_arr_slots_allocated(const struct hdag_arr *arr)
{
    assert(hdag_arr_is_valid(arr));
    return arr->slots_allocated;
}

/**
 * Get the size of allocated (occupied, if immutable) slots, in bytes.
 *
 * @param arr   The array to get the size of allocated slots.
 *
 * @return The size of allocated (occupied, if immutable) slots, bytes.
 */
static inline size_t
hdag_arr_size_allocated(const struct hdag_arr *arr)
{
    assert(hdag_arr_is_valid(arr));
    return arr->slot_size * hdag_arr_slots_allocated(arr);
}

/**
 * Retrieve the const pointer to a const array element slot at
 * specified index.
 *
 * @param arr   The array to retrieve the element pointer from.
 *              Cannot be void.
 * @param idx   The index of the element slot to retrieve the pointer to.
 *              Must be within allocated area, but not necessarily within
 *              occupied area.
 *
 * @return The element slot pointer.
 */
static inline const void *
hdag_arr_slot_const(const struct hdag_arr *arr, size_t idx)
{
    assert(hdag_arr_is_valid(arr));
    assert(!hdag_arr_is_void(arr));
    assert(idx <= arr->slots_allocated);
    return (char *)arr->slots + arr->slot_size * idx;
}

/**
 * Retrieve the const pointer to the array slot right after the last
 * element.
 *
 * @param arr   The array to retrieve the last slot pointer from.
 *              Cannot be void.
 *
 * @return The end slot pointer.
 */
static inline const void *
hdag_arr_end_slot_const(const struct hdag_arr *arr)
{
    assert(hdag_arr_is_valid(arr));
    assert(!hdag_arr_is_void(arr));
    if (hdag_arr_is_void(arr)) {
        return NULL;
    }
    return (const char *)arr->slots + arr->slot_size * arr->slots_occupied;
}

/**
 * Retrieve the pointer to a variable element slot of an array at
 * specified index.
 *
 * @param arr   The array to retrieve the element pointer from.
 *              Cannot be void. Must be variable.
 * @param idx   The index of the element slot to retrieve the pointer to.
 *              Must be within allocated area, but not necessarily within
 *              occupied area.
 *
 * @return The element slot pointer.
 */
static inline void *
hdag_arr_slot(const struct hdag_arr *arr, size_t idx)
{
    assert(hdag_arr_is_valid(arr));
    assert(!hdag_arr_is_void(arr));
    assert(idx <= arr->slots_allocated);
    assert(hdag_arr_is_variable(arr));
    return (char *)arr->slots + arr->slot_size * idx;
}

/**
 * Retrieve the pointer to a variable slot of an array at specified index,
 * asserting that the size of the slot matches the specified size.
 *
 * @param arr   The array to retrieve the slot pointer from.
 *              Cannot be void. Must be variable.
 * @param size  The expected size of the slot to assert.
 * @param idx   The index of the slot to retrieve the pointer to.
 *              Must be within allocated area, but not necessarily within
 *              occupied area.
 *
 * @return The slot pointer.
 */
static inline void *
hdag_arr_slot_sized(const struct hdag_arr *arr, size_t size, size_t idx)
{
    assert(hdag_arr_is_valid(arr));
    assert(!hdag_arr_is_void(arr));
    assert(size == arr->slot_size);
    assert(idx <= arr->slots_allocated);
    assert(hdag_arr_is_variable(arr));
    (void)size;
    return (char *)arr->slots + arr->slot_size * idx;
}

/**
 * Retrieve the const pointer to an slot at specified index inside a const
 * array, asserting that the size of the slot matches the specified size.
 *
 * @param arr   The array to retrieve the slot pointer from.
 *              Cannot be void.
 * @param size  The expected size of the slot to assert.
 * @param idx   The index of the slot to retrieve the pointer to.
 *              Must be within allocated area, but not necessarily within
 *              occupied area.
 *
 * @return The slot pointer.
 */
static inline const void *
hdag_arr_slot_sized_const(const struct hdag_arr *arr, size_t size, size_t idx)
{
    assert(hdag_arr_is_valid(arr));
    assert(!hdag_arr_is_void(arr));
    assert(size == arr->slot_size);
    assert(idx <= arr->slots_allocated);
    (void)size;
    return (const char *)arr->slots + arr->slot_size * idx;
}

/**
 * Retrieve the pointer to a variable element of an array at specified
 * index.
 *
 * @param arr   The array to retrieve the element pointer from.
 *              Cannot be void. Must be variable.
 * @param idx   The index of the element to retrieve the pointer to.
 *              Must be within occupied area.
 *
 * @return The element pointer.
 */
static inline void *
hdag_arr_element(const struct hdag_arr *arr, size_t idx)
{
    assert(hdag_arr_is_valid(arr));
    assert(!hdag_arr_is_void(arr));
    assert(idx < arr->slots_occupied);
    assert(hdag_arr_is_variable(arr));
    return (char *)arr->slots + arr->slot_size * idx;
}

/**
 * Retrieve the pointer to a variable element of an array at specified
 * index, asserting that the size of the element matches the specified size.
 *
 * @param arr   The array to retrieve the element pointer from.
 *              Cannot be void.
 * @param size  The expected size of the element to assert.
 * @param idx   The index of the element to retrieve the pointer to.
 *              Must be within occupied area.
 *
 * @return The element pointer.
 */
static inline void *
hdag_arr_element_sized(const struct hdag_arr *arr, size_t size, size_t idx)
{
    assert(hdag_arr_is_valid(arr));
    assert(!hdag_arr_is_void(arr));
    assert(size == arr->slot_size);
    assert(idx < arr->slots_occupied);
    assert(hdag_arr_is_variable(arr));
    (void)size;
    return (char *)arr->slots + arr->slot_size * idx;
}

/**
 * Retrieve the const pointer to an element at specified index inside an
 * array.
 *
 * @param arr   The array to retrieve the element pointer from.
 *              Cannot be void.
 * @param idx   The index of the element to retrieve the pointer to.
 *              Must be within occupied area.
 *
 * @return The element pointer.
 */
static inline const void *
hdag_arr_element_const(const struct hdag_arr *arr, size_t idx)
{
    assert(hdag_arr_is_valid(arr));
    assert(!hdag_arr_is_void(arr));
    assert(idx < arr->slots_occupied);
    return (const char *)arr->slots + arr->slot_size * idx;
}

/**
 * Retrieve the const pointer to an element at specified index inside a const
 * array, asserting that the size of the element matches the specified
 * size.
 *
 * @param arr   The array to retrieve the element pointer from.
 *              Cannot be void.
 * @param size  The expected size of the element to assert.
 * @param idx   The index of the element to retrieve the pointer to.
 *              Must be within occupied area.
 *
 * @return The element pointer.
 */
static inline const void *
hdag_arr_element_sized_const(const struct hdag_arr *arr,
                             size_t size, size_t idx)
{
    assert(hdag_arr_is_valid(arr));
    assert(!hdag_arr_is_void(arr));
    assert(size == arr->slot_size);
    assert(idx < arr->slots_occupied);
    (void)size;
    return (const char *)arr->slots + arr->slot_size * idx;
}

/**
 * Retrieve the pointer to a slot of specified type, at specified index inside
 * an array. Assert that the size of the slot type matches the array's slot
 * size. Cast the returned pointer to a pointer to the specified type.
 *
 * @param _arr  The array to retrieve the slot pointer from.
 *              Cannot be void. Must be variable.
 * @param _type The type of the array slot (not a pointer to it) to assert
 *              the size of, and to cast the returned type to (making it a
 *              pointer).
 * @param _idx  The index of the slot to retrieve the pointer to.
 *              Must be within allocated area.
 *
 * @return The slot pointer with specified type.
 */
#define HDAG_ARR_SLOT(_arr, _type, _idx) \
    ((_type *)hdag_arr_slot_sized(_arr, sizeof(_type), _idx))

/**
 * Retrieve the pointer to a slot of specified type, at specified index inside
 * an array. Do *not* assert that the size of the specified slot type matches
 * the array's slot size, catering e.g. to the cases when each slot is in
 * itself an array. Cast the returned pointer to a pointer to the specified
 * type.
 *
 * @param _arr  The array to retrieve the slot pointer from.
 *              Cannot be void. Must be variable.
 * @param _type The type of the array slot (not a pointer to it) to cast
 *              the returned type to (making it a pointer).
 * @param _idx  The index of the slot to retrieve the pointer to.
 *              Must be within allocated area.
 *
 * @return The slot pointer with specified type.
 */
#define HDAG_ARR_SLOT_UNSIZED(_arr, _type, _idx) \
    ((_type *)hdag_arr_slot(_arr, _idx))

/**
 * Retrieve the const pointer to a slot of specified type, at specified index
 * inside an array. Assert that the size of the slot type matches the array's
 * slot size. Cast the returned pointer to a pointer to the specified type,
 * with "const" added.
 *
 * @param _arr  The array to retrieve the slot pointer from.
 *              Cannot be void.
 * @param _type The type of the array slot (not a pointer to it) to assert
 *              the size of, and to cast the returned type to (making it a
 *              pointer). Must not contain "const".
 * @param _idx  The index of the slot to retrieve the pointer to.
 *              Must be within allocated area.
 *
 * @return The slot pointer with specified type and "const" added.
 */
#define HDAG_ARR_SLOT_CONST(_arr, _type, _idx) \
    ((const _type *)hdag_arr_slot_sized_const(_arr, sizeof(_type), _idx))

/**
 * Retrieve the const pointer to a slot of specified type, at specified index
 * inside an array. Do *not* assert the size of the specified slot type of the
 * slot matches the array's slot size, catering e.g. to the cases when each
 * slot is in itself an array. Cast the returned pointer to a pointer to the
 * specified type, with "const" added.
 *
 * @param _arr  The array to retrieve the slot pointer from.
 *              Cannot be void.
 * @param _type The type of the array slot (not a pointer to it) to cast
 *              the returned type to (making it a pointer).
 * @param _idx  The index of the slot to retrieve the pointer to.
 *              Must be within allocated area.
 *
 * @return The slot pointer with specified type and "const" added.
 */
#define HDAG_ARR_SLOT_UNSIZED_CONST(_arr, _type, _idx) \
    ((const _type *)hdag_arr_slot_const(_arr, _idx))

/**
 * Retrieve the pointer to an element of specified type, at specified index
 * inside an array. Assert that the size of the element type matches the
 * array's slot size. Cast the returned pointer to a pointer to the specified
 * type.
 *
 * @param _arr  The array to retrieve the element pointer from.
 *              Cannot be void. Must be variable.
 * @param _type The type of the array element (not a pointer to it) to assert
 *              the size of, and to cast the returned type to (making it a
 *              pointer).
 * @param _idx  The index of the element to retrieve the pointer to.
 *              Must be within occupied area.
 *
 * @return The element pointer with specified type.
 */
#define HDAG_ARR_ELEMENT(_arr, _type, _idx) \
    ((_type *)hdag_arr_element_sized(_arr, sizeof(_type), _idx))

/**
 * Retrieve the pointer to an element of specified type, at specified index
 * inside an array. Do *not* assert that the size of the specified element
 * type the array's slot size, catering e.g. to the cases when each element is
 * in itself an array. Cast the returned pointer to a pointer to the specified
 * type.
 *
 * @param _arr  The array to retrieve the element pointer from.
 *              Cannot be void. Must be variable.
 * @param _type The type of the array element (not a pointer to it) to cast
 *              the returned type to (making it a pointer).
 * @param _idx  The index of the element to retrieve the pointer to.
 *              Must be within occupied area.
 *
 * @return The element pointer with specified type.
 */
#define HDAG_ARR_ELEMENT_UNSIZED(_arr, _type, _idx) \
    ((_type *)hdag_arr_element(_arr, _idx))

/**
 * Retrieve the const pointer to an element of specified type, at specified
 * index inside an array. Assert that the size of the element type matches the
 * array's slot size. Cast the returned pointer to a pointer to the specified
 * type, with "const" added.
 *
 * @param _arr  The array to retrieve the element pointer from.
 *              Cannot be void.
 * @param _type The type of the array element (not a pointer to it) to assert
 *              the size of, and to cast the returned type to (making it a
 *              pointer). Must not contain "const".
 * @param _idx  The index of the element to retrieve the pointer to.
 *              Must be within occupied area.
 *
 * @return The element pointer with specified type and "const" added.
 */
#define HDAG_ARR_ELEMENT_CONST(_arr, _type, _idx) \
    ((const _type *)hdag_arr_element_sized_const(_arr, sizeof(_type), _idx))

/**
 * Retrieve the const pointer to an element of specified type, at specified
 * index inside an array. Do *not* assert that the size of the specified
 * element type matches the array's slot size, catering e.g. to the cases when
 * each element is in itself an array. Cast the returned pointer to a pointer
 * to the specified type, with "const" added.
 *
 * @param _arr  The array to retrieve the element pointer from.
 *              Cannot be void.
 * @param _type The type of the array element (not a pointer to it) to cast
 *              the returned type to (making it a pointer).
 * @param _idx  The index of the element to retrieve the pointer to.
 *              Must be within occupied area.
 *
 * @return The slot pointer with specified type and "const" added.
 */
#define HDAG_ARR_ELEMENT_UNSIZED_CONST(_arr, _type, _idx) \
    ((const _type *)hdag_arr_element_const(_arr, _idx))

/**
 * Make sure an array has slots allocated for specified number of
 * extra elements.
 *
 * @param arr   The array to allocate slots in.
 *              Cannot be void unless num is zero.
 * @param num   The number of elements to allocate slots for.
 *
 * @return The pointer to the first allocated element slot in the array, if
 *         succeeded. NULL, if "num" was zero, the array was "void", or
 *         allocation failed (in which case errno is set).
 */
[[nodiscard]]
extern void *hdag_arr_alloc(struct hdag_arr *arr, size_t num);

/**
 * Make sure an array has slots allocated and zeroed for specified
 * number of extra elements.
 *
 * @param arr   The array to allocate and zero slots in.
 *              Cannot be void unless num is zero.
 * @param num   The number of elements to allocate slots for.
 *
 * @return The pointer to the first allocated element slot in the array, if
 *         succeeded. NULL, if "num" was zero, the array was "void", or
 *         allocation failed (in which case errno is set).
 */
[[nodiscard]]
static inline void *
hdag_arr_calloc(struct hdag_arr *arr, size_t num)
{
    assert(hdag_arr_is_valid(arr));
    assert(num == 0 || hdag_arr_is_movable(arr));
    void *slots = hdag_arr_alloc(arr, num);
    if (slots != NULL) {
        memset(slots, 0, arr->slot_size * num);
    }
    return slots;
}

/**
 * Make sure an array has a slot allocated for one more element.
 *
 * @param arr   The array to allocate the slot in. Cannot be void.
 *
 * @return The pointer to the allocated element slot in the array, if
 *         succeeded. NULL, if the array was "void", or allocation failed (in
 *         which case errno is set).
 */
[[nodiscard]]
static inline void *
hdag_arr_alloc_one(struct hdag_arr *arr)
{
    assert(hdag_arr_is_valid(arr));
    assert(hdag_arr_is_movable(arr));
    assert(!hdag_arr_is_void(arr));
    return hdag_arr_alloc(arr, 1);
}

/**
 * Make sure an array has a slot allocated and zeroed for one more
 * element.
 *
 * @param arr   The array to allocate and zero the slot in.
 *              Cannot be void.
 *
 * @return The pointer to the allocated element slot in the array, if
 *         succeeded. NULL, if the array was "void", or allocation failed (in
 *         which case errno is set).
 */
[[nodiscard]]
static inline void *
hdag_arr_calloc_one(struct hdag_arr *arr)
{
    assert(hdag_arr_is_valid(arr));
    assert(hdag_arr_is_movable(arr));
    assert(!hdag_arr_is_void(arr));
    return hdag_arr_calloc(arr, 1);
}

/**
 * Insert an unitialized slice of elements into an array.
 *
 * @param arr       The array to insert the slice into.
 *                  Cannot be void unless num == 0.
 * @param start     The index of the slot to insert the slice at.
 * @param num       The number of elements to insert.
 *
 * @return The pointer to the first inserted element in the array, if
 *         succeeded. NULL, if "num" was zero, the array was "void", or
 *         allocation failed (in which case errno is set).
 */
[[nodiscard]]
static inline void *
hdag_arr_uinsert(struct hdag_arr *arr, size_t start, size_t num)
{
    assert(hdag_arr_is_valid(arr));
    assert(num == 0 || hdag_arr_is_dynamic(arr));
    assert(num == 0 || hdag_arr_is_variable(arr));
    assert(start <= arr->slots_occupied);
    size_t new_slots_occupied = arr->slots_occupied + num;
    if (num == 0 || hdag_arr_alloc(arr, new_slots_occupied) == NULL) {
        return NULL;
    }
    void *start_slot = hdag_arr_slot(arr, start);
    assert(!hdag_arr_is_void(arr));
    memmove(start_slot,
            hdag_arr_slot(arr, start + num),
            (arr->slots_occupied - start) * arr->slot_size);
    arr->slots_occupied = new_slots_occupied;
    return start_slot;
}

/**
 * Insert a slice of zeroed elements into an array.
 *
 * @param arr       The array to insert the slice into.
 *                  Cannot be void unless num == 0.
 * @param start     The index of the slot to insert the slice at.
 * @param num       The number of elements from the array to insert.
 *
 * @return The pointer to the first inserted element in the array, if
 *         succeeded. NULL, if "num" was zero, the array was "void", or
 *         allocation failed (in which case errno is set).
 */
[[nodiscard]]
static inline void *
hdag_arr_cinsert(struct hdag_arr *arr, size_t start, size_t num)
{
    assert(hdag_arr_is_valid(arr));
    assert(num == 0 || hdag_arr_is_dynamic(arr));
    assert(num == 0 || hdag_arr_is_variable(arr));
    assert(start <= arr->slots_occupied);
    void *start_slot = hdag_arr_uinsert(arr, start, num);
    if (start_slot != NULL) {
        memset(start_slot, 0, num * arr->slot_size);
    }
    return start_slot;
}

/**
 * Insert a slice of elements into an array.
 *
 * @param arr       The array to insert the slice into.
 *                  Cannot be void unless num == 0.
 * @param start     The index of the slot to insert the slice at.
 * @param elements  The array containing elements to insert.
 * @param num       The number of elements from the array to insert.
 *
 * @return The pointer to the first inserted element in the array, if
 *         succeeded. NULL, if "num" was zero, the array was "void", or
 *         allocation failed (in which case errno is set).
 */
[[nodiscard]]
static inline void *
hdag_arr_insert(struct hdag_arr *arr, size_t start,
                const void *elements, size_t num)
{
    assert(hdag_arr_is_valid(arr));
    assert(num == 0 || hdag_arr_is_dynamic(arr));
    assert(num == 0 || hdag_arr_is_variable(arr));
    assert(start <= arr->slots_occupied);
    assert(elements != NULL || num == 0);
    void *start_slot = hdag_arr_uinsert(arr, start, num);
    if (start_slot != NULL) {
        memcpy(start_slot, elements, num * arr->slot_size);
    }
    return start_slot;
}

/**
 * Insert one element into an array.
 *
 * @param arr       The array to insert the element into.
 *                  Cannot be void.
 * @param idx       The index of the slot to insert the element at.
 * @param element   The pointer to the element to insert.
 *
 * @return The pointer to the inserted element in the array, if succeeded.
 *         NULL, if allocation failed (in which case errno is set).
 */
[[nodiscard]]
static inline void *
hdag_arr_insert_one(struct hdag_arr *arr, size_t idx, const void *element)
{
    assert(hdag_arr_is_valid(arr));
    assert(hdag_arr_is_dynamic(arr));
    assert(hdag_arr_is_variable(arr));
    assert(idx <= arr->slots_occupied);
    assert(element != NULL);
    return hdag_arr_insert(arr, idx, element, 1);
}

/**
 * Append the specified number of *uninitialized* elements to an array.
 *
 * @param arr   The array to append elements to.
 *              Cannot be void unless num is zero.
 * @param num   The number of elements to append.
 *
 * @return The pointer to the first appended element in the array, if
 *         succeeded. NULL, if "num" was zero, the array was "void", or
 *         allocation failed (in which case errno is set).
 */
[[nodiscard]]
static inline void *
hdag_arr_uappend(struct hdag_arr *arr, size_t num)
{
    assert(hdag_arr_is_valid(arr));
    assert(num == 0 || hdag_arr_is_dynamic(arr));
    assert(num == 0 || hdag_arr_is_variable(arr));
    return hdag_arr_uinsert(arr, arr->slots_occupied, num);
}

/**
 * Append the specified number of elements to an array.
 *
 * @param arr       The array to append elements to.
 *                  Cannot be void unless num is zero.
 * @param elements  The array containing elements to append.
 * @param num       The number of elements from the array to append.
 *
 * @return The pointer to the first appended element in the array, if
 *         succeeded. NULL, if "num" was zero, the array was "void", or
 *         allocation failed (in which case errno is set).
 */
[[nodiscard]]
static void *
hdag_arr_append(struct hdag_arr *arr, const void *elements, size_t num)
{
    assert(hdag_arr_is_valid(arr));
    assert(num == 0 || hdag_arr_is_dynamic(arr));
    assert(num == 0 || hdag_arr_is_variable(arr));
    assert(elements != NULL || num == 0);
    return hdag_arr_insert(arr, arr->slots_occupied, elements, num);
}

/**
 * Append one element to an array.
 *
 * @param arr       The array to append elements to. Cannot be void.
 * @param element   The pointer to the element to append.
 *
 * @return The pointer to the appended element in the array, if succeeded.
 *         NULL, if the array was "void", or allocation failed (in which case
 *         errno is set).
 */
[[nodiscard]]
static inline void *
hdag_arr_append_one(struct hdag_arr *arr, const void *element)
{
    assert(hdag_arr_is_valid(arr));
    assert(hdag_arr_is_dynamic(arr));
    assert(hdag_arr_is_variable(arr));
    assert(!hdag_arr_is_void(arr));
    return hdag_arr_append(arr, element, 1);
}

/**
 * Append the specified number of zero-filled elements to an array.
 *
 * @param arr       The array to append elements to.
 *                  Cannot be void unless num is zero.
 * @param num       The number of zeroed elements to append.
 *
 * @return The pointer to the first appended element in the array, if
 *         succeeded. NULL, if "num" was zero, the array was "void", or
 *         allocation failed (in which case errno is set).
 */
[[nodiscard]]
static inline void *
hdag_arr_cappend(struct hdag_arr *arr, size_t num)
{
    assert(hdag_arr_is_valid(arr));
    assert(num == 0 || hdag_arr_is_dynamic(arr));
    assert(num == 0 || hdag_arr_is_variable(arr));
    return hdag_arr_cinsert(arr, arr->slots_occupied, num);
}

/**
 * Append a zero-filled element to an array.
 *
 * @param arr       The array to append the zeroed element to.
 *                  Cannot be void.
 *
 * @return The pointer to the appended element in the array, if succeeded.
 *         NULL, if the array was "void", or allocation failed (in which case
 *         errno is set).
 */
[[nodiscard]]
static inline void *
hdag_arr_cappend_one(struct hdag_arr *arr)
{
    assert(hdag_arr_is_valid(arr));
    assert(hdag_arr_is_dynamic(arr));
    assert(hdag_arr_is_variable(arr));
    assert(!hdag_arr_is_void(arr));
    return hdag_arr_cappend(arr, 1);
}

/**
 * Free all empty element slots in an array ("deflate").
 *
 * @param arr   The array to deflate.
 *
 * @return True if deflating succeeded, false if memory reallocation failed
 *         (in which case errno is set).
 */
[[nodiscard]]
extern bool hdag_arr_deflate(struct hdag_arr *arr);

/**
 * Remove all elements from an array, but keep the allocated slots.
 *
 * @param arr   The array to empty.
 */
static inline void
hdag_arr_empty(struct hdag_arr *arr)
{
    assert(hdag_arr_is_valid(arr));
    assert(hdag_arr_is_dynamic(arr));
    arr->slots_occupied = 0;
}

/**
 * Check if an array is empty.
 *
 * @param arr   The array to check.
 *
 * @return True if the array has no elements, false otherwise.
 */
static inline bool
hdag_arr_is_empty(const struct hdag_arr *arr)
{
    assert(hdag_arr_is_valid(arr));
    return arr->slots_occupied == 0;
}

/**
 * Remove all element slots from an array, empty or not.
 * This deallocates all referenced memory for mutable arrays, and makes
 * immutable array mutable.
 *
 * @param arr   The array to cleanup.
 */
static inline void
hdag_arr_cleanup(struct hdag_arr *arr)
{
    assert(hdag_arr_is_valid(arr));
    if (hdag_arr_is_movable(arr)) {
        free(arr->slots);
    }
    arr->slots = NULL;
    arr->slots_occupied = 0;
    arr->slots_allocated = 0;
    assert(hdag_arr_is_valid(arr));
}

/**
 * Check if an array is clean, that is has no element slots, which
 * means it doesn't refer to any allocated memory.
 *
 * @param arr   The array to check.
 *
 * @return True if the array is clean, false otherwise.
 */
static inline bool
hdag_arr_is_clean(const struct hdag_arr *arr)
{
    assert(hdag_arr_is_valid(arr));
    return arr->slots_allocated == 0;
}

/**
 * Remove a slice of elements from an array.
 *
 * @param arr   The array to remove the slice from.
 *              Cannot be void unless start == end.
 *              Must be dynamic unless start == end.
 *              Must be variable unless "end" is at the array's end.
 * @param start The index of the first slot of the slice to remove.
 * @param end   The index of the first slot after the slice being removed.
 *              Must be equal to, or greater than "start".
 */
static inline void
hdag_arr_remove(struct hdag_arr *arr, size_t start, size_t end)
{
    assert(hdag_arr_slice_is_valid(arr, start, end));
    assert(start == end || hdag_arr_is_dynamic(arr));
    assert(end == arr->slots_occupied || hdag_arr_is_variable(arr));
    if (start != end) {
        assert(!hdag_arr_is_void(arr));
        memmove(hdag_arr_slot(arr, start), hdag_arr_slot(arr, end),
                (arr->slots_occupied - end) * arr->slot_size);
        arr->slots_occupied -= end - start;
    }
}

/**
 * Remove an element from an array.
 *
 * @param arr   The array to remove the element from.
 *              Cannot be void. Must be dynamic.
 *              Must be variable unless removing the last element.
 * @param idx   The index of the element to remove.
 */
static inline void
hdag_arr_remove_one(struct hdag_arr *arr, size_t idx)
{
    assert(hdag_arr_is_valid(arr));
    assert(hdag_arr_is_dynamic(arr));
    assert(idx < arr->slots_occupied);
    assert((idx + 1) == arr->slots_occupied || hdag_arr_is_variable(arr));
    assert(!hdag_arr_is_void(arr));
    hdag_arr_remove(arr, idx, idx + 1);
}

/**
 * Iterate the following statement over the elements of an array,
 * forward.
 *
 * @param _arr          The array to iterate over. Must be variable.
 * @param _index_name   Name of the ssize_t index variable.
 * @param _element_name Name of the element pointer variable.
 * @param _init_expr    A trailing initialization expression.
 * @param _incr_expr    A preceding increment expression.
 */
#define HDAG_ARR_IFWD(_arr, _index_name, _element_name, \
                      _init_expr, _incr_expr)                   \
    assert(hdag_arr_is_valid(_arr));                            \
    for (_index_name = 0,                                       \
         _element_name = hdag_arr_slot_sized(                   \
             _arr, sizeof(*(_element_name)), 0                  \
         ),                                                     \
         _init_expr;                                            \
         _index_name < (ssize_t)(_arr)->slots_occupied;         \
         _incr_expr,                                            \
         _index_name++,                                         \
         _element_name =                                        \
            (void *)((char *)_element_name + (_arr)->slot_size) \
    )

/**
 * Iterate the following statement over the elements of an array, forward.
 * Do *not* assert that the array element size matches the size of the type
 * pointed to by "_element_name".
 *
 * @param _arr          The array to iterate over. Must be variable.
 * @param _index_name   Name of the ssize_t index variable.
 * @param _element_name Name of the element pointer variable.
 * @param _init_expr    A trailing initialization expression.
 * @param _incr_expr    A preceding increment expression.
 */
#define HDAG_ARR_IFWD_UNSIZED(_arr, _index_name, \
                              _element_name,                    \
                              _init_expr, _incr_expr)           \
    assert(hdag_arr_is_valid(_arr));                            \
    for (_index_name = 0,                                       \
         _element_name = hdag_arr_slot(_arr, 0),                \
         _init_expr;                                            \
         _index_name < (ssize_t)(_arr)->slots_occupied;         \
         _incr_expr,                                            \
         _index_name++,                                         \
         _element_name =                                        \
            (void *)((char *)_element_name + (_arr)->slot_size) \
    )

/**
 * Iterate the following statement over constant elements of an array,
 * forward.
 *
 * @param _arr          The array to iterate over.
 * @param _index_name   Name of the ssize_t index variable.
 * @param _element_name Name of the element pointer variable.
 * @param _init_expr    A trailing initialization expression.
 * @param _incr_expr    A preceding increment expression.
 */
#define HDAG_ARR_IFWD_CONST(_arr, _index_name, _element_name, \
                            _init_expr, _incr_expr)             \
    assert(hdag_arr_is_valid(_arr));                            \
    for (_index_name = 0,                                       \
         _element_name = hdag_arr_slot_sized_const(             \
             _arr, sizeof(*(_element_name)), 0                  \
         ),                                                     \
         _init_expr;                                            \
         _index_name < (ssize_t)(_arr)->slots_occupied;         \
         _incr_expr,                                            \
         _index_name++,                                         \
         _element_name =                                        \
            (void *)((char *)_element_name + (_arr)->slot_size) \
    )

/**
 * Iterate the following statement over constant elements of an array,
 * forward. Do *not* assert that the array element size matches the size of
 * the type pointed to by "_element_name".
 *
 * @param _arr          The array to iterate over.
 * @param _index_name   Name of the ssize_t index variable.
 * @param _element_name Name of the element pointer variable.
 * @param _init_expr    A trailing initialization expression.
 * @param _incr_expr    A preceding increment expression.
 */
#define HDAG_ARR_IFWD_UNSIZED_CONST(_arr, _index_name, \
                                    _element_name,              \
                                    _init_expr, _incr_expr)     \
    assert(hdag_arr_is_valid(_arr));                            \
    for (_index_name = 0,                                       \
         _element_name = hdag_arr_slot_const(_arr, 0),          \
         _init_expr;                                            \
         _index_name < (ssize_t)(_arr)->slots_occupied;         \
         _incr_expr,                                            \
         _index_name++,                                         \
         _element_name =                                        \
            (void *)((char *)_element_name + (_arr)->slot_size) \
    )

/**
 * Iterate the following statement over the elements of an array,
 * backward.
 *
 * @param _arr          The array to iterate over. Must be variable.
 * @param _index_name   Name of the ssize_t index variable.
 * @param _element_name Name of the element pointer variable.
 * @param _init_expr    A trailing initialization expression.
 * @param _incr_expr    A preceding increment expression.
 */
#define HDAG_ARR_IBWD(_arr, _index_name, _element_name, \
                      _init_expr, _incr_expr)                           \
    assert(hdag_arr_is_valid(_arr));                                    \
    for (_index_name = (ssize_t)(_arr)->slots_occupied - 1,             \
         _element_name = hdag_arr_slot_sized(                           \
             _arr, sizeof(*(_element_name)), _index_name                \
         ),                                                             \
         _init_expr;                                                    \
         _element_name = (void *)(                                      \
             (char *)(_arr)->slots + _index_name * (_arr)->slot_size    \
         ), _init_expr;                                                 \
         _index_name >= 0;                                              \
         _incr_expr,                                                    \
         _index_name--,                                                 \
         _element_name =                                                \
            (void *)((char *)_element_name - (_arr)->slot_size)         \
    )

/**
 * Iterate the following statement over the elements of an array, backward.
 * Do *not* assert that the array element size matches the size of the type
 * pointed to by "_element_name".
 *
 * @param _arr          The array to iterate over. Must be variable.
 * @param _index_name   Name of the ssize_t index variable.
 * @param _element_name Name of the element pointer variable.
 * @param _init_expr    A trailing initialization expression.
 * @param _incr_expr    A preceding increment expression.
 */
#define HDAG_ARR_IBWD_UNSIZED(_arr, _index_name, \
                              _element_name,                            \
                              _init_expr, _incr_expr)                   \
    assert(hdag_arr_is_valid(_arr));                                    \
    for (_index_name = (ssize_t)(_arr)->slots_occupied - 1,             \
         _element_name = hdag_arr_slot(_arr, _index_name),              \
         _init_expr;                                                    \
         _element_name = (void *)(                                      \
             (char *)(_arr)->slots + _index_name * (_arr)->slot_size    \
         ), _init_expr;                                                 \
         _index_name >= 0;                                              \
         _incr_expr,                                                    \
         _index_name--,                                                 \
         _element_name =                                                \
            (void *)((char *)_element_name - (_arr)->slot_size)         \
    )

/**
 * Iterate the following statement over constant elements of an array,
 * backward.
 *
 * @param _arr          The array to iterate over.
 * @param _index_name   Name of the ssize_t index variable.
 * @param _element_name Name of the element pointer variable.
 * @param _init_expr    A trailing initialization expression.
 * @param _incr_expr    A preceding increment expression.
 */
#define HDAG_ARR_IBWD_CONST(_arr, _index_name, _element_name, \
                            _init_expr, _incr_expr)                     \
    assert(hdag_arr_is_valid(_arr));                                    \
    for (_index_name = (ssize_t)(_arr)->slots_occupied - 1,             \
         _element_name = hdag_arr_slot_sized_const(                     \
             _arr, sizeof(*(_element_name)), _index_name                \
         ),                                                             \
         _init_expr;                                                    \
         _element_name = (void *)(                                      \
             (char *)(_arr)->slots + _index_name * (_arr)->slot_size    \
         ), _init_expr;                                                 \
         _index_name >= 0;                                              \
         _incr_expr,                                                    \
         _index_name--,                                                 \
         _element_name =                                                \
            (void *)((char *)_element_name - (_arr)->slot_size)         \
    )

/**
 * Iterate the following statement over constant elements of an array,
 * backward. Do *not* assert that the array element size matches the size of
 * the type pointed to by "_element_name".
 *
 * @param _arr          The array to iterate over.
 * @param _index_name   Name of the ssize_t index variable.
 * @param _element_name Name of the element pointer variable.
 * @param _init_expr    A trailing initialization expression.
 * @param _incr_expr    A preceding increment expression.
 */
#define HDAG_ARR_IBWD_UNSIZED_CONST(_arr, _index_name, \
                                    _element_name,                      \
                                    _init_expr, _incr_expr)             \
    assert(hdag_arr_is_valid(_arr));                                    \
    for (_index_name = (ssize_t)(_arr)->slots_occupied - 1,             \
         _element_name = hdag_arr_slot_const(_arr, _index_name),        \
         _init_expr;                                                    \
         _element_name = (void *)(                                      \
             (char *)(_arr)->slots + _index_name * (_arr)->slot_size    \
         ), _init_expr;                                                 \
         _index_name >= 0;                                              \
         _incr_expr,                                                    \
         _index_name--,                                                 \
         _element_name =                                                \
            (void *)((char *)_element_name - (_arr)->slot_size)         \
    )

/**
 * An initializer for a pinned (non-reallocatable) slice of an array.
 *
 * @param _arr      The array to extract the slice from.
 * @param _start    The start index of the slice.
 * @param _end      The end index of the slice (pointing right after the last
 *                  element of the slice).
 *                  Must be greater than or equal to start, and less than or
 *                  equal to _arr->slots_occupied.
 */
#define HDAG_ARR_PINNED_SLICE(_arr, _start, _end) \
    HDAG_ARR_PINNED(                                                        \
        hdag_arr_slot(hdag_arr_slice_validate(_arr, _start, _end), _start), \
        (_arr)->slot_size,                                                  \
        (_end) - (_start),                                                  \
        (_end) - (_start)                                                   \
    )

/**
 * An initializer for a pinned static (non-reallocatable and non-resizeable)
 * slice of an array.
 *
 * @param _arr      The array to extract the slice from.
 * @param _start    The start index of the slice.
 * @param _end      The end index of the slice (pointing right after the last
 *                  element of the slice).
 *                  Must be greater than or equal to start, and less than or
 *                  equal to _arr->slots_occupied.
 */
#define HDAG_ARR_STATIC_PINNED_SLICE(_arr, _start, _end) \
    HDAG_ARR_PINNED_STATIC(                                                 \
        hdag_arr_slot(hdag_arr_slice_validate(_arr, _start, _end), _start), \
        (_arr)->slot_size,                                                  \
        (_end) - (_start)                                                   \
    )

/**
 * An initializer for a completely-immutable (pinned, static, and constant)
 * slice of an array.
 *
 * @param _arr      The array to extract the slice from.
 * @param _start    The start index of the slice.
 * @param _end      The end index of the slice (pointing right after the last
 *                  element of the slice).
 *                  Must be greater than or equal to start, and less than or
 *                  equal to _arr->slots_occupied.
 */
#define HDAG_ARR_IMMUTABLE_SLICE(_arr, _start, _end) \
    HDAG_ARR_IMMUTABLE(                                     \
        /* The "constant" flag should protect us */         \
        (void *)hdag_arr_slot_const(                        \
            hdag_arr_slice_validate(_arr, _start, _end),    \
            _start                                          \
        ),                                                  \
        (_arr)->slot_size,                                  \
        (_end) - (_start)                                   \
    )

static inline struct hdag_arr
hdag_arr_immutable_slice(const struct hdag_arr *arr,
                         size_t start, size_t end)
{
    assert(hdag_arr_slice_is_valid(arr, start, end));
    const size_t len = end - start;
    return HDAG_ARR_IMMUTABLE(/* The constant flag should protect us */
                              (void *)hdag_arr_slot_const(arr, start),
                              arr->slot_size, len);
}

/**
 * Check if a sorting specification is valid.
 *
 * @param cmp_min   The minimum comparison result of adjacent elements
 *                  [-1, cmp_max].
 * @param cmp_max   The maximum comparison result of adjacent elements
 *                  [cmp_min, 1].
 */
static inline bool
hdag_arr_sort_is_valid(int cmp_min, int cmp_max)
{
    return cmp_min >= -1 && cmp_max <= 1 && cmp_max >= cmp_min;
}

/**
 * Sort an array.
 *
 * @param arr   The array to sort.
 *              Cannot be void or constant, if has more than one element.
 * @param cmp   The element comparison function.
 * @param data  The private data to pass to the comparison function.
 */
static inline void
hdag_arr_sort(struct hdag_arr *arr, hdag_cmp_fn cmp, void *data)
{
    assert(hdag_arr_is_valid(arr));
    assert(cmp != NULL);
    if (arr->slots_occupied > 1) {
        assert(!hdag_arr_is_void(arr));
        assert(hdag_arr_is_variable(arr));
        qsort_r(arr->slots, arr->slots_occupied,
                arr->slot_size, cmp, data);
    }
}

/**
 * Sort an array using memcmp to compare elements.
 *
 * @param arr   The array to sort.
 */
static inline void
hdag_arr_mem_sort(struct hdag_arr *arr)
{
    assert(hdag_arr_is_valid(arr));
    assert(hdag_arr_is_variable(arr));
    hdag_arr_sort(arr, hdag_cmp_mem, (void *)(uintptr_t)arr->slot_size);
}

/**
 * Check if an array is sorted according to a specification.
 *
 * @param arr       The array to check.
 * @param cmp       The element comparison function.
 * @param data      The private data to pass to the comparison function.
 * @param cmp_min   The minimum comparison result of adjacent elements
 *                  [-1, cmp_max].
 * @param cmp_max   The maximum comparison result of adjacent elements
 *                  [cmp_min, 1].
 *
 * @return True if the array is sorted as specified.
 */
extern bool hdag_arr_is_sorted_as(const struct hdag_arr *arr,
                                  hdag_cmp_fn cmp, void *data,
                                  int cmp_min, int cmp_max);

/**
 * Check if an array is sorted according to specification, using memcmp
 * to compare elements.
 *
 * @param arr       The array to check.
 * @param cmp_min   The minimum comparison result of adjacent elements
 *                  [-1, cmp_max].
 * @param cmp_max   The maximum comparison result of adjacent elements
 *                  [cmp_min, 1].
 *
 * @return True if the array is sorted as specified.
 */
static inline bool
hdag_arr_is_mem_sorted_as(const struct hdag_arr *arr,
                          int cmp_min, int cmp_max)
{
    assert(hdag_arr_is_valid(arr));
    assert(hdag_arr_sort_is_valid(cmp_min, cmp_max));
    return hdag_arr_is_sorted_as(arr,
                                 hdag_cmp_mem,
                                 (void *)(uintptr_t)arr->slot_size,
                                 cmp_min,
                                 cmp_max);
}

/**
 * Check if an array is sorted in ascending order.
 *
 * @param arr   The array to check.
 * @param cmp   The element comparison function.
 * @param data  The private data to pass to the comparison function.
 *
 * @return True if the array is sorted in ascending order.
 */
static inline bool
hdag_arr_is_sorted(const struct hdag_arr *arr,
                   hdag_cmp_fn cmp, void *data)
{
    assert(hdag_arr_is_valid(arr));
    assert(cmp != NULL);
    return hdag_arr_is_sorted_as(arr, cmp, data, -1, 0);
}

/**
 * Check if an array is sorted in ascending order, using memcmp to
 * compare elements.
 *
 * @param arr   The array to check.
 *
 * @return True if the array is sorted in ascending order.
 */
static inline bool
hdag_arr_is_mem_sorted(const struct hdag_arr *arr)
{
    assert(hdag_arr_is_valid(arr));
    return hdag_arr_is_mem_sorted_as(arr, -1, 0);
}

/**
 * Check if an array is sorted in ascending order and deduplicated.
 *
 * @param arr   The array to check.
 * @param cmp   The element comparison function.
 * @param data  The private data to pass to the comparison function.
 *
 * @return True if the array is sorted in ascending order.
 */
static inline bool
hdag_arr_is_sorted_and_deduped(const struct hdag_arr *arr,
                               hdag_cmp_fn cmp, void *data)
{
    assert(hdag_arr_is_valid(arr));
    assert(cmp != NULL);
    return hdag_arr_is_sorted_as(arr, cmp, data, -1, -1);
}

/**
 * Check if an array is sorted in ascending order and deduplicated,
 * using memcmp to compare elements.
 *
 * @param arr   The array to check.
 *
 * @return True if the array is sorted in ascending order.
 */
static inline bool
hdag_arr_is_mem_sorted_and_deduped(const struct hdag_arr *arr)
{
    assert(hdag_arr_is_valid(arr));
    return hdag_arr_is_mem_sorted_as(arr, -1, -1);
}

/**
 * Deduplicate an array (remove adjacent equal elements).
 * Truncate the array to remove deduplicated elements.
 *
 * @param arr   The array to deduplicate. Must be variable and dynamic,
 *              unless empty or containing a single element.
 * @param cmp   The element comparison function.
 * @param data  The private data to pass to the comparison function.
 *
 * @return The new size of the array, after deduplicating.
 */
extern size_t hdag_arr_dedup(struct hdag_arr *arr,
                             hdag_cmp_fn cmp, void *data);

/**
 * Deduplicate an array (remove adjacent equal elements), using memcmp
 * to compare elements. Truncate the array to remove freed slots.
 *
 * @param arr   The array to deduplicate.
 *              Must be variable and dynamic, unless empty or containing a
 *              single element.
 *
 * @return The new size of the array, after deduplicating.
 */
static inline size_t
hdag_arr_mem_dedup(struct hdag_arr *arr)
{
    assert(hdag_arr_is_valid(arr));
    assert(hdag_arr_is_variable(arr));
    assert(hdag_arr_is_dynamic(arr));
    return hdag_arr_dedup(arr, hdag_cmp_mem,
                          (void *)(uintptr_t)arr->slot_size);
}

/**
 * Sort and deduplicate an array.
 * Truncate the array to remove deduplicated elements.
 *
 * @param arr   The array to sort and deduplicate.
 *              Must be variable and dynamic,
 *              unless empty or containing a single element.
 * @param cmp   The element comparison function.
 * @param data  The private data to pass to the comparison function.
 *
 * @return The new size of the array, after deduplicating.
 */
static inline size_t
hdag_arr_sort_and_dedup(struct hdag_arr *arr,
                        hdag_cmp_fn cmp, void *data)
{
    assert(hdag_arr_is_valid(arr));
    assert(hdag_arr_is_variable(arr));
    assert(hdag_arr_is_dynamic(arr));
    assert(cmp != NULL);
    hdag_arr_sort(arr, cmp, data);
    return hdag_arr_dedup(arr, cmp, data);
}

/**
 * Sort and deduplicate an array, using memcmp to compare elements.
 * Truncate the array to remove deduplicated elements.
 *
 * @param arr   The array to sort and deduplicate.
 *              Must be variable and dynamic,
 *              unless empty or containing a single element.
 *
 * @return The new size of the array, after deduplicating.
 */
static inline size_t
hdag_arr_mem_sort_and_dedup(struct hdag_arr *arr)
{
    assert(hdag_arr_is_valid(arr));
    assert(hdag_arr_is_variable(arr));
    assert(hdag_arr_is_dynamic(arr));
    hdag_arr_sort(arr, hdag_cmp_mem, (void *)(uintptr_t)arr->slot_size);
    return hdag_arr_dedup(arr,
                          hdag_cmp_mem,
                          (void *)(uintptr_t)arr->slot_size);
}

/**
 * Return the index corresponding to an array slot pointer.
 *
 * @param arr   The array containing the slot.
 *              Cannot be void.
 * @param slot  The pointer to the slot to return index of.
 *
 * @return The slot index.
 */
static inline size_t
hdag_arr_slot_idx(const struct hdag_arr *arr, const void *slot)
{
    assert(hdag_arr_is_valid(arr));
    assert(!hdag_arr_is_void(arr));
    assert(slot >= arr->slots);
    assert(slot <= hdag_arr_slot_const(arr, arr->slots_allocated));
    size_t off = (const char *)slot - (const char *)arr->slots;
    assert(off % arr->slot_size == 0);
    return off / arr->slot_size;
}

/**
 * Return the index corresponding to an array element pointer.
 *
 * @param arr       The array containing the element.
 *                  Cannot be void.
 * @param element   The pointer to the element to return index of.
 *
 * @return The element index.
 */
static inline size_t
hdag_arr_element_idx(const struct hdag_arr *arr, const void *element)
{
    assert(hdag_arr_is_valid(arr));
    assert(!hdag_arr_is_void(arr));
    assert(element >= arr->slots);
    assert(element < hdag_arr_slot_const(arr, arr->slots_occupied));
    return hdag_arr_slot_idx(arr, element);
}

/**
 * Copy an array over another one.
 *
 * @param pdst  Location for the output array.
 *              Cannot be NULL. Not modified in case of failure.
 * @param src   The source array to copy.
 *
 * @return True if copying succeeded, false if memory reallocation failed,
 *         and errno was set.
 */
static inline bool
hdag_arr_copy(struct hdag_arr *pdst, const struct hdag_arr *src)
{
    assert(pdst != NULL);
    assert(hdag_arr_is_valid(src));
    struct hdag_arr dst = HDAG_ARR_EMPTY(
        src->slot_size,
        (src->flags_slots_preallocate & HDAG_ARR_FSP_SLOTS_PREALLOCATE_MASK)
    );
    if (src->slots_occupied != 0 &&
        hdag_arr_append(&dst, src->slots, src->slots_occupied) == NULL) {
        return false;
    }
    *pdst = dst;
    return true;
}

/**
 * Resize an array to the specified number of elements, without
 * initializing the new elements.
 *
 * @param arr   The array to resize. Cannot be void.
 * @param num   The number of elements to resize the array to.
 *              Any existing elements above this number will be discarded.
 *              Any new elements below this number will be uninitialized.
 *
 * @return True if resizing succeeded, false if memory reallocation failed,
 *         and errno was set.
 */
static inline bool
hdag_arr_uresize(struct hdag_arr *arr, size_t num)
{
    assert(hdag_arr_is_valid(arr));
    if (num > arr->slots_allocated) {
        if (hdag_arr_alloc(arr, num - arr->slots_allocated) == NULL) {
            return false;
        }
    }
    arr->slots_occupied = num;
    return true;
}

/**
 * Find an element value in an array with binary search.
 *
 * @param arr       The array to search. Must be sorted in ascending
 *                  order (according to "cmp").
 * @param cmp       The element comparison function.
 * @param data      The private data to pass to the comparison function.
 * @param value     Pointer to the element value to search for.
 * @param pidx      Location for the index on which the search has stopped.
 *                  If the value was found, it will point to it.
 *                  If not, it will point to the closest value above it,
 *                  that is the place where the value should've been.
 *                  Can be NULL to skip index output.
 *
 * @return True if the value was found, false if not.
 */
extern bool hdag_arr_bsearch(const struct hdag_arr *arr,
                             hdag_cmp_fn cmp,
                             void *data,
                             const void *value,
                             size_t *pidx);

/**
 * Find an element value in an array with binary search, using memcmp
 * to compare elements.
 *
 * @param arr       The array to search. Must be sorted in ascending
 *                  order (according to "cmp").
 * @param value     Pointer to the element value to search for.
 * @param pidx      Location for the index on which the search has stopped.
 *                  If the value was found, it will point to it.
 *                  If not, it will point to the closest value above it,
 *                  that is the place where the value should've been.
 *                  Can be NULL to skip index output.
 *
 * @return True if the value was found, false if not.
 */
static inline bool
hdag_arr_mem_bsearch(const struct hdag_arr *arr,
                     const void *value,
                     size_t *pidx)
{
    assert(hdag_arr_is_valid(arr));
    assert(!hdag_arr_is_void(arr));
    assert(hdag_arr_is_mem_sorted(arr));
    assert(value != NULL);
    return hdag_arr_bsearch(arr,
                            hdag_cmp_mem,
                            (void *)(uintptr_t)arr->slot_size,
                            value,
                            pidx);
}

#endif /* _HDAG_ARR_H */
