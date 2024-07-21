/*
 * Hash DAG dynamic array
 */

#ifndef _HDAG_DARR_H
#define _HDAG_DARR_H

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>

/** A dynamic array */
struct hdag_darr {
    /** The size of each array element slot */
    size_t slot_size;
    /** The memory slots allocated for the elements */
    void *slots;
    /** Number of slots to preallocate initially */
    size_t slots_preallocate;
    /** Number of allocated element slots */
    size_t slots_allocated;
    /** Number of occupied element slots */
    size_t slots_occupied;
};

/**
 * Check if a slot size is valid.
 *
 * @param slot_size  The element slot size to check.
 *
 * @return True if the size is valid, false otherwise.
 */
static inline bool
hdag_darr_slot_size_is_valid(size_t slot_size)
{
    return slot_size != 0;
}

/**
 * Check if the number of slots to preallocate is valid.
 *
 * @param slots_preallocate The number of slots to check.
 *
 * @return True if the number is valid, false otherwise.
 */
static inline bool
hdag_darr_slots_preallocate_is_valid(size_t slots_preallocate)
{
    return slots_preallocate != 0;
}

/**
 * Validate a dynamic array's element slot size.
 *
 * @param size  The element slot size to validate.
 *
 * @return The validated size.
 */
static inline size_t
hdag_darr_slot_size_validate(size_t slot_size)
{
    assert(hdag_darr_slot_size_is_valid(slot_size));
    return slot_size;
}

/**
 * Validate the number of slots to preallocate for an array.
 *
 * @param slots_preallocate   The number of slots to validate.
 *
 * @return The validated number of slots.
 */
static inline size_t
hdag_darr_slots_preallocate_validate(size_t slots_preallocate)
{
    assert(hdag_darr_slots_preallocate_is_valid(slots_preallocate));
    return slots_preallocate;
}

/**
 * An initializer for an empty dynamic array.
 *
 * @param _slot_size            The size of each element slot.
 *                              Cannot be zero.
 * @param _slots_preallocate    Number of slots to preallocate.
 *                              Cannot be zero.
 */
#define HDAG_DARR_EMPTY(_slot_size, _slots_preallocate) (struct hdag_darr){ \
    .slot_size =                                                            \
        hdag_darr_slot_size_validate(_slot_size),                           \
    .slots_preallocate =                                                    \
        hdag_darr_slots_preallocate_validate(_slots_preallocate),           \
}

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
        hdag_darr_slot_size_is_valid(darr->slot_size) &&
        hdag_darr_slots_preallocate_is_valid(darr->slots_preallocate) &&
        darr->slots_occupied <= darr->slots_allocated &&
        (darr->slots != NULL || darr->slots_allocated == 0);
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
 * Get the size of allocated slots, in bytes.
 *
 * @param darr  The dynamic array to get the size of allocated slots.
 *
 * @return The size of allocated slots, bytes.
 */
static inline size_t
hdag_darr_allocated_size(const struct hdag_darr *darr)
{
    assert(hdag_darr_is_valid(darr));
    return darr->slot_size * darr->slots_allocated;
}

/**
 * Make sure a dynamic array has slots allocated for specified number of
 * elements.
 *
 * @param darr  The dynamic array to allocate slots in.
 * @param num   The number of elements to allocate slots for.
 *
 * @return The pointer to the first allocated element slot in the array, if
 *         succeeded. NULL, if allocation failed, and errno was set.
 */
extern void *hdag_darr_alloc(struct hdag_darr *darr, size_t num);

/**
 * Make sure a dynamic array has slots allocated and zeroed for specified
 * number of elements.
 *
 * @param darr  The dynamic array to allocate and zero slots in.
 * @param num   The number of elements to allocate slots for.
 *
 * @return The pointer to the first allocated element slot in the array, if
 *         succeeded. NULL, if allocation failed, and errno was set.
 */
static inline void *
hdag_darr_calloc(struct hdag_darr *darr, size_t num)
{
    assert(hdag_darr_is_valid(darr));
    void *slots = hdag_darr_alloc(darr, num);
    if (slots != NULL) {
        memset(slots, 0, darr->slot_size * num);
    }
    return slots;
}

/**
 * Make sure a dynamic array has a slot allocated for one more elements.
 *
 * @param darr  The dynamic array to allocate the slot in.
 *
 * @return The pointer to the allocated element slot in the array, if
 *         succeeded. NULL, if allocation failed, and errno was set.
 */
static inline void *
hdag_darr_alloc_one(struct hdag_darr *darr)
{
    assert(hdag_darr_is_valid(darr));
    return hdag_darr_alloc(darr, 1);
}

/**
 * Make sure a dynamic array has a slot allocated and zeroed for one more
 * element.
 *
 * @param darr  The dynamic array to allocate and zero the slot in.
 *
 * @return The pointer to the allocated element slot in the array, if
 *         succeeded. NULL, if allocation failed, and errno was set.
 */
static inline void *
hdag_darr_calloc_one(struct hdag_darr *darr)
{
    assert(hdag_darr_is_valid(darr));
    return hdag_darr_calloc(darr, 1);
}

/**
 * Append the specified number of elements to a dynamic array.
 *
 * @param darr      The dynamic array to append elements to
 * @param elements  The array containing elements to append.
 * @param num       The number of elements from the array to append.
 *
 * @return The pointer to the appended elements in the array, if
 *         succeeded. NULL, if allocation failed, and errno was set.
 */
extern void *hdag_darr_append(struct hdag_darr *darr,
                              void *elements, size_t num);

/**
 * Append one element to a dynamic array.
 *
 * @param darr      The dynamic array to append elements to
 * @param element   The pointer to the element to append.
 *
 * @return The pointer to the appended element in the array, if
 *         succeeded. NULL, if allocation failed, and errno was set.
 */
static inline void *
hdag_darr_append_one(struct hdag_darr *darr, void *element)
{
    assert(hdag_darr_is_valid(darr));
    return hdag_darr_append(darr, element, 1);
}

/**
 * Retrieve the pointer to a dynamic array element slot at specified index.
 *
 * @param darr  The array to retrieve the element pointer from.
 * @param idx   The index of the element slot to retrieve the pointer to.
 *              Must be within allocated area, but not necessarily within
 *              occupied area.
 *
 * @return The element slot pointer.
 */
static inline void *
hdag_darr_slot(struct hdag_darr *darr, size_t idx)
{
    assert(hdag_darr_is_valid(darr));
    assert(idx < darr->slots_allocated);
    return (char *)darr->slots + darr->slot_size * idx;
}

/**
 * Retrieve the pointer to a dynamic array element at specified index.
 *
 * @param darr  The array to retrieve the element pointer from.
 * @param idx   The index of the element to retrieve the pointer to.
 *              Must be within occupied area.
 *
 * @return The element slot pointer.
 */
static inline void *
hdag_darr_element(struct hdag_darr *darr, size_t idx)
{
    assert(hdag_darr_is_valid(darr));
    assert(idx < darr->slots_occupied);
    return (char *)darr->slots + darr->slot_size * idx;
}

/**
 * Free all empty element slots in a dynamic array ("deflate").
 *
 * @param darr  The dynamic array to deflate.
 *
 * @return True if deflating succeeded, false if memory reallocation failed,
 *         and errno was set. 
 */
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
 * This deallocates all referenced memory.
 *
 * @param darr  The dynamic array to cleanup.
 */
static inline void
hdag_darr_cleanup(struct hdag_darr *darr)
{
    assert(hdag_darr_is_valid(darr));
    free(darr->slots);
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

#endif /* _HDAG_DARR_H */
