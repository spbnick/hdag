/**
 * Hash DAG - abstract sequence.
 */
#ifndef _HDAG_SEQ_H
#define _HDAG_SEQ_H

#include <hdag/res.h>
#include <hdag/misc.h>
#include <stdint.h>
#include <stdbool.h>

/* The forward declaration of the sequence */
struct hdag_seq;

/**
 * The prototype for a function resetting a sequence to the start position.
 *
 * @param seq  Sequence to reset.
 *
 * @return  Zero (HDAG_RES_OK), if the sequence was reset successfully.
 *          A negative number (a failure result) if reset has failed.
 */
typedef hdag_res (*hdag_seq_reset_fn)(struct hdag_seq *seq);

/**
 * The prototype for a function returning the next item from a sequence.
 *
 * @param seq   Sequence being traversed.
 * @param pitem Location for the pointer to the retrieved sequence item.
 *              Only valid until the next call.
 *
 * @return  One, if the item was retrieved successfully.
 *          Zero (HDAG_RES_OK), if there were no more items.
 *          A negative number (a failure result) if item retrieval has failed.
 *          The particular choice of values allows writing loop conditions
 *          like this: while (HDAG_RES_TRY(..._next(seq, &item))) {...}
 */
typedef hdag_res (*hdag_seq_next_fn)(struct hdag_seq *seq,
                                     void **pitem);

/** An abstract sequence */
struct hdag_seq {
    /**
     * The function resetting the sequence to the first item.
     * If NULL, the sequence is single-pass only.
     */
    hdag_seq_reset_fn   reset_fn;
#ifndef NDEBUG
    /**
     * True if the sequence items can be changed (are "variable").
     * False if not (are "constant"). In the latter case, only the
     * hdag_seq_next_const() function can be used to retrieve items.
     */
    bool                variable;
#endif
    /** The function retrieving the next item from the sequence */
    hdag_seq_next_fn    next_fn;
};

/**
 * Create an abstract sequence
 *
 * @param next_fn   The function retrieving the next item from the sequence.
 * @param variable  True if the items returned by the sequence can be
 *                  modified. False if not, and the items can only be
 *                  retrieved using hdag_seq_next_const().
 * @param reset_fn  The function resetting the sequence to the first item.
 *                  If NULL, the sequence is single-pass only.
 *
 * @return The created sequence.
 */
static inline struct hdag_seq
hdag_seq(hdag_seq_next_fn next_fn, bool variable, hdag_seq_reset_fn reset_fn)
{
    assert(next_fn != NULL);
#ifdef NDEBUG
    (void)variable;
#endif
    return (struct hdag_seq){
        .next_fn = next_fn,
#ifndef NDEBUG
        .variable = variable,
#endif
        .reset_fn = reset_fn,
    };
}

/**
 * An initializer for an abstract sequence
 *
 * @param _next_fn  The function retrieving the next item from the sequence.
 * @param _variable True if the items returned by the sequence can be
 *                  modified. False if not, and the items can only be
 *                  retrieved using hdag_seq_next_const().
 * @param _reset_fn The function resetting the sequence to the first item.
 *                  If NULL, the sequence is single-pass only.
 */
#define HDAG_SEQ(_next_fn, _variable, _reset_fn) \
    hdag_seq(_next_fn, _variable, _reset_fn)

/**
 * Check if a sequence is valid.
 *
 * @param seq  The sequence to check.
 *
 * @return True if the sequence is valid, false otherwise.
 */
static inline bool
hdag_seq_is_valid(const struct hdag_seq *seq)
{
    return seq != NULL && seq->next_fn != NULL;
}

/**
 * Check if a sequence is "variable", that is if the items returned from it
 * can be changed.
 *
 * @param seq  The sequence to check.
 *
 * @return True if the sequence is "variable", false otherwise.
 */
static inline bool
hdag_seq_is_variable(const struct hdag_seq *seq)
{
    assert(hdag_seq_is_valid(seq));
    return seq->variable;
}

/**
 * Check if a sequence is resettable (multi-pass).
 *
 * @param seq  The sequence to check.
 *
 * @return True if the sequence is resettable (can be rewound and reused),
 *         false if it's single-pass only.
 */
static inline bool
hdag_seq_is_resettable(const struct hdag_seq *seq)
{
    assert(hdag_seq_is_valid(seq));
    return seq->reset_fn != NULL;
}

/**
 * Reset a sequence to the start position.
 *
 * @param seq  The sequence to reset. Must be resettable.
 *
 * @return  Zero (HDAG_RES_OK), if the sequence was reset successfully.
 *          A negative number (a failure result) if reset has failed.
 */
static inline hdag_res
hdag_seq_reset(struct hdag_seq *seq)
{
    assert(hdag_seq_is_valid(seq));
    assert(hdag_seq_is_resettable(seq));
    return seq->reset_fn(seq);
}

/**
 * Get the next variable item out of a sequence, if possible.
 *
 * @param seq   The sequence to retrieve the next item from.
 *              Must be "variable".
 * @param pitem Location for the pointer to the retrieved item.
 *              Could be NULL to not have the pointer output.
 *              Can be modified. Only valid until the next call.
 *
 * @return  One, if the item was retrieved successfully.
 *          Zero (HDAG_RES_OK), if there were no more items.
 *          A negative number (a failure result) if item retrieval has failed.
 *          The particular choice of values allows writing loop conditions
 *          like this: while (HDAG_RES_TRY(..._next(seq, &item))) {...}
 */
static inline hdag_res
hdag_seq_next(struct hdag_seq *seq, void **pitem)
{
    void *item;
    assert(hdag_seq_is_valid(seq));
    assert(hdag_seq_is_variable(seq));
    return seq->next_fn(seq, pitem ? pitem : &item);
}

/**
 * Get the next const item out of a sequence, if possible.
 *
 * @param seq   The sequence to retrieve the next item from.
 * @param pitem Location for the pointer to the retrieved item.
 *              Could be NULL to not have the pointer output.
 *              Cannot be modified. Only valid until the next call.
 *
 * @return  One, if the item was retrieved successfully.
 *          Zero (HDAG_RES_OK), if there were no more items.
 *          A negative number (a failure result) if item retrieval has failed.
 *          The particular choice of values allows writing loop conditions
 *          like this: while (HDAG_RES_TRY(..._next(seq, &item))) {...}
 */
static inline hdag_res
hdag_seq_next_const(struct hdag_seq *seq, const void **pitem)
{
    void *item;
    assert(hdag_seq_is_valid(seq));
    return seq->next_fn(seq, pitem ? (void **)pitem : &item);
}

/** A next-item retrieval function which never returns anything */
[[nodiscard]]
extern hdag_res hdag_seq_empty_next(struct hdag_seq *seq, void **pitem);

/** A reset function which does nothing */
extern hdag_res hdag_seq_empty_reset(struct hdag_seq *seq);

/** An initializer for an empty sequence */
#define HDAG_SEQ_EMPTY \
    hdag_seq(hdag_seq_empty_next, true, hdag_seq_empty_reset)

/** An initializer for a constant empty sequence */
#define HDAG_SEQ_EMPTY_CONST \
    hdag_seq(hdag_seq_empty_next, false, hdag_seq_empty_reset)

/**
 * Compare two sequences.
 *
 * @param seq_a         The first sequence to compare.
 * @param seq_b         The second sequence to compare.
 * @param res_cmp_fn    The item comparison function.
 * @param res_cmp_data  The private data to pass to the item comparison
 *                      function.
 *
 * @return A universal comparison result code.
 */
extern hdag_res hdag_seq_cmp(struct hdag_seq *seq_a,
                             struct hdag_seq *seq_b,
                             hdag_res_cmp_fn res_cmp_fn,
                             void *res_cmp_data);

/**
 * Check if two sequences have any items in common.
 * The sequences must be sorted in ascending order, according to the
 * comparison function.
 *
 * @param seq_a         The first sequence to check.
 * @param seq_b         The second sequence to check.
 * @param res_cmp_fn    The item comparison function.
 * @param res_cmp_data  The private data to pass to the item comparison
 *                      function.
 *
 * @return A universal result code:
 *         0 - the sequences have no common hashes,
 *         1 - the sequences *do* have common hashes,
 *         or a hash retrieval fault.
 *         The particular choice of values allows writing conditions like
 *         this: if (HDAG_RES_TRY(..._intersecting(seq_a, seq_b))) {...}
 */
extern hdag_res hdag_seq_are_intersecting(struct hdag_seq *seq_a,
                                          struct hdag_seq *seq_b,
                                          hdag_res_cmp_fn res_cmp_fn,
                                          void *res_cmp_data);

#endif /* _HDAG_SEQ_H */
