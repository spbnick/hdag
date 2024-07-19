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

#endif /* _HDAG_HASH_H */
