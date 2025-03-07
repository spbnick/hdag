/**
 * Hash DAG hash sequence.
 */
#ifndef _HDAG_HASH_SEQ_H
#define _HDAG_HASH_SEQ_H

#include <hdag/hash.h>
#include <hdag/res.h>
#include <stdint.h>
#include <stdbool.h>

/* The forward declaration of the hash sequence */
struct hdag_hash_seq;

/**
 * The prototype for a function resetting a hash sequence to the start
 * position.
 *
 * @param hash_seq  Hash sequence to reset.
 */
typedef void (*hdag_hash_seq_reset_fn)(struct hdag_hash_seq *hash_seq);

/**
 * The prototype for a function returning the next hash from a sequence.
 *
 * @param hash_seq  Hash sequence being traversed.
 * @param phash     Location for the pointer to the retrieved hash.
 *                  Only valid until the next call.
 *
 * @return  Zero (HDAG_RES_OK) if the hash was retrieved successfully.
 *          A positive number if there were no more hashes.
 *          A negative number (a failure result) if hash retrieval has failed.
 */
typedef hdag_res (*hdag_hash_seq_next_fn)(struct hdag_hash_seq *hash_seq,
                                          const uint8_t **phash);

/** A hash sequence */
struct hdag_hash_seq {
    /** The length of returned hashes, bytes */
    uint16_t                hash_len;
    /**
     * The function resetting the sequence to the first element.
     * If NULL, the sequence is single-pass only.
     */
    hdag_hash_seq_reset_fn  reset_fn;
    /** The function retrieving the next hash from the sequence */
    hdag_hash_seq_next_fn   next_fn;
};

/**
 * Check if a hash sequence is valid.
 *
 * @param hash_seq  The hash sequence to check.
 *
 * @return True if the sequence is valid, false otherwise.
 */
static inline bool
hdag_hash_seq_is_valid(const struct hdag_hash_seq *hash_seq)
{
    return hash_seq != NULL &&
        hdag_hash_len_is_valid(hash_seq->hash_len) &&
        hash_seq->next_fn != NULL;
}

/**
 * Check if a hash sequence is resettable (multi-pass).
 *
 * @param hash_seq  The hash sequence to check.
 *
 * @return True if the sequence is resettable (can be rewound and reused),
 *         false if it's single-pass only.
 */
static inline bool
hdag_hash_seq_is_resettable(const struct hdag_hash_seq *hash_seq)
{
    assert(hdag_hash_seq_is_valid(hash_seq));
    return hash_seq->reset_fn != NULL;
}

/**
 * Reset a hash sequence to the start position.
 *
 * @param hash_seq  Hash sequence to reset. Must be resettable.
 */
static inline void
hdag_hash_seq_reset(struct hdag_hash_seq *hash_seq)
{
    assert(hdag_hash_seq_is_valid(hash_seq));
    assert(hdag_hash_seq_is_resettable(hash_seq));
    hash_seq->reset_fn(hash_seq);
}

/**
 * Get the next hash out of a sequence, if possible.
 *
 * @param hash_seq  The sequence to retrieve the next hash from.
 * @param phash     Location for the pointer to the retrieved hash.
 *                  Could be NULL to not have the pointer output.
 *                  Only valid until the next call.
 *
 * @return  Zero (HDAG_RES_OK) if the hash was retrieved successfully.
 *          A positive number if there were no more hashes.
 *          A negative number (a failure result) if hash retrieval has failed.
 */
static inline hdag_res
hdag_hash_seq_next(struct hdag_hash_seq *hash_seq,
                   const uint8_t **phash)
{
    const uint8_t *hash;
    assert(hdag_hash_seq_is_valid(hash_seq));
    return hash_seq->next_fn(hash_seq, phash ? phash : &hash);
}

/** A next-hash retrieval function which never returns hashes */
[[nodiscard]]
extern hdag_res hdag_hash_seq_empty_next(
                    struct hdag_hash_seq *hash_seq,
                    const uint8_t **phash);

/** A reset function which does nothing */
extern void hdag_hash_seq_empty_reset(
                    struct hdag_hash_seq *hash_seq);

/**
 * An initializer for an empty hash sequence
 *
 * @param _hash_len The length of the hashes in the sequence
 *                  (if they were returned).
 */
#define HDAG_HASH_SEQ_EMPTY(_hash_len) (struct hdag_hash_seq){ \
    .hash_len = hdag_hash_len_validate(_hash_len),              \
    .reset_fn = hdag_hash_seq_empty_reset,                      \
    .next_fn = hdag_hash_seq_empty_next                         \
}

/**
 * Compare two hash sequences with the same hash length.
 *
 * @param seq_a    The first sequence to compare.
 * @param seq_b    The second sequence to compare.
 *
 * @return A universal result code:
 *         1 - seq_a < seq_b,
 *         2 - seq_a == seq_b,
 *         3 - seq_a > seq_b,
 *         or a hash retrieval fault.
 */
extern hdag_res hdag_hash_seq_cmp(struct hdag_hash_seq *seq_a,
                                  struct hdag_hash_seq *seq_b);

/**
 * Check if two lexicographically-ordered hash sequences have any hashes in
 * common. Both must have the same hash length.
 *
 * @param seq_a The first sequence to check.
 * @param seq_b The second sequence to check.
 *
 * @return A universal result code:
 *         0 - the sequences have no common hashes,
 *         1 - the sequences *do* have common hashes,
 *         or a hash retrieval fault.
 */
extern hdag_res hdag_hash_seq_are_intersecting(struct hdag_hash_seq *seq_a,
                                               struct hdag_hash_seq *seq_b);

#endif /* _HDAG_HASH_SEQ_H */
