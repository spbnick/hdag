/*
 * Hash DAG (lexicographically) sorted hash array
 */

#ifndef _HDAG_HASHES_H
#define _HDAG_HASHES_H

#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <assert.h>
#include <hdag/hash_seq.h>
#include <hdag/darr.h>

/**
 * Check if a hash array is valid, including that it's sorted and
 * deduplicated.
 *
 * @param hashes    The hash array to check.
 * @param hash_len  The length of each hash in the array.
 *                  Must be valid according to hdag_hash_len_is_valid().
 * @param hash_num  The number of hashes in the array.
 *
 * @return True if the array is valid, false otherwise.
 */
extern bool hdag_hashes_are_valid(const uint8_t *hashes,
                                  uint16_t hash_len, size_t hash_num);

/**
 * Find a hash in a slice of a hash array.
 *
 * @param hashes    The hash array to search.
 *                  Must be sorted lexicographically.
 * @param hash_len  The length of each hash in the array.
 *                  Must be valid according to hdag_hash_len_is_valid().
 * @param start_idx The start index of the hashes array slice to search in.
 *                  Must be less than INT32_MAX.
 * @param end_idx   The end index of the hashes array slice to search in
 *                  (pointing right after the last hash of the slice).
 *                  Must be greater than or equal to start_idx.
 *                  Must be less than INT32_MAX.
 * @param hash_ptr  Pointer to the hash to find in the array.
 * @param phash_idx Location for the index on which the search has stopped.
 *                  If the hash was found, it will point to it.
 *                  If not, it will point to the closest hash above it,
 *                  that is the place where the hash should've been.
 *                  Can be NULL to skip index output.
 *
 * @return True if the hash was found, false if not.
 */
extern bool hdag_hashes_slice_find(const uint8_t *hashes,
                                   uint16_t hash_len,
                                   size_t start_idx, size_t end_idx,
                                   const uint8_t *hash_ptr,
                                   size_t *phash_idx);

/**
 * Find a hash in a hash array.
 *
 * @param hashes    The hash array to search.
 *                  Must be sorted lexicographically.
 * @param hash_len  The length of each hash in the array.
 *                  Must be valid according to hdag_hash_len_is_valid().
 * @param hash_num  Number of hashes in the array.
 * @param hash_ptr  Pointer to the hash to find in the array.
 * @param phash_idx Location for the index on which the search has stopped.
 *                  If the hash was found, it will point to it.
 *                  If not, it will point to the closest hash above it,
 *                  that is the place where the hash should've been.
 *                  Can be NULL to skip index output.
 *
 * @return True if the hash was found, false if not.
 */
static inline bool
hdag_hashes_find(const uint8_t *hashes,
                 uint16_t hash_len,
                 size_t hash_num,
                 const uint8_t *hash_ptr,
                 size_t *phash_idx)
{
    assert(hashes != NULL || hash_num == 0);
    assert(hdag_hash_len_is_valid(hash_len));
    assert(hash_ptr != NULL);
    return hdag_hashes_slice_find(hashes, hash_len, 0, hash_num, hash_ptr,
                                  phash_idx);
}

/**
 * Check if a dynamic hash array is valid, including that it's sorted and
 * deduplicated.
 *
 * @param hashes    The dynamic hash array to check.
 *
 * @return True if the dynamic hash array is valid, false otherwise.
 */
static inline bool
hdag_hashes_darr_is_valid(const struct hdag_darr *hashes)
{
    return hdag_darr_is_valid(hashes) &&
        hdag_hashes_are_valid(hashes->slots,
                              hashes->slot_size, hashes->slots_occupied);
}

/**
 * Find a hash in a dynamic hash array.
 *
 * @param hashes    The dynamic hash array to search.
 *                  Must be sorted lexicographically.
 * @param hash_ptr  Pointer to the hash to find in the array.
 * @param phash_idx Location for the index on which the search has stopped.
 *                  If the hash was found, it will point to it.
 *                  If not, it will point to the closest hash above it,
 *                  that is the place where the hash should've been.
 *                  Can be NULL to skip index output.
 *
 * @return True if the hash was found, false if not.
 */
static inline bool
hdag_hashes_darr_find(const struct hdag_darr *hashes,
                      const uint8_t *hash_ptr,
                      size_t *phash_idx)
{
    assert(hdag_darr_is_valid(hashes));
    assert(hdag_hash_len_is_valid(hashes->slot_size));
    assert(hash_ptr != NULL);
    return hdag_hashes_find(hashes->slots, hashes->slot_size, hashes->slots_occupied,
                            hash_ptr, phash_idx);
}

/** Hash array sequence (resettable) */
struct hdag_hashes_seq {
    /** The base abstract hash sequence */
    struct hdag_hash_seq base;
    /** The array of hashes */
    const uint8_t *hashes;
    /** Number of hashes in the array */
    size_t hash_num;
    /** The index of the next hash to return */
    size_t next_idx;
};

/** The next-hash retrieval function of a hash array sequence */
[[nodiscard]]
extern hdag_res hdag_hashes_seq_next(
                    struct hdag_hash_seq *hash_seq,
                    uint8_t *phash);

/** The reset function of a hash array sequence */
extern void hdag_hashes_seq_reset(
                    struct hdag_hash_seq *hash_seq);

/**
 * Initialize a (resettable) hash array sequence.
 *
 * @param pseq      The location of the hash array sequence to initialize.
 * @param hash_len  The length of each hash in the array.
 * @param hashes Pointer to the hash array to iterate over.
 * @param hash_num The number of hashes in the array.
 *
 * @return The abstract sequence pointer.
 */
static inline struct hdag_hash_seq *
hdag_hashes_seq_init(struct hdag_hashes_seq *pseq,
                     uint16_t hash_len,
                     const uint8_t *hashes,
                     size_t hash_num)
{
    assert(pseq != NULL);
    assert(hdag_hash_len_is_valid(hash_len));
    assert(hashes != NULL || hash_num == 0);
    *pseq = (struct hdag_hashes_seq){
        .base = {
            .hash_len = hash_len,
            .reset_fn = hdag_hashes_seq_reset,
            .next_fn = hdag_hashes_seq_next,
        },
        .hashes = hashes,
        .hash_num = hash_num,
        .next_idx = 0,
    };
    return &pseq->base;
}

#endif /* _HDAG_HASHES_H */
