/*
 * Hash DAG node sequence
 */

#ifndef _HDAG_NODE_SEQ_H
#define _HDAG_NODE_SEQ_H

#include <hdag/hash_seq.h>
#include <hdag/res.h>
#include <stdint.h>
#include <stdbool.h>

/** Type of items (nodes) returned from a node sequence */
struct hdag_node_seq_item {
    /** The node hash (length specified in the sequence) */
    const uint8_t      *hash;
    /**
     * The (unsized) sequence of the node's target hashes
     * (with length specified in the node sequence).
     */
    struct hdag_seq    *target_hash_seq;
};

/**
 * Check if a node sequence item is valid.
 *
 * @param item  The item to check.
 *
 * @return True if the item is valid, false otherwise.
 */
static inline bool
hdag_node_seq_item_is_valid(const struct hdag_node_seq_item *item)
{
    return item != NULL && (
        item->hash == NULL ||
        hdag_seq_is_valid(item->target_hash_seq)
    );
}

/**
 * Check if a node sequence item is void.
 *
 * @param item  The item to check.
 *
 * @return True if the item is void, false otherwise.
 */
static inline bool
hdag_node_seq_item_is_void(const struct hdag_node_seq_item *item)
{
    assert(hdag_node_seq_item_is_valid(item));
    return item->hash == NULL;
}

/**
 * Compare two node sequence items.
 *
 * @param first     The first item to compare.
 * @param second    The second item to compare.
 * @param data      The size of each value (uintptr_t).
 *
 * @return Universal comparison result.
 */
extern hdag_res hdag_node_seq_item_cmp(const void *first, const void *second,
                                       void *data);

/** An abstract node sequence */
struct hdag_node_seq {
    /** The base abstract sequence */
    struct hdag_seq base;
    /**
     * The length of the node hashes in the sequence, bytes, or zero to
     * signify a "void" sequence which never returns items (nodes).
     */
    size_t          hash_len;
};

/**
 * Check if a node sequence is valid.
 *
 * @param seq   The node sequence to check.
 *
 * @return True if the sequence is valid, false otherwise.
 */
static inline bool
hdag_node_seq_is_valid(const struct hdag_node_seq *seq)
{
    return
        seq != NULL &&
        (seq->hash_len == 0 || hdag_hash_len_is_valid(seq->hash_len)) &&
        hdag_seq_is_valid(&seq->base);
}

/**
 * Validate a node sequence.
 *
 * @param seq   The node sequence to validate.
 *
 * @return The validated sequence.
 */
static inline struct hdag_node_seq*
hdag_node_seq_validate(struct hdag_node_seq *seq)
{
    assert(hdag_node_seq_is_valid(seq));
    return seq;
}

/**
 * Check if a node sequence is "void", that is never returns items.
 *
 * @param seq   The node sequence to check.
 *
 * @return True if the sequence is void, false otherwise.
 */
static inline bool
hdag_node_seq_is_void(const struct hdag_node_seq *seq)
{
    assert(hdag_node_seq_is_valid(seq));
    return seq->hash_len == 0;
}

/**
 * Retrieve the hash length of a node sequence.
 *
 * @param seq   The sequence to return the hash length of.
 *
 * @return The hash length. Zero if the sequence is void.
 */
static inline size_t
hdag_node_seq_get_hash_len(const struct hdag_node_seq *seq)
{
    assert(hdag_node_seq_is_valid(seq));
    return seq->hash_len;
}

/**
 * Reset a node sequence to the start position.
 *
 * @param seq  The sequence to reset. Must be resettable (or void).
 */
static inline hdag_res
hdag_node_seq_reset(struct hdag_node_seq *seq)
{
    assert(hdag_node_seq_is_valid(seq));
    if (hdag_node_seq_is_void(seq)) {
        return HDAG_RES_OK;
    }
    return hdag_seq_reset(&seq->base);
}

/**
 * Get the next item out of a node sequence, if possible.
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
hdag_node_seq_next(struct hdag_node_seq *seq,
                   struct hdag_node_seq_item **pitem)
{
    assert(hdag_node_seq_is_valid(seq));
    if (hdag_node_seq_is_void(seq)) {
        return HDAG_RES_OK;
    }
    return hdag_seq_next(&seq->base, (void **)pitem);
}

/**
 * Get the next const item out of a node sequence, if possible.
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
hdag_node_seq_next_const(struct hdag_node_seq *seq,
                         const struct hdag_node_seq_item **pitem)
{
    assert(hdag_node_seq_is_valid(seq));
    if (hdag_node_seq_is_void(seq)) {
        return HDAG_RES_OK;
    }
    return hdag_seq_next_const(&seq->base, (const void **)pitem);
}

/** Cast an abstract sequence pointer to a node sequence pointer */
#define HDAG_NODE_SEQ_FROM_SEQ(_seq) \
    hdag_node_seq_validate(                                 \
        HDAG_CONTAINER_OF(struct hdag_node_seq, base, _seq) \
    )

/** Cast a node sequence pointer to an abstract sequence pointer */
#define HDAG_NODE_SEQ_TO_SEQ(_node_seq) \
    (&hdag_node_seq_validate(_node_seq)->base)

/**
 * Create an abstract node sequence.
 *
 * @param next_fn   The function retrieving the next node from the
 *                  sequence.
 * @param reset_fn  The function resetting the sequence to the first node.
 *                  If NULL, the sequence is single-pass only.
 * @param hash_len  The length of node hashes in the sequence.
 *
 */
static inline struct hdag_node_seq
hdag_node_seq(hdag_seq_next_fn next_fn,
              hdag_seq_reset_fn reset_fn, size_t hash_len)
{
    assert(next_fn != NULL);
    assert(hash_len == 0 || hdag_hash_len_is_valid(hash_len));
    return (struct hdag_node_seq){
        .base = hdag_seq(next_fn, false, reset_fn),
        .hash_len = hash_len,
    };
}

/**
 * An initializer for an abstract node sequence.
 *
 * @param _next_fn  The function retrieving the next node from the
 *                  sequence.
 * @param _reset_fn The function resetting the sequence to the first node.
 *                  If NULL, the sequence is single-pass only.
 * @param _hash_len The length of node hashes in the sequence.
 *
 */
#define HDAG_NODE_SEQ(_next_fn, _reset_fn, _hash_len) \
    hdag_node_seq(_next_fn, _reset_fn, _hash_len)

/**
 * An initializer for an empty node sequence.
 *
 * @param _hash_len The length of node hashes in the sequence
 *                  (if there would be any).
 */
#define HDAG_NODE_SEQ_EMPTY(_hash_len) \
    hdag_node_seq(hdag_seq_empty_next, hdag_seq_empty_reset, _hash_len)

/** An initializer for a void node sequence */
#define HDAG_NODE_SEQ_VOID HDAG_NODE_SEQ_EMPTY(0)

/**
 * Compare two node sequences.
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
hdag_node_seq_cmp(struct hdag_node_seq *seq_a,
                  struct hdag_node_seq *seq_b)
{
    assert(hdag_node_seq_is_valid(seq_a));
    assert(hdag_node_seq_is_valid(seq_b));
    if (seq_a->hash_len < seq_b->hash_len) {
        return 1;
    }
    if (seq_a->hash_len > seq_b->hash_len) {
        return 3;
    }
    /* If both sequences are void */
    if (seq_a->hash_len == 0) {
        return 2;
    }
    return hdag_seq_cmp(
        HDAG_NODE_SEQ_TO_SEQ(seq_a),
        HDAG_NODE_SEQ_TO_SEQ(seq_b),
        hdag_node_seq_item_cmp,
        (void *)(uintptr_t)seq_a->hash_len
    );
}

#endif /* _HDAG_NODE_SEQ_H */
