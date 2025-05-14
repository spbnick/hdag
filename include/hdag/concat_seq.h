/*
 * Hash DAG - concatenated sequence
 */

#ifndef _HDAG_CONCAT_SEQ_H
#define _HDAG_CONCAT_SEQ_H

#include <hdag/seq.h>
#include <hdag/res.h>
#include <stdint.h>
#include <stdbool.h>

/** A concatenated sequence */
struct hdag_concat_seq {
    /** The base abstract sequence */
    struct hdag_seq     base;
    /** The sequence returning sequences to concatenate */
    struct hdag_seq    *seq_seq;
    /** The current sequence being traversed */
    struct hdag_seq    *seq;
};

/** The next-item retrieval function for a concatenated sequence */
[[nodiscard]]
extern hdag_res hdag_concat_seq_next(struct hdag_seq *seq, void **pitem);

/** The reset function for a concatenated sequence */
[[nodiscard]]
extern hdag_res hdag_concat_seq_reset(struct hdag_seq *seq);

/**
 * Check if a concatenated sequence is valid.
 *
 * @param seq   The concatenated sequence to check.
 *
 * @return True if the sequence is valid, false otherwise.
 */
static inline bool
hdag_concat_seq_is_valid(const struct hdag_concat_seq *seq)
{
    return
        seq != NULL &&
        hdag_seq_is_valid(&seq->base) &&
        hdag_seq_is_valid(seq->seq_seq) &&
        (seq->seq == NULL || hdag_seq_is_valid(seq->seq));
}

/**
 * Validate a concatenated sequence.
 *
 * @param seq   The concatenated sequence to validate.
 *
 * @return The validated sequence.
 */
static inline struct hdag_concat_seq*
hdag_concat_seq_validate(struct hdag_concat_seq *seq)
{
    assert(hdag_concat_seq_is_valid(seq));
    return seq;
}

/** Cast an abstract sequence pointer to a concat sequence pointer */
#define HDAG_CONCAT_SEQ_FROM_SEQ(_seq) \
    hdag_concat_seq_validate(                                 \
        HDAG_CONTAINER_OF(struct hdag_concat_seq, base, _seq) \
    )

/** Cast a concat sequence pointer to an abstract sequence pointer */
#define HDAG_CONCAT_SEQ_TO_SEQ(_concat_seq) \
    (&hdag_concat_seq_validate(_concat_seq)->base)

/**
 * Create a concat sequence.
 *
 * @param seq_seq               The sequence returning sequences to
 *                              concatenate.
 * @param all_seq_variable      True if all the sequences in _seq_seq are
 *                              variable. False otherwise.
 * @param all_seq_resettable    True if all the sequences in _seq_seq are
 *                              resettable. False otherwise.
 */
static inline struct hdag_concat_seq
hdag_concat_seq(struct hdag_seq *seq_seq,
                bool all_seq_variable,
                bool all_seq_resettable)
{
    assert(hdag_seq_is_valid(seq_seq));
    return (struct hdag_concat_seq){
        .seq_seq = seq_seq,
        .base = hdag_seq(
            hdag_concat_seq_next,
            all_seq_variable,
            hdag_seq_is_resettable(seq_seq) && all_seq_resettable
                ? hdag_concat_seq_reset
                : NULL
        ),
    };
}

/**
 * An initializer for a concat sequence
 *
 * @param _seq_seq              The sequence returning sequences to
 *                              concatenate.
 * @param _all_seq_variable     True if all the sequences in _seq_seq are
 *                              variable. False otherwise.
 * @param _all_seq_resettable   True if all the sequences in _seq_seq are
 *                              resettable. False otherwise.
 */
#define HDAG_CONCAT_SEQ(_seq_seq, _all_seq_variable, _all_seq_resettable) \
    hdag_concat_seq(_seq_seq, _all_seq_variable, _all_seq_resettable)

#endif /* _HDAG_CONCAT_SEQ_H */
