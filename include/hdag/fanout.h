/*
 * Hash DAG - an abstract fanout array.
 */

#ifndef _HDAG_FANOUT_H
#define _HDAG_FANOUT_H

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

/** An initializer for an empty fanout array */
#define HDAG_FANOUT_EMPTY   {}

/**
 * Empty a fanout array.
 *
 * @param fanout_ptr    The pointer to the fanout array to empty.
 * @param fanout_len    The length of the fanout array being emptied.
 */
static inline void
hdag_fanout_empty(uint32_t *fanout_ptr, size_t fanout_len)
{
    assert(fanout_ptr != NULL || fanout_len == 0);
    memset(fanout_ptr, 0, sizeof(*fanout_ptr) * fanout_len);
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
    assert(hdag_fanout_is_valid(fanout_ptr, fanout_len));
    return fanout_len == 0 || fanout_ptr[fanout_len - 1] == 0;
}

#endif /* _HDAG_FANOUT_H */
