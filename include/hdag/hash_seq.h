/**
 * Hash DAG - (sized) hash sequence.
 */
#ifndef _HDAG_HASH_SEQ_H
#define _HDAG_HASH_SEQ_H

#include <hdag/sized_seq.h>
#include <hdag/hash.h>
#include <hdag/res.h>
#include <stdint.h>
#include <stdbool.h>

/** A hash sequence */
struct hdag_hash_seq {
    /** The base abstract sized sequence */
    struct hdag_sized_seq   base;
};

/**
 * Check if a hash sequence is valid.
 *
 * @param seq   The hash sequence to check.
 *
 * @return True if the sequence is valid, false otherwise.
 */
static inline bool
hdag_hash_seq_is_valid(const struct hdag_hash_seq *seq)
{
    return seq != NULL &&
        hdag_sized_seq_is_valid(&seq->base) && (
            seq->base.item_size == 0 ||
            hdag_hash_len_is_valid(seq->base.item_size)
        );
}

/**
 * Validate a hash sequence.
 *
 * @param seq   The hash sequence to validate.
 *
 * @return The validated sequence.
 */
static inline struct hdag_hash_seq*
hdag_hash_seq_validate(struct hdag_hash_seq *seq)
{
    assert(hdag_hash_seq_is_valid(seq));
    return seq;
}

/**
 * Validate a const hash sequence.
 *
 * @param seq   The hash sequence to validate.
 *
 * @return The validated sequence.
 */
static inline const struct hdag_hash_seq*
hdag_hash_seq_validate_const(const struct hdag_hash_seq *seq)
{
    assert(hdag_hash_seq_is_valid(seq));
    return seq;
}

/**
 * Check if a hash sequence is "void", that is never returns items.
 *
 * @param seq   The hash sequence to check.
 *
 * @return True if the sequence is void, false otherwise.
 */
static inline bool
hdag_hash_seq_is_void(const struct hdag_hash_seq *seq)
{
    assert(hdag_hash_seq_is_valid(seq));
    return seq->base.item_size == 0;
}

/** Cast a sized sequence pointer to a hash sequence pointer */
#define HDAG_HASH_SEQ_FROM_SIZED_SEQ(_seq) \
    hdag_hash_seq_validate(                                 \
        HDAG_CONTAINER_OF(struct hdag_hash_seq, base, _seq) \
    )

/** Cast an abstract sequence pointer to a hash sequence pointer */
#define HDAG_HASH_SEQ_FROM_SEQ(_seq) \
    HDAG_HASH_SEQ_FROM_SIZED_SEQ(HDAG_SIZED_SEQ_FROM_SEQ(_seq))

/** Cast a hash sequence pointer to a sized sequence pointer */
#define HDAG_HASH_SEQ_TO_SIZED_SEQ(_seq) \
    (&hdag_hash_seq_validate(_seq)->base)

/** Cast a const hash sequence pointer to a const sized sequence pointer */
#define HDAG_HASH_SEQ_TO_SIZED_SEQ_CONST(_seq) \
    (&hdag_hash_seq_validate_const(_seq)->base)

/** Cast a hash sequence pointer to an abstract sequence pointer */
#define HDAG_HASH_SEQ_TO_SEQ(_seq) \
    HDAG_SIZED_SEQ_TO_SEQ(HDAG_HASH_SEQ_TO_SIZED_SEQ(_seq))

/** Cast a const hash sequence pointer to a const abstract sequence pointer */
#define HDAG_HASH_SEQ_TO_SEQ_CONST(_seq) \
    HDAG_SIZED_SEQ_TO_SEQ_CONST(HDAG_HASH_SEQ_TO_SIZED_SEQ_CONST(_seq))

/**
 * Create a hash sequence.
 *
 * @param next_fn  The function retrieving the next item from the
 *                 sequence.
 * @param variable True if the items returned by the sequence can be
 *                 modified. False if not, and the items can only be
 *                 retrieved using hdag_seq_next_const().
 * @param reset_fn The function resetting the sequence to the first item.
 *                 If NULL, the sequence is single-pass only.
 * @param hash_len The length of the hashes in the sequence.
 */
static inline struct hdag_hash_seq
hdag_hash_seq(hdag_seq_next_fn next_fn, bool variable,
              hdag_seq_reset_fn reset_fn, size_t hash_len)
{
    assert(next_fn != NULL);
    assert(hash_len == 0 || hdag_hash_len_is_valid(hash_len));
    return (struct hdag_hash_seq){
        .base = hdag_sized_seq(next_fn, variable, reset_fn, hash_len),
    };
}

/**
 * An initializer for a hash sequence.
 *
 * @param _next_fn      The function retrieving the next item from the
 *                      sequence.
 * @param _variable     True if the items returned by the sequence can be
 *                      modified. False if not, and the items can only be
 *                      retrieved using hdag_seq_next_const().
 * @param _reset_fn     The function resetting the sequence to the first item.
 *                      If NULL, the sequence is single-pass only.
 * @param _hash_len     The length of the hashes in the sequence.
 */
#define HDAG_HASH_SEQ(_next_fn, _variable, _reset_fn, _hash_len) \
    hdag_hash_seq(_next_fn, _variable, _reset_fn, _hash_len)

/**
 * Reset a hash sequence to the start position.
 *
 * @param seq  The sequence to reset. Must be resettable (or void).
 */
static inline hdag_res
hdag_hash_seq_reset(struct hdag_hash_seq *seq)
{
    assert(hdag_hash_seq_is_valid(seq));
    return hdag_sized_seq_reset(&seq->base);
}

/**
 * Get the next hash out of a sequence, if possible.
 *
 * @param seq   The sequence to retrieve the next hash from.
 * @param phash Location for the pointer to the retrieved hash.
 *              Could be NULL to not have the pointer output.
 *              Only valid until the next call.
 *
 * @return  One, if the hash was retrieved successfully.
 *          Zero (HDAG_RES_OK), if there were no more hashes.
 *          A negative number (a failure result) if hash retrieval has failed.
 *          The particular choice of values allows writing loop conditions
 *          like this: while (HDAG_RES_TRY(..._next(seq, &item))) {...}
 */
static inline hdag_res
hdag_hash_seq_next(struct hdag_hash_seq *seq, const uint8_t **phash)
{
    assert(hdag_hash_seq_is_valid(seq));
    return hdag_sized_seq_next(&seq->base, (void **)phash);
}

/**
 * Get the next const hash out of a sequence, if possible.
 *
 * @param seq   The sequence to retrieve the next hash from.
 * @param phash Location for the pointer to the retrieved item.
 *              Could be NULL to not have the pointer output.
 *              Only valid until the next call.
 *
 * @return  One, if the item was retrieved successfully.
 *          Zero (HDAG_RES_OK), if there were no more items.
 *          A negative number (a failure result) if item retrieval has failed.
 *          The particular choice of values allows writing loop conditions
 *          like this: while (HDAG_RES_TRY(..._next(seq, &item))) {...}
 */
static inline hdag_res
hdag_hash_seq_next_const(struct hdag_hash_seq *seq, const uint8_t **phash)
{
    assert(hdag_hash_seq_is_valid(seq));
    return hdag_sized_seq_next_const(&seq->base, (const void **)phash);
}

/**
 * An initializer for an empty hash sequence
 *
 * @param _hash_len The length of the hashes in the sequence
 *                  (if they were returned).
 */
#define HDAG_HASH_SEQ_EMPTY(_hash_len) \
    hdag_hash_seq(hdag_seq_empty_next, true, hdag_seq_empty_reset, _hash_len)

/**
 * An initializer for a constant empty hash sequence
 *
 * @param _hash_len The length of the hashes in the sequence
 *                  (if they were returned).
 */
#define HDAG_HASH_SEQ_EMPTY_CONST(_hash_len) \
    hdag_hash_seq(hdag_seq_empty_next, false, hdag_seq_empty_reset, _hash_len)

/** An initializer for a void hash sequence */
#define HDAG_HASH_SEQ_VOID HDAG_HASH_SEQ_EMPTY(0)

/** An initializer for a constant void hash sequence */
#define HDAG_HASH_SEQ_VOID_CONST HDAG_HASH_SEQ_EMPTY_CONST(0)

#endif /* _HDAG_HASH_SEQ_H */
