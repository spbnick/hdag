/**
 * Hash DAG - sequence specifying item size.
 */
#ifndef _HDAG_SIZED_SEQ_H
#define _HDAG_SIZED_SEQ_H

#include <hdag/seq.h>
#include <hdag/res.h>
#include <stdint.h>
#include <stdbool.h>

/** A sequence with item size specified */
struct hdag_sized_seq {
    /** The base abstract sequence */
    struct hdag_seq base;
    /**
     * The size of returned items, bytes,
     * or zero to signify an always-empty "void" sequence.
     */
    size_t          item_size;
};

/**
 * Check if a sized sequence is valid.
 *
 * @param seq   The sized sequence to check.
 *
 * @return True if the sequence is valid, false otherwise.
 */
static inline bool
hdag_sized_seq_is_valid(const struct hdag_sized_seq *seq)
{
    return seq != NULL && hdag_seq_is_valid(&seq->base);
}

/**
 * Validate a sized sequence.
 *
 * @param seq   The sized sequence to validate.
 *
 * @return The validated sequence.
 */
static inline struct hdag_sized_seq*
hdag_sized_seq_validate(struct hdag_sized_seq *seq)
{
    assert(hdag_sized_seq_is_valid(seq));
    return seq;
}

/**
 * Validate a const sized sequence.
 *
 * @param seq   The sized sequence to validate.
 *
 * @return The validated sequence.
 */
static inline const struct hdag_sized_seq*
hdag_sized_seq_validate_const(const struct hdag_sized_seq *seq)
{
    assert(hdag_sized_seq_is_valid(seq));
    return seq;
}

/**
 * Check if a sized sequence is "void", that is cannot return items,
 * or be reset.
 *
 * @param seq   The sized sequence to check.
 *
 * @return True if the sequence is void, false otherwise.
 */
static inline bool
hdag_sized_seq_is_void(const struct hdag_sized_seq *seq)
{
    assert(hdag_sized_seq_is_valid(seq));
    return seq->item_size == 0;
}

/** Cast an abstract sequence pointer to a sized sequence pointer */
#define HDAG_SIZED_SEQ_FROM_SEQ(_seq) \
    hdag_sized_seq_validate(                                    \
        HDAG_CONTAINER_OF(struct hdag_sized_seq, base, _seq)    \
    )

/** Cast a sized sequence pointer to an abstract sequence pointer */
#define HDAG_SIZED_SEQ_TO_SEQ(_seq) \
    (&hdag_sized_seq_validate(_seq)->base)

/**
 * Cast a const sized sequence pointer to a const abstract sequence pointer
 */
#define HDAG_SIZED_SEQ_TO_SEQ_CONST(_seq) \
    (&hdag_sized_seq_validate_const(_seq)->base)

/**
 * Create a sized sequence.
 *
 * @param next_fn   The function retrieving the next item from the
 *                  sequence.
 * @param variable  True if the items returned by the sequence can be
 *                  modified. False if not, and the items can only be
 *                  retrieved using hdag_seq_next_const().
 * @param reset_fn  The function resetting the sequence to the first item.
 *                  If NULL, the sequence is single-pass only.
 * @param item_size The size of sequence items, bytes.
 */
static inline struct hdag_sized_seq
hdag_sized_seq(hdag_seq_next_fn next_fn, bool variable,
               hdag_seq_reset_fn reset_fn, size_t item_size)
{
    assert(next_fn != NULL);
    return (struct hdag_sized_seq){
        .base = hdag_seq(next_fn, variable, reset_fn),
        .item_size = item_size,
    };
}

/**
 * An initializer for a sized sequence
 *
 * @param _next_fn      The function retrieving the next item from the
 *                      sequence.
 * @param _variable     True if the items returned by the sequence can be
 *                      modified. False if not, and the items can only be
 *                      retrieved using hdag_seq_next_const().
 * @param _reset_fn     The function resetting the sequence to the first item.
 *                      If NULL, the sequence is single-pass only.
 * @param _item_size    The size of sequence items, bytes.
 */
#define HDAG_SIZED_SEQ(_next_fn, _variable, _reset_fn, _item_size) \
    hdag_sized_seq(_next_fn, _variable, _reset_fn, _item_size)

/**
 * Reset a sized sequence to the start position.
 *
 * @param seq  The sequence to reset. Must be resettable (or void).
 */
static inline hdag_res
hdag_sized_seq_reset(struct hdag_sized_seq *seq)
{
    assert(hdag_sized_seq_is_valid(seq));
    if (hdag_sized_seq_is_void(seq)) {
        return HDAG_RES_OK;
    }
    return hdag_seq_reset(&seq->base);
}

/**
 * Get the next item out of a sized sequence, if possible.
 *
 * @param seq   The sequence to retrieve the next item from.
 * @param pitem Location for the pointer to the retrieved item.
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
hdag_sized_seq_next(struct hdag_sized_seq *seq, void **pitem)
{
    assert(hdag_sized_seq_is_valid(seq));
    if (hdag_sized_seq_is_void(seq)) {
        return HDAG_RES_OK;
    }
    return hdag_seq_next(&seq->base, pitem);
}

/**
 * Get the next const item out of a sized sequence, if possible.
 *
 * @param seq   The sequence to retrieve the next item from.
 * @param pitem Location for the pointer to the retrieved item.
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
hdag_sized_seq_next_const(struct hdag_sized_seq *seq, const void **pitem)
{
    assert(hdag_sized_seq_is_valid(seq));
    if (hdag_sized_seq_is_void(seq)) {
        return HDAG_RES_OK;
    }
    return hdag_seq_next_const(&seq->base, pitem);
}

/**
 * An initializer for an empty sized sequence.
 *
 * @param _item_size    The size of sequence items, bytes.
 */
#define HDAG_SIZED_SEQ_EMPTY(_item_size) \
    hdag_sized_seq(hdag_seq_empty_next, true, \
                   hdag_seq_empty_reset, _item_size)

/** An initializer for a void sized sequence */
#define HDAG_SIZED_SEQ_VOID HDAG_SIZED_SEQ_EMPTY(0)

/**
 * Compare two sized sequences using memcmp.
 *
 * @param seq_a     The first sequence to compare.
 * @param seq_b     The second sequence to compare.
 *
 * @return A universal result code:
 *         1 - seq_a < seq_b,
 *         2 - seq_a == seq_b,
 *         3 - seq_a > seq_b,
 *         or a hash retrieval fault.
 */
static inline hdag_res
hdag_sized_seq_mem_cmp(struct hdag_sized_seq *seq_a,
                       struct hdag_sized_seq *seq_b)
{
    assert(hdag_sized_seq_is_valid(seq_a));
    assert(hdag_sized_seq_is_valid(seq_b));
    if (seq_a->item_size < seq_b->item_size) {
        return 1;
    }
    if (seq_a->item_size > seq_b->item_size) {
        return 3;
    }
    /* If both sequences are void */
    if (seq_a->item_size == 0) {
        return 2;
    }
    return hdag_seq_cmp(
        &seq_a->base,
        &seq_b->base,
        hdag_res_cmp_mem,
        (void *)(uintptr_t)seq_a->item_size
    );
}

/**
 * Check if two sized sequences have any items in common, using memcmp.
 * The sequences must be sorted in ascending order, according to memcmp.
 *
 * @param seq_a     The first sequence to check.
 * @param seq_b     The second sequence to check.
 *
 * @return A universal result code:
 *         0 - the sequences have no common hashes,
 *         1 - the sequences *do* have common hashes,
 *         or a hash retrieval fault.
 *         The particular choice of values allows writing conditions like
 *         this: if (HDAG_RES_TRY(..._intersecting(seq_a, seq_b))) {...}
 */
static inline hdag_res
hdag_sized_seq_are_mem_intersecting(struct hdag_sized_seq *seq_a,
                                    struct hdag_sized_seq *seq_b)
{
    assert(hdag_sized_seq_is_valid(seq_a));
    assert(hdag_sized_seq_is_valid(seq_b));
    if (seq_a->item_size != seq_b->item_size || seq_a->item_size == 0) {
        return 0;
    }
    return hdag_seq_are_intersecting(
        &seq_a->base,
        &seq_b->base,
        hdag_res_cmp_mem,
        (void *)(uintptr_t)seq_a->item_size
    );
}

#endif /* _HDAG_SIZED_SEQ_H */
