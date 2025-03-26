/**
 * Hash DAG - an abstract fanout array.
 */

#ifndef _HDAG_FANOUT_H
#define _HDAG_FANOUT_H

#include <hdag/arr.h>

/**
 * Check if a fanout array is valid.
 *
 * @param fanout_ptr    The pointer to the fanout array to check.
 * @param fanout_len    The length of the fanout array being checked.
 *
 * @return True if the fanout array is valid, false otherwise.
 */
static inline bool
hdag_fanout_is_valid(const uint32_t *fanout_ptr, size_t fanout_len)
{
    size_t i;
    uint32_t count;
    uint32_t prev_count;
    assert(fanout_ptr != NULL || fanout_len == 0);
    for (i = 0, prev_count = 0;
         i < fanout_len && (count = fanout_ptr[i]) >= prev_count;
         i++, prev_count = count);
    return i >= fanout_len;
}

/**
 * Check if a fanout array is valid.
 *
 * @param fanout    The fanout array to check.
 *
 * @return True if the fanout array is valid, false otherwise.
 */
static inline bool
hdag_fanout_arr_is_valid(const struct hdag_arr *fanout)
{
    return
        fanout != NULL &&
        hdag_arr_is_valid(fanout) &&
        fanout->slot_size == sizeof(uint32_t) &&
        hdag_fanout_is_valid(fanout->slots, fanout->slots_occupied);
}

/**
 * Check if a fanout array is empty.
 *
 * @param fanout_ptr    The pointer to the fanout array to check.
 * @param fanout_len    The length of the fanout array being checked.
 *
 * @return True if the fanout array is empty, false otherwise.
 */
static inline bool
hdag_fanout_is_empty(const uint32_t *fanout_ptr, size_t fanout_len)
{
    (void)fanout_ptr;
    return fanout_len == 0;
}

/**
 * Check if a fanout array is empty.
 *
 * @param fanout    The fanout array to check.
 *
 * @return True if the fanout array is empty, false otherwise.
 */
static inline bool
hdag_fanout_arr_is_empty(const struct hdag_arr *fanout)
{
    hdag_arr_is_valid(fanout);
    return hdag_arr_is_empty(fanout);
}

/**
 * Empty a fanout array.
 *
 * @param fanout    The fanout array to empty.
 */
static inline void
hdag_fanout_arr_empty(struct hdag_arr *fanout)
{
    assert(hdag_arr_is_valid(fanout));
    hdag_arr_empty(fanout);
    assert(hdag_fanout_arr_is_valid(fanout));
    assert(hdag_fanout_arr_is_empty(fanout));
}

/** An initializer for an all-zeroes fanout */
#define HDAG_FANOUT_ZERO   {0, }

/**
 * Check if a (non-empty) fanout array is zero.
 *
 * @param fanout_ptr    The pointer to the fanout array to check.
 * @param fanout_len    The length of the fanout array being checked.
 *
 * @return True if the fanout array is zeroed, false otherwise.
 */
static inline bool
hdag_fanout_is_zero(const uint32_t *fanout_ptr, size_t fanout_len)
{
    assert(hdag_fanout_is_valid(fanout_ptr, fanout_len));
    assert(!hdag_fanout_is_empty(fanout_ptr, fanout_len));
    return fanout_ptr[fanout_len - 1] == 0;
}

/**
 * Zero a (non-empty) fanout array.
 *
 * @param fanout_ptr    The pointer to the fanout array to zero.
 * @param fanout_len    The length of the fanout array being zeroed.
 */
static inline void
hdag_fanout_zero(uint32_t *fanout_ptr, size_t fanout_len)
{
    assert(fanout_ptr != NULL);
    assert(fanout_len != 0);
    memset(fanout_ptr, 0, sizeof(*fanout_ptr) * fanout_len);
    assert(hdag_fanout_is_zero(fanout_ptr, fanout_len));
}

/**
 * Check if a (non-empty) fanout array is zero.
 *
 * @param fanout    The fanout array to check.
 *
 * @return True if the fanout array is zero, false otherwise.
 */
static inline bool
hdag_fanout_arr_is_zero(const struct hdag_arr *fanout)
{
    assert(hdag_arr_is_valid(fanout));
    assert(!hdag_arr_is_empty(fanout));
    return hdag_fanout_is_zero(fanout->slots, fanout->slots_occupied);
}

/**
 * Zero a (non-empty) fanout array.
 *
 * @param fanout    The fanout array to zero.
 */
static inline void
hdag_fanout_arr_zero(struct hdag_arr *fanout)
{
    assert(hdag_arr_is_valid(fanout));
    assert(!hdag_arr_is_empty(fanout));
    hdag_fanout_zero(fanout->slots, fanout->slots_occupied);
    assert(hdag_fanout_arr_is_valid(fanout));
    assert(hdag_fanout_arr_is_zero(fanout));
}

/**
 * Get the value of an element in a (non-empty) fanout array.
 *
 * @param fanout    The fanout array to access.
 * @param idx       The index of the element to retrieve the value of.
 *
 * @return The retrieved element value.
 */
static inline uint32_t
hdag_fanout_arr_get(const struct hdag_arr *fanout, size_t idx)
{
    assert(hdag_fanout_arr_is_valid(fanout));
    assert(!hdag_fanout_arr_is_empty(fanout));
    return *HDAG_ARR_ELEMENT_CONST(fanout, uint32_t, idx);
}

/**
 * Set the value of an element in a (non-empty) fanout array.
 *
 * @param fanout    The fanout array to access.
 * @param idx       The index of the element to set the value of.
 * @param val       The value of the element to set.
 *
 * @return The retrieved element value.
 */
static inline void
hdag_fanout_arr_set(struct hdag_arr *fanout, size_t idx, uint32_t val)
{
    assert(hdag_arr_is_valid(fanout));
    assert(!hdag_fanout_arr_is_empty(fanout));
    *HDAG_ARR_ELEMENT(fanout, uint32_t, idx) = val;
}

#endif /* _HDAG_FANOUT_H */
