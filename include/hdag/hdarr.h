/**
 * Hash DAG dynamic hash array
 */

#ifndef _HDAG_HDARR_H
#define _HDAG_HDARR_H

#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <assert.h>
#include <hdag/hash_seq.h>
#include <hdag/darr.h>

/**
 * Check if a dynamic hash array slice is valid.
 *
 * @param hdarr The dynamic hash array containing the slice to check.
 * @param start The start index of the slice.
 * @param end   The end index of the slice (pointing right after the last
 *              hash of the slice). Must be greater than or equal to start,
 *              and less than or equal to hdarr->slots_occupied.
 *
 * @return True if the dynamic hash array slice is valid, false otherwise.
 */
static inline bool
hdag_hdarr_slice_is_valid(const struct hdag_darr *hdarr,
                          size_t start, size_t end)
{
    return
        hdag_darr_slice_is_valid(hdarr, start, end) &&
        hdarr->slot_size <= UINT16_MAX &&
        (
            hdarr->slot_size == 0 ?
                hdarr->slots_occupied == 0 :
                hdag_hash_len_is_valid(hdarr->slot_size)
        );
}

/**
 * Check if a dynamic hash array is valid.
 *
 * @param hdarr The dynamic hash array to check.
 *
 * @return True if the dynamic hash array is valid, false otherwise.
 */
static inline bool
hdag_hdarr_is_valid(const struct hdag_darr *hdarr)
{
    return hdag_hdarr_slice_is_valid(
        hdarr, 0, hdag_darr_occupied_slots(hdarr)
    );
}

/**
 * Check if a slice of a dynamic hash array is sorted according to
 * a specification.
 *
 * @param darr      The dynamic array containing the slice to check.
 *                  Cannot be void unless both start and end are zero.
 * @param start     The index of the first element of the slice to be checked.
 * @param end       The index of the first element *after* the slice to be
 *                  checked.
 * @param cmp_min   The minimum comparison result of adjacent elements
 *                  [-1, cmp_max].
 * @param cmp_max   The maximum comparison result of adjacent elements
 *                  [cmp_min, 1].
 *
 * @return True if the slice is sorted as specified.
 */
static inline bool
hdag_hdarr_slice_is_sorted_as(const struct hdag_darr *hdarr,
                              size_t start, size_t end,
                              int cmp_min, int cmp_max)
{
    assert(hdag_hdarr_slice_is_valid(hdarr, start, end));
    return hdag_darr_slice_is_sorted_as(hdarr, start, end,
                                        hdag_hash_cmp,
                                        (void *)(uintptr_t)hdarr->slot_size,
                                        cmp_min, cmp_max);
}

/**
 * Check if a slice of a dynamic hash array is sorted in ascending order.
 *
 * @param darr  The dynamic array containing the slice to check.
 *              Cannot be void unless both start and end are zero.
 * @param start The index of the first element of the slice to be checked.
 * @param end   The index of the first element *after* the slice to be
 *              checked.
 *
 * @return True if the slice is sorted in ascending order.
 */
static inline bool
hdag_hdarr_slice_is_sorted(const struct hdag_darr *hdarr,
                           size_t start, size_t end)
{
    assert(hdag_hdarr_slice_is_valid(hdarr, start, end));
    return hdag_hdarr_slice_is_sorted_as(hdarr, start, end, -1, 0);
}

/**
 * Check if a dynamic hash array slice is sorted and deduplicated.
 *
 * @param hdarr The dynamic hash array containing the slice to check.
 * @param start The start index of the slice.
 * @param end   The end index of the slice (pointing right after the last
 *              hash of the slice).
 *
 * @return True if the slice is sorted and deduped, false otherwise.
 */
static inline bool
hdag_hdarr_slice_is_sorted_and_deduped(const struct hdag_darr *hdarr,
                                       size_t start, size_t end)
{
    assert(hdag_hdarr_slice_is_valid(hdarr, start, end));
    return hdag_hdarr_slice_is_sorted_as(hdarr, start, end, -1, -1);
}

/**
 * Find a hash in a slice of a dynamic hash array.
 *
 * @param hdarr     The dynamic hash array to search.
 *                  Must be sorted and deduplicated.
 * @param start     The start index of the array slice to search in.
 * @param end       The end index of the array slice to search in
 *                  (pointing right after the last hash of the slice).
 *                  Must be greater than or equal to start.
 * @param hash      Pointer to the hash to find in the array.
 * @param phash_idx Location for the index on which the search has stopped.
 *                  If the hash was found, it will point to it.
 *                  If not, it will point to the closest hash above it,
 *                  that is the place where the hash should've been.
 *                  Can be NULL to skip index output.
 *
 * @return True if the hash was found, false if not.
 */
extern bool hdag_hdarr_slice_find(const struct hdag_darr *hdarr,
                                  size_t start, size_t end,
                                  const uint8_t *hash,
                                  size_t *phash_idx);

/**
 * Find a hash in a dynamic hash array.
 *
 * @param hdarr     The dynamic hash array to search.
 *                  Must be sorted and deduplicated.
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
hdag_hdarr_find(const struct hdag_darr *hdarr,
                const uint8_t *hash_ptr,
                size_t *phash_idx)
{
    hdag_hdarr_is_valid(hdarr);
    return hdag_hdarr_slice_find(hdarr, 0, hdarr->slots_occupied,
                                 hash_ptr, phash_idx);
}

/**
 * Sort a slice of a dynamic array of hashes.
 *
 * @param hdarr The dynamic hash array containing the slice to sort.
 * @param start The start index of the slice.
 * @param end   The end index of the slice (pointing right after the last
 *              hash of the slice).
 */
static inline void
hdag_hdarr_slice_sort(struct hdag_darr *hdarr, size_t start, size_t end)
{
    assert(hdag_hdarr_slice_is_valid(hdarr, start, end));
    hdag_darr_slice_sort(hdarr, start, end,
                         hdag_hash_cmp,
                         (void *)(uintptr_t)hdarr->slot_size);
    assert(hdag_hdarr_slice_is_sorted(hdarr, start, end));
}

/**
 * Deduplicate a slice of a dynamic hash array (remove adjacent equal
 * elements). Do not remove the deduplicated hashes, let the caller do it.
 *
 * @param hdarr The dynamic hash array containing the slice to deduplicate.
 * @param start The index of the first element of the slice.
 * @param end   The index of the first element *after* the slice.
 *
 * @return The new end index of the slice, after deduplicating.
 */
static inline size_t
hdag_hdarr_slice_dedup(struct hdag_darr *hdarr, size_t start, size_t end)
{
    assert(hdag_hdarr_slice_is_valid(hdarr, start, end));
    return hdag_darr_slice_dedup(hdarr, start, end,
                                 hdag_hash_cmp,
                                 (void *)(uintptr_t)hdarr->slot_size);
}

/**
 * Sort and deduplicate a slice of a dynamic hash array.
 * Do not remove the deduplicated hashes, let the caller do it.
 *
 * @param hdarr The dynamic hash array containing the slice.
 * @param start The index of the first element of the slice.
 * @param end   The index of the first element *after* the slice.
 *
 * @return The new end index of the slice, after deduplicating.
 */
static inline size_t
hdag_hdarr_slice_sort_and_dedup(struct hdag_darr *hdarr,
                                size_t start, size_t end)
{
    assert(hdag_hdarr_slice_is_valid(hdarr, start, end));
    hdag_hdarr_slice_sort(hdarr, start, end);
    return hdag_hdarr_slice_dedup(hdarr, start, end);
}

/**
 * Sort a dynamic array of hashes.
 *
 * @param hdarr The dynamic array to sort.
 */
static inline void
hdag_hdarr_sort(struct hdag_darr *hdarr)
{
    assert(hdag_hdarr_is_valid(hdarr));
    hdag_hdarr_slice_sort(hdarr, 0, hdarr->slots_occupied);
}

/** Hash array sequence (resettable) */
struct hdag_hdarr_seq {
    /** The base abstract hash sequence */
    struct hdag_hash_seq base;
    /** The dynamic array of hashes to iterate over */
    struct hdag_darr *hdarr;
    /** The index of the next hash to return */
    size_t next_idx;
};

/** The next-hash retrieval function of a dynamic hash array sequence */
[[nodiscard]]
extern hdag_res hdag_hdarr_seq_next(struct hdag_hash_seq *base_seq,
                                    const uint8_t **phash);

/** The reset function of a dynamic hash array sequence */
extern void hdag_hdarr_seq_reset(struct hdag_hash_seq *base_seq);

/**
 * Initialize a (resettable) dynamic hash array sequence.
 *
 * @param pseq  The location of the hash array sequence to initialize.
 * @param hdarr The dynamic hash array to iterate over.
 *
 * @return The abstract sequence pointer.
 */
static inline struct hdag_hash_seq *
hdag_hdarr_seq_init(struct hdag_hdarr_seq *pseq,
                    struct hdag_darr *hdarr)
{
    assert(pseq != NULL);
    assert(hdag_hdarr_is_valid(hdarr));
    *pseq = (struct hdag_hdarr_seq){
        .base = {
            .hash_len = hdarr->slot_size,
            .reset_fn = hdag_hdarr_seq_reset,
            .next_fn = hdag_hdarr_seq_next,
        },
        .hdarr = hdarr,
        .next_idx = 0,
    };
    return &pseq->base;
}

/** An initializer for a dynamic hash array sequence */
#define HDAG_HDARR_SEQ(_hdarr) \
    (*(hdag_hdarr_seq_init(&(struct hdag_hdarr_seq){}, _hdarr)))

#endif /* _HDAG_HDARR_H */
