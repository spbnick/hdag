/**
 * Hash DAG - a pointer list sequence.
 */
#ifndef _HDAG_LIST_SEQ_H
#define _HDAG_LIST_SEQ_H

#include <hdag/seq.h>
#include <hdag/res.h>
#include <stdint.h>
#include <stdbool.h>

/** A pointer list sequence */
struct hdag_list_seq {
    /** The base abstract sequence */
    struct hdag_seq base;
    /**
     * The list of pointers to iterate over.
     * Can be NULL if len is zero.
     */
    void    **list;
    /** The number of pointers in the list */
    size_t    len;
    /** The index of the currently-traversed pointer */
    size_t    idx;
};

/** The next-item retrieval function for pointer list sequence */
[[nodiscard]]
extern hdag_res hdag_list_seq_next(struct hdag_seq *seq, void **pitem);

/** The reset function for pointer list sequence */
extern hdag_res hdag_list_seq_reset(struct hdag_seq *seq);

/**
 * Check if a list sequence is valid.
 *
 * @param seq   The list sequence to check.
 *
 * @return True if the sequence is valid, false otherwise.
 */
static inline bool
hdag_list_seq_is_valid(const struct hdag_list_seq *seq)
{
    return
        seq != NULL &&
        (seq->list != NULL || seq->len == 0) &&
        seq->idx <= seq->len &&
        hdag_seq_is_valid(&seq->base);
}

/**
 * Validate a list sequence.
 *
 * @param seq   The list sequence to validate.
 *
 * @return The validated list sequence.
 */
static inline struct hdag_list_seq*
hdag_list_seq_validate(struct hdag_list_seq *seq)
{
    assert(hdag_list_seq_is_valid(seq));
    return seq;
}

/** Cast an abstract sequence pointer to a pointer list sequence pointer */
#define HDAG_LIST_SEQ_FROM_SEQ(_seq) \
    hdag_list_seq_validate(                                 \
        HDAG_CONTAINER_OF(struct hdag_list_seq, base, _seq) \
    )

/** Cast a pointer list sequence pointer to an abstract sequence pointer */
#define HDAG_LIST_SEQ_TO_SEQ(_seq) \
    (&hdag_list_seq_validate(_seq)->base)

/**
 * Create a list sequence.
 *
 * @param list      Pointer to the pointer list.
 * @param len       The length of the pointer list.
 * @param variable  True if the items returned by the sequence can be
 *                  modified. False if not, and the items can only be
 *                  retrieved using hdag_seq_next_const().
 */
static inline struct hdag_list_seq
hdag_list_seq(void **list, size_t len, bool variable)
{
    assert(list != NULL || len == 0);
    return (struct hdag_list_seq){
        .base = hdag_seq(hdag_list_seq_next, variable, hdag_list_seq_reset),
        .list = list,
        .len = len,
    };
}

/**
 * An initializer for a list sequence.
 *
 * @param _list     Pointer to the pointer list.
 * @param _len      The length of the pointer list.
 * @param _variable True if the items returned by the sequence can be
 *                  modified. False if not, and the items can only be
 *                  retrieved using hdag_seq_next_const().
 */
#define HDAG_LIST_SEQ(_list, _len, _variable) \
    hdag_list_seq(_list, _len, _variable)

#endif /* _HDAG_LIST_SEQ_H */
