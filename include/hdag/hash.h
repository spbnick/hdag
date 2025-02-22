/**
 * Hash DAG node ID hash
 */

#ifndef _HDAG_HASH_H
#define _HDAG_HASH_H

#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <assert.h>

/**
 * Check if a hash length is valid (divisible by four and not zero).
 *
 * @param hash_len  The hash length to check.
 *
 * @return True if the hash length is valid, false otherwise.
 */
static inline bool
hdag_hash_len_is_valid(uint16_t hash_len)
{
    return hash_len != 0 && (hash_len & 3) == 0;
}

/**
 * Make sure a supplied and returned hash len is valid.
 *
 * @param hash_len  The hash length to check.
 *
 * @return The validated hash length.
 */
static inline uint16_t
hdag_hash_len_validate(uint16_t hash_len)
{
    assert(hdag_hash_len_is_valid(hash_len));
    return hash_len;
}

/**
 * Fill a hash with specified 32-bit unsigned integer.
 *
 * @param hash  The hash to fill.
 * @param len   The length of the hash.
 * @param fill  The value to fill the hash with.
 *
 * @return The filled "hash".
 */
static inline uint8_t *
hdag_hash_fill(uint8_t *hash, uint16_t len, uint32_t fill)
{
    size_t i;
    assert(hash != NULL);
    assert(hdag_hash_len_is_valid(len));
    for (i = 0; i < len >> 2; i++) {
        ((uint32_t *)hash)[i] = fill;
    }
    return hash;
}

/**
 * Check if a hash is filled with specified 32-bit unsigned integer.
 *
 * @param hash  The hash to check.
 * @param len   The length of the hash.
 * @param fill  The fill value to check against
 *
 * @return True if the node's hash is filled with the specified value,
 *         false otherwise.
 */
static inline bool
hdag_hash_is_filled(const uint8_t *hash, uint16_t len, uint32_t fill)
{
    size_t i;
    assert(hash != NULL);
    assert(hdag_hash_len_is_valid(len));
    for (i = 0; i < len >> 2; i++) {
        if (((const uint32_t *)hash)[i] != fill) {
            return false;
        }
    }
    return true;
}

/**
 * Compare two hashes lexicographically.
 *
 * @param a     The first hash to compare.
 * @param b     The second hash to compare.
 * @param len   Hash length (uintptr_t).
 *
 * @return -1, if a < b
 *          0, if a == b
 *          1, if a > b
 */
extern int hdag_hash_cmp(const void *a, const void *b, void *len);

#endif /* _HDAG_HASH_H */
