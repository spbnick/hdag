/*
 * Hash DAG node ID hash
 */

#ifndef _HDAG_HASH_H
#define _HDAG_HASH_H

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
 */
static inline void
hdag_hash_fill(uint8_t *hash, uint16_t len, uint32_t fill)
{
    size_t i;
    assert(hash != NULL);
    assert(hdag_hash_len_is_valid(len));
    for (i = 0; i < len >> 2; i++) {
        ((uint32_t *)hash)[i] = fill;
    }
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
hdag_hash_is_filled(uint8_t *hash, uint16_t len, uint32_t fill)
{
    size_t i;
    assert(hash != NULL);
    assert(hdag_hash_len_is_valid(len));
    for (i = 0; i < len >> 2; i++) {
        if (((uint32_t *)hash)[i] != fill) {
            return false;
        }
    }
    return true;
}

#endif /* _HDAG_HASH_H */
