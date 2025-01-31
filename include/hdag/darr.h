/**
 * Hash DAG dynamic array
 */

#ifndef _HDAG_DARR_H
#define _HDAG_DARR_H

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>

/** A dynamic array */
struct hdag_darr {
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
    /** Number of slots to preallocate initially */
    size_t slots_preallocate;
    /**
     * Number of allocated element slots,
     * or SIZE_MAX if the array is immutable (not really dynamic).
     */
    size_t slots_allocated;
    /** Number of occupied element slots */
    size_t slots_occupied;
};

/**
 * An initializer for an empty dynamic array.
 *
 * @param _slot_size            The size of each element slot.
 *                              The array is considered "void", if zero.
 * @param _slots_preallocate    Number of slots to preallocate.
 */
#define HDAG_DARR_EMPTY(_slot_size, _slots_preallocate) (struct hdag_darr){ \
    .slot_size = (_slot_size),                                              \
    .slots_preallocate = (_slots_preallocate),                              \
}

/**
 * An initializer for an immutable (not really dynamic) array.
 *
 * @param _slots            The pointer to the array slots.
 * @param _slot_size        The size of each element slot.
 *                          The array is considered "void", if zero.
 * @param _slots_occupied   Number of occupied slots (number of elements).
 */
#define HDAG_DARR_IMMUTABLE(_slots, _slot_size, _slots_occupied) \
    (struct hdag_darr){                                             \
        .slots = (_slots),                                          \
        .slot_size = (_slot_size),                                  \
        .slots_occupied = (_slots_occupied),                        \
        .slots_allocated = SIZE_MAX,                                \
    }

/** An initializer for a void dynamic array */
#define HDAG_DARR_VOID  HDAG_DARR_EMPTY(0, 0)

/**
 * Check if a dynamic array is valid.
 *
 * @param darr  The dynamic array to check.
 *
 * @return True if the dynamic array is valid, false otherwise.
 */
static inline bool
hdag_darr_is_valid(const struct hdag_darr *darr)
{
    return darr != NULL &&
        darr->slots_occupied <= darr->slots_allocated &&
        (darr->slots != NULL ||
         darr->slots_allocated == 0 ||
         (darr->slots_allocated == SIZE_MAX && darr->slots_occupied == 0) ||
         darr->slot_size == 0);
}

/**
 * Check if a dynamic array is immutable.
 *
 * @param darr  The dynamic array to check.
 *
 * @return True if the dynamic array is immutable, false otherwise.
 */
static inline bool
hdag_darr_is_immutable(const struct hdag_darr *darr)
{
    assert(hdag_darr_is_valid(darr));
    return darr->slots_allocated == SIZE_MAX;
}

/**
 * Check if a dynamic array is mutable.
 *
 * @param darr  The dynamic array to check.
 *
 * @return True if the dynamic array is mutable, false otherwise.
 */
static inline bool
hdag_darr_is_mutable(const struct hdag_darr *darr)
{
    assert(hdag_darr_is_valid(darr));
    return darr->slots_allocated != SIZE_MAX;
}

/**
 * Check if a dynamic array is void (has slot size zero), cannot be
 * accessed or modified, and always has size zero.
 *
 * @param darr  The dynamic array to check.
 *
 * @return True if the dynamic array is void, false otherwise.
 */
static inline bool
hdag_darr_is_void(const struct hdag_darr *darr)
{
    assert(hdag_darr_is_valid(darr));
    return darr->slot_size == 0;
}

/**
 * Get the number of occupied slots.
 *
 * @param darr  The dynamic array to get the number of occupied slots for.
 *
 * @return The number of occupied slots, bytes.
 */
static inline size_t
hdag_darr_occupied_slots(const struct hdag_darr *darr)
{
    assert(hdag_darr_is_valid(darr));
    return darr->slots_occupied;
}

/**
 * Get the size of occupied slots, in bytes.
 *
 * @param darr  The dynamic array to get the size of occupied slots.
 *
 * @return The size of occupied slots, bytes.
 */
static inline size_t
hdag_darr_occupied_size(const struct hdag_darr *darr)
{
    assert(hdag_darr_is_valid(darr));
    return darr->slot_size * darr->slots_occupied;
}

/**
 * Get the number of allocated (occupied, if immutable) slots.
 *
 * @param darr  The dynamic array to get the number of allocated slots for.
 *
 * @return The number of allocated (occupied, if immutable) slots.
 */
static inline size_t
hdag_darr_allocated_slots(const struct hdag_darr *darr)
{
    assert(hdag_darr_is_valid(darr));
    return darr->slots_allocated == SIZE_MAX
        ? darr->slots_occupied
        : darr->slots_allocated;
}

/**
 * Get the size of allocated (occupied, if immutable) slots, in bytes.
 *
 * @param darr  The dynamic array to get the size of allocated slots.
 *
 * @return The size of allocated (occupied, if immutable) slots, bytes.
 */
static inline size_t
hdag_darr_allocated_size(const struct hdag_darr *darr)
{
    assert(hdag_darr_is_valid(darr));
    return darr->slot_size * hdag_darr_allocated_slots(darr);
}

/**
 * Retrieve the const pointer to a const dynamic array element slot at
 * specified index.
 *
 * @param darr  The array to retrieve the element pointer from.
 *              Cannot be void.
 * @param idx   The index of the element slot to retrieve the pointer to.
 *              Must be within allocated area, but not necessarily within
 *              occupied area.
 *
 * @return The element slot pointer. NULL if the array was void.
 */
static inline const void *
hdag_darr_slot_const(const struct hdag_darr *darr, size_t idx)
{
    assert(hdag_darr_is_valid(darr));
    assert(!hdag_darr_is_void(darr));
    assert(idx <= darr->slots_allocated);
    return (char *)darr->slots + darr->slot_size * idx;
}

/**
 * Retrieve the pointer to a dynamic array element slot at specified index.
 *
 * @param darr  The array to retrieve the element pointer from.
 *              Cannot be void.
 * @param idx   The index of the element slot to retrieve the pointer to.
 *              Must be within allocated area, but not necessarily within
 *              occupied area.
 *
 * @return The element slot pointer. NULL if the array was void.
 */
static inline void *
hdag_darr_slot(struct hdag_darr *darr, size_t idx)
{
    assert(hdag_darr_is_valid(darr));
    assert(!hdag_darr_is_void(darr));
    assert(idx <= darr->slots_allocated);
    assert(hdag_darr_is_mutable(darr));
    return (char *)darr->slots + darr->slot_size * idx;
}

/**
 * Retrieve the pointer to a dynamic array element at specified index.
 *
 * @param darr  The array to retrieve the element pointer from.
 *              Cannot be void.
 * @param idx   The index of the element to retrieve the pointer to.
 *              Must be within occupied area.
 *
 * @return The element pointer. NULL if the array was void.
 */
static inline void *
hdag_darr_element(struct hdag_darr *darr, size_t idx)
{
    assert(hdag_darr_is_valid(darr));
    assert(!hdag_darr_is_void(darr));
    assert(idx < darr->slots_occupied);
    assert(hdag_darr_is_mutable(darr));
    return (char *)darr->slots + darr->slot_size * idx;
}

/**
 * Retrieve the pointer to a dynamic array element at specified index,
 * asserting that the size of the element matches the specified size.
 *
 * @param darr  The array to retrieve the element pointer from.
 *              Cannot be void.
 * @param size  The expected size of the element to assert.
 * @param idx   The index of the element to retrieve the pointer to.
 *              Must be within occupied area.
 *
 * @return The element pointer. NULL if the array was void.
 */
static inline void *
hdag_darr_element_sized(struct hdag_darr *darr, size_t size, size_t idx)
{
    assert(hdag_darr_is_valid(darr));
    assert(!hdag_darr_is_void(darr));
    assert(size == darr->slot_size);
    assert(idx < darr->slots_occupied);
    assert(hdag_darr_is_mutable(darr));
    (void)size;
    return (char *)darr->slots + darr->slot_size * idx;
}

/**
 * Retrieve the const pointer to an element at specified index inside a const
 * dynamic array.
 *
 * @param darr  The array to retrieve the element pointer from.
 *              Cannot be void.
 * @param idx   The index of the element to retrieve the pointer to.
 *              Must be within occupied area.
 *
 * @return The element pointer. NULL if the array was void.
 */
static inline const void *
hdag_darr_element_const(const struct hdag_darr *darr, size_t idx)
{
    assert(hdag_darr_is_valid(darr));
    assert(!hdag_darr_is_void(darr));
    assert(idx < darr->slots_occupied);
    return (const char *)darr->slots + darr->slot_size * idx;
}

/**
 * Retrieve the const pointer to an element at specified index inside a const
 * dynamic array, asserting that the size of the element matches the specified
 * size.
 *
 * @param darr  The array to retrieve the element pointer from.
 *              Cannot be void.
 * @param idx   The index of the element to retrieve the pointer to.
 *              Must be within occupied area.
 * @param size  The expected size of the element to assert.
 *
 * @return The element pointer. NULL if the array was void.
 */
static inline const void *
hdag_darr_element_sized_const(const struct hdag_darr *darr,
                              size_t size, size_t idx)
{
    assert(hdag_darr_is_valid(darr));
    assert(!hdag_darr_is_void(darr));
    assert(size == darr->slot_size);
    assert(idx < darr->slots_occupied);
    (void)size;
    return (const char *)darr->slots + darr->slot_size * idx;
}

/**
 * Retrieve the (possibly const) pointer to an element of specified type, at
 * specified index inside a (possibly const) dynamic array. Assert that the
 * size of the element type matches the dynamic array's slot size. Cast the
 * returned pointer to a pointer to the specified type.
 *
 * @param _darr The array to retrieve the element pointer from.
 *              Cannot be void.
 * @param _type The type of the array element (not a pointer to it) to assert
 *              the size of, and to cast the returned type to (making it a
 *              pointer). Will have "const" qualifier added, if _darr is
 *              const.
 * @param _idx  The index of the element to retrieve the pointer to.
 *              Must be within occupied area.
 *
 * @return The element pointer (const, if the dynamic array was const).
 *         NULL if the array was void.
 */
#define HDAG_DARR_ELEMENT(_darr, _type, _idx) \
    _Generic(                                                                   \
        _darr,                                                                  \
        struct hdag_darr *: (_type *)hdag_darr_element_sized(                   \
            (struct hdag_darr *)_darr, sizeof(_type), _idx                      \
        ),                                                                      \
        const struct hdag_darr *: (const _type *)hdag_darr_element_sized_const( \
            _darr, sizeof(_type), _idx                                          \
        )                                                                       \
    )

/**
 * Retrieve the (possibly const) pointer to an element of specified type, at
 * specified index inside a (possibly const) dynamic array. Do *not* assert
 * that the size of the element type matches the dynamic array's slot size,
 * catering to the cases when each element is in itself an array, or when size
 * verification is not necessary. Cast the returned pointer to a pointer to
 * the specified type.
 *
 * @param _darr The array to retrieve the element pointer from.
 *              Cannot be void.
 * @param _type The type of the array element (not a pointer to it) to cast
 *              the returned type to (making it a pointer). Will have "const"
 *              qualifier added, if _darr is const.
 * @param _idx  The index of the element to retrieve the pointer to.
 *              Must be within occupied area.
 *
 * @return The element pointer (const, if the dynamic array was const).
 *         NULL if the array was void.
 */
#define HDAG_DARR_ELEMENT_UNSIZED(_darr, _type, _idx) \
    _Generic(                                                               \
        _darr,                                                              \
        struct hdag_darr *: (_type *)hdag_darr_element(                     \
            (struct hdag_darr *)_darr, _idx                                 \
        ),                                                                  \
        const struct hdag_darr *: (const _type *)hdag_darr_element_const(   \
            _darr, _idx                                                     \
        )                                                                   \
    )

/**
 * Make sure a dynamic array has slots allocated for specified number of
 * extra elements.
 *
 * @param darr  The dynamic array to allocate slots in.
 *              Cannot be void unless num is zero.
 * @param num   The number of elements to allocate slots for.
 *
 * @return The pointer to the first allocated element slot in the array, if
 *         succeeded. NULL, if "num" was zero, the array was "void", or
 *         allocation failed (in which case errno is set).
 */
[[nodiscard]]
extern void *hdag_darr_alloc(struct hdag_darr *darr, size_t num);

/**
 * Make sure a dynamic array has slots allocated and zeroed for specified
 * number of extra elements.
 *
 * @param darr  The dynamic array to allocate and zero slots in.
 *              Cannot be void unless num is zero.
 * @param num   The number of elements to allocate slots for.
 *
 * @return The pointer to the first allocated element slot in the array, if
 *         succeeded. NULL, if "num" was zero, the array was "void", or
 *         allocation failed (in which case errno is set).
 */
[[nodiscard]]
static inline void *
hdag_darr_calloc(struct hdag_darr *darr, size_t num)
{
    assert(hdag_darr_is_valid(darr));
    assert(hdag_darr_is_mutable(darr));
    void *slots = hdag_darr_alloc(darr, num);
    if (slots != NULL) {
        memset(slots, 0, darr->slot_size * num);
    }
    return slots;
}

/**
 * Make sure a dynamic array has a slot allocated for one more element.
 *
 * @param darr  The dynamic array to allocate the slot in. Cannot be void.
 *
 * @return The pointer to the allocated element slot in the array, if
 *         succeeded. NULL, if the array was "void", or allocation failed (in
 *         which case errno is set).
 */
[[nodiscard]]
static inline void *
hdag_darr_alloc_one(struct hdag_darr *darr)
{
    assert(hdag_darr_is_valid(darr));
    assert(hdag_darr_is_mutable(darr));
    assert(!hdag_darr_is_void(darr));
    return hdag_darr_alloc(darr, 1);
}

/**
 * Make sure a dynamic array has a slot allocated and zeroed for one more
 * element.
 *
 * @param darr  The dynamic array to allocate and zero the slot in.
 *              Cannot be void.
 *
 * @return The pointer to the allocated element slot in the array, if
 *         succeeded. NULL, if the array was "void", or allocation failed (in
 *         which case errno is set).
 */
[[nodiscard]]
static inline void *
hdag_darr_calloc_one(struct hdag_darr *darr)
{
    assert(hdag_darr_is_valid(darr));
    assert(hdag_darr_is_mutable(darr));
    assert(!hdag_darr_is_void(darr));
    return hdag_darr_calloc(darr, 1);
}

/**
 * Insert an unitialized slice of elements into a dynamic array.
 *
 * @param darr      The dynamic array to insert the slice into.
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
hdag_darr_uinsert(struct hdag_darr *darr, size_t start, size_t num)
{
    assert(hdag_darr_is_valid(darr));
    assert(hdag_darr_is_mutable(darr));
    assert(start <= darr->slots_occupied);
    size_t new_slots_occupied = darr->slots_occupied + num;
    if (num == 0 || hdag_darr_alloc(darr, new_slots_occupied) == NULL) {
        return NULL;
    }
    void *start_slot = hdag_darr_slot(darr, start);
    assert(!hdag_darr_is_void(darr));
    memmove(start_slot,
            hdag_darr_slot(darr, start + num),
            (darr->slots_occupied - start) * darr->slot_size);
    darr->slots_occupied = new_slots_occupied;
    return start_slot;
}

/**
 * Insert a slice of zeroed elements into a dynamic array.
 *
 * @param darr      The dynamic array to insert the slice into.
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
hdag_darr_cinsert(struct hdag_darr *darr, size_t start, size_t num)
{
    assert(hdag_darr_is_valid(darr));
    assert(hdag_darr_is_mutable(darr));
    assert(start <= darr->slots_occupied);
    void *start_slot = hdag_darr_uinsert(darr, start, num);
    if (start_slot != NULL) {
        memset(start_slot, 0, num * darr->slot_size);
    }
    return start_slot;
}

/**
 * Insert a slice of elements into a dynamic array.
 *
 * @param darr      The dynamic array to insert the slice into.
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
hdag_darr_insert(struct hdag_darr *darr, size_t start,
                 const void *elements, size_t num)
{
    assert(hdag_darr_is_valid(darr));
    assert(hdag_darr_is_mutable(darr));
    assert(start <= darr->slots_occupied);
    assert(elements != NULL || num == 0);
    void *start_slot = hdag_darr_uinsert(darr, start, num);
    if (start_slot != NULL) {
        memcpy(start_slot, elements, num * darr->slot_size);
    }
    return start_slot;
}

/**
 * Insert one element into a dynamic array.
 *
 * @param darr      The dynamic array to insert the element into.
 *                  Cannot be void.
 * @param idx       The index of the slot to insert the element at.
 * @param element   The pointer to the element to insert.
 *
 * @return The pointer to the inserted element in the array, if succeeded.
 *         NULL, if allocation failed (in which case errno is set).
 */
[[nodiscard]]
static inline void *
hdag_darr_insert_one(struct hdag_darr *darr, size_t idx, const void *element)
{
    assert(hdag_darr_is_valid(darr));
    assert(hdag_darr_is_mutable(darr));
    assert(idx <= darr->slots_occupied);
    assert(element != NULL);
    return hdag_darr_insert(darr, idx, element, 1);
}

/**
 * Append the specified number of *uninitialized* elements to a dynamic array.
 *
 * @param darr  The dynamic array to append elements to.
 *              Cannot be void unless num is zero.
 * @param num   The number of elements to append.
 *
 * @return The pointer to the first appended element in the array, if
 *         succeeded. NULL, if "num" was zero, the array was "void", or
 *         allocation failed (in which case errno is set).
 */
[[nodiscard]]
static inline void *
hdag_darr_uappend(struct hdag_darr *darr, size_t num)
{
    assert(hdag_darr_is_valid(darr));
    assert(hdag_darr_is_mutable(darr));
    return hdag_darr_uinsert(darr, darr->slots_occupied, num);
}

/**
 * Append the specified number of elements to a dynamic array.
 *
 * @param darr      The dynamic array to append elements to.
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
hdag_darr_append(struct hdag_darr *darr, const void *elements, size_t num)
{
    assert(hdag_darr_is_valid(darr));
    assert(hdag_darr_is_mutable(darr));
    assert(elements != NULL || num == 0);
    return hdag_darr_insert(darr, darr->slots_occupied, elements, num);
}

/**
 * Append one element to a dynamic array.
 *
 * @param darr      The dynamic array to append elements to. Cannot be void.
 * @param element   The pointer to the element to append.
 *
 * @return The pointer to the appended element in the array, if succeeded.
 *         NULL, if the array was "void", or allocation failed (in which case
 *         errno is set).
 */
[[nodiscard]]
static inline void *
hdag_darr_append_one(struct hdag_darr *darr, const void *element)
{
    assert(hdag_darr_is_valid(darr));
    assert(hdag_darr_is_mutable(darr));
    assert(!hdag_darr_is_void(darr));
    return hdag_darr_append(darr, element, 1);
}

/**
 * Append the specified number of zero-filled elements to a dynamic array.
 *
 * @param darr      The dynamic array to append elements to.
 *                  Cannot be void unless num is zero.
 * @param num       The number of zeroed elements to append.
 *
 * @return The pointer to the first appended element in the array, if
 *         succeeded. NULL, if "num" was zero, the array was "void", or
 *         allocation failed (in which case errno is set).
 */
[[nodiscard]]
static inline void *
hdag_darr_cappend(struct hdag_darr *darr, size_t num)
{
    assert(hdag_darr_is_valid(darr));
    assert(hdag_darr_is_mutable(darr));
    return hdag_darr_cinsert(darr, darr->slots_occupied, num);
}

/**
 * Append a zero-filled element to a dynamic array.
 *
 * @param darr      The dynamic array to append the zeroed element to.
 *                  Cannot be void.
 *
 * @return The pointer to the appended element in the array, if succeeded.
 *         NULL, if the array was "void", or allocation failed (in which case
 *         errno is set).
 */
[[nodiscard]]
static inline void *
hdag_darr_cappend_one(struct hdag_darr *darr)
{
    assert(hdag_darr_is_valid(darr));
    assert(hdag_darr_is_mutable(darr));
    assert(!hdag_darr_is_void(darr));
    return hdag_darr_cappend(darr, 1);
}

/**
 * Free all empty element slots in a dynamic array ("deflate").
 *
 * @param darr  The dynamic array to deflate.
 *
 * @return True if deflating succeeded, false if memory reallocation failed
 *         (in which case errno is set).
 */
[[nodiscard]]
extern bool hdag_darr_deflate(struct hdag_darr *darr);

/**
 * Remove all elements from a dynamic array, but keep the allocated slots.
 *
 * @param darr  The dynamic array to empty.
 */
static inline void
hdag_darr_empty(struct hdag_darr *darr)
{
    assert(hdag_darr_is_valid(darr));
    assert(hdag_darr_is_mutable(darr));
    darr->slots_occupied = 0;
}

/**
 * Check if a dynamic array is empty.
 *
 * @param darr  The dynamic array to check.
 *
 * @return True if the dynamic array has no elements, false otherwise.
 */
static inline bool
hdag_darr_is_empty(const struct hdag_darr *darr)
{
    assert(hdag_darr_is_valid(darr));
    return darr->slots_occupied == 0;
}

/**
 * Remove all element slots from a dynamic array, empty or not.
 * This deallocates all referenced memory for mutable arrays, and makes
 * immutable array mutable.
 *
 * @param darr  The dynamic array to cleanup.
 */
static inline void
hdag_darr_cleanup(struct hdag_darr *darr)
{
    assert(hdag_darr_is_valid(darr));
    if (hdag_darr_is_mutable(darr)) {
        free(darr->slots);
    }
    darr->slots = NULL;
    darr->slots_occupied = 0;
    darr->slots_allocated = 0;
    assert(hdag_darr_is_valid(darr));
}

/**
 * Check if a dynamic array is clean, that is has no element slots, which
 * means it doesn't refer to any allocated memory.
 *
 * @param darr  The dynamic array to check.
 *
 * @return True if the dynamic array is clean, false otherwise.
 */
static inline bool
hdag_darr_is_clean(const struct hdag_darr *darr)
{
    assert(hdag_darr_is_valid(darr));
    return darr->slots_allocated == 0;
}

/**
 * Remove a slice of elements from a dynamic array.
 *
 * @param darr  The dynamic array to remove the slice from.
 *              Cannot be void unless start == end.
 * @param start The index of the first slot of the slice to remove.
 * @param end   The index of the first slot after the slice being removed.
 *              Must be equal to, or greater than "start".
 */
static inline void
hdag_darr_remove(struct hdag_darr *darr, size_t start, size_t end)
{
    assert(hdag_darr_is_valid(darr));
    assert(hdag_darr_is_mutable(darr));
    assert(start <= end);
    assert(end <= darr->slots_occupied);
    if (start != end) {
        assert(!hdag_darr_is_void(darr));
        memmove(hdag_darr_slot(darr, start), hdag_darr_slot(darr, end),
                (darr->slots_occupied - end) * darr->slot_size);
        darr->slots_occupied -= end - start;
    }
}

/**
 * Remove an element from a dynamic array.
 *
 * @param darr  The dynamic array to remove the slice from.
 *              Cannot be void.
 * @param idx   The index of the element to remove.
 */
static inline void
hdag_darr_remove_one(struct hdag_darr *darr, size_t idx)
{
    assert(hdag_darr_is_valid(darr));
    assert(hdag_darr_is_mutable(darr));
    assert(!hdag_darr_is_void(darr));
    assert(idx < darr->slots_occupied);
    hdag_darr_remove(darr, idx, idx + 1);
}

/**
 * Iterate the following statement over the elements of a dynamic array,
 * forward.
 *
 * @param _darr         The array to iterate over.
 * @param _index_name   Name of the ssize_t index variable.
 * @param _element_name Name of the element pointer variable.
 * @param _init_expr    A trailing initialization expression.
 * @param _incr_expr    A preceding increment expression.
 */
#define HDAG_DARR_ITER_FORWARD(_darr, _index_name, _element_name, \
                               _init_expr, _incr_expr)                  \
    assert(hdag_darr_is_valid(_darr));                                  \
    for (_index_name = 0, _element_name = (_darr)->slots, _init_expr;   \
         _index_name < (ssize_t)(_darr)->slots_occupied;                \
         _incr_expr,                                                    \
         _index_name++,                                                 \
         _element_name =                                                \
            (void *)((char *)_element_name + (_darr)->slot_size)        \
    )

/**
 * Iterate the following statement over the elements of a dynamic array,
 * backward.
 *
 * @param _darr         The array to iterate over.
 * @param _index_name   Name of the ssize_t index variable.
 * @param _element_name Name of the element pointer variable.
 * @param _init_expr    A trailing initialization expression.
 * @param _incr_expr    A preceding increment expression.
 */
#define HDAG_DARR_ITER_BACKWARD(_darr, _index_name, _element_name, \
                                _init_expr, _incr_expr)                 \
    assert(hdag_darr_is_valid(_darr));                                  \
    for (_index_name = (ssize_t)(_darr)->slots_occupied - 1,            \
         _element_name = (void *)(                                      \
             (char *)(_darr)->slots + _index_name * (_darr)->slot_size  \
         ), _init_expr;                                                 \
         _index_name >= 0;                                              \
         _incr_expr,                                                    \
         _index_name--,                                                 \
         _element_name =                                                \
            (void *)((char *)_element_name - (_darr)->slot_size)        \
    )

/**
 * The prototype for a dynamic array element comparison function.
 *
 * @param first     The first element to compare.
 * @param second    The second element to compare.
 * @param data      The function's private data.
 *
 * @return -1 if first < second, 0 if first == second, 1 if first > second.
 */
typedef int (*hdag_darr_cmp_fn)(const void *first, const void *second,
                                void *data);

/**
 * Qsort a slice of a dynamic array.
 *
 * @param darr  The dynamic array containing the slice to sort.
 *              Cannot be void unless both start and end are zero.
 * @param start The index of the first element of the slice to be sorted.
 * @param end   The index of the first element *after* the slice to be sorted.
 * @param cmp   The element comparison function.
 * @param data  The private data to pass to the comparison function.
 */
static inline void
hdag_darr_qsort(struct hdag_darr *darr, size_t start, size_t end,
                hdag_darr_cmp_fn cmp, void *data)
{
    assert(hdag_darr_is_valid(darr));
    assert(hdag_darr_is_mutable(darr));
    assert(start <= end);
    assert(end <= darr->slots_occupied);
    assert(cmp != NULL);
    if (start != end) {
        assert(!hdag_darr_is_void(darr));
        qsort_r(hdag_darr_slot(darr, start), end - start,
                darr->slot_size, cmp, data);
    }
}

/**
 * Qsort a complete dynamic array.
 *
 * @param darr  The dynamic array containing the slice to sort.
 * @param cmp   The element comparison function.
 * @param data  The private data to pass to the comparison function.
 */
static inline void
hdag_darr_qsort_all(struct hdag_darr *darr, hdag_darr_cmp_fn cmp, void *data)
{
    assert(hdag_darr_is_valid(darr));
    assert(hdag_darr_is_mutable(darr));
    assert(cmp != NULL);
    hdag_darr_qsort(darr, 0, darr->slots_occupied, cmp, data);
}

/**
 * Return the index corresponding to an array slot pointer.
 *
 * @param darr  The dynamic array containing the slot.
 *              Cannot be void.
 * @param slot  The pointer to the slot to return index of.
 *
 * @return The slot index.
 */
static inline size_t
hdag_darr_element_idx(const struct hdag_darr *darr, const void *element)
{
    assert(hdag_darr_is_valid(darr));
    assert(!hdag_darr_is_void(darr));
    assert(element >= darr->slots);
    assert(element < hdag_darr_slot_const(darr, darr->slots_occupied));
    size_t off = (const char *)element - (const char *)darr->slots;
    assert(off % darr->slot_size == 0);
    return off / darr->slot_size;
}

/**
 * Copy a dynamic array over another one.
 *
 * @param pdst  Location for the output dynamic array.
 *              Cannot be NULL. Not modified in case of failure.
 * @param src   The source dynamic array to copy.
 *
 * @return True if copying succeeded, false if memory reallocation failed,
 *         and errno was set.
 */
static inline bool
hdag_darr_copy(struct hdag_darr *pdst, const struct hdag_darr *src)
{
    assert(pdst != NULL);
    assert(hdag_darr_is_valid(src));
    struct hdag_darr dst = HDAG_DARR_EMPTY(src->slot_size,
                                           src->slots_preallocate);
    if (src->slots_occupied != 0 &&
        hdag_darr_append(&dst, src->slots, src->slots_occupied) == NULL) {
        return false;
    }
    *pdst = dst;
    return true;
}

/**
 * Resize a dynamic array to the specified number of elements, without
 * initializing the new elements.
 *
 * @param darr  The dynamic array to resize. Cannot be void.
 * @param num   The number of elements to resize the array to.
 *              Any existing elements above this number will be discarded.
 *              Any new elements below this number will be uninitialized.
 *
 * @return True if resizing succeeded, false if memory reallocation failed,
 *         and errno was set.
 */
static inline bool
hdag_darr_uresize(struct hdag_darr *darr, size_t num)
{
    assert(hdag_darr_is_valid(darr));
    if (num > darr->slots_allocated) {
        if (hdag_darr_alloc(darr, num - darr->slots_allocated) == NULL) {
            return false;
        }
    }
    darr->slots_occupied = num;
    return true;
}

#endif /* _HDAG_DARR_H */
