/*
 * Hash DAG node sequence
 */

#ifndef _HDAG_NODE_SEQ_H
#define _HDAG_NODE_SEQ_H

#include <hdag/hash_seq.h>
#include <hdag/res.h>
#include <stdint.h>
#include <stdbool.h>

/* Forward declaration of the node sequence */
struct hdag_node_seq;

/**
 * The prototype for a function resetting a node sequence to the start
 * position.
 *
 * @param node_seq  Node sequence to reset.
 */
typedef void (*hdag_node_seq_reset_fn)(struct hdag_node_seq *node_seq);

/**
 * The prototype for a function returning the information on the next node in
 * a sequence.
 *
 * @param node_seq          The node sequence being traversed.
 * @param phash             Location for the pointer to the retrieved node's
 *                          hash. Only valid until the next call to this
 *                          function.
 * @param ptarget_hash_seq  Location for the pointer to the sequence of the node's
 *                          target hashes. Valid until the next call of this
 *                          function.
 *
 * @return  Zero (HDAG_RES_OK) if the node was retrieved successfully.
 *          A positive number if there were no more nodes.
 *          A negative number (a failure result) if node retrieval has failed.
 */
typedef hdag_res (*hdag_node_seq_next_fn)(
    struct hdag_node_seq   *node_seq,
    const uint8_t         **phash,
    struct hdag_hash_seq  **ptarget_hash_seq
);

/** A node sequence */
struct hdag_node_seq {
    /** The length of the node hashes in the sequence, bytes */
    uint16_t                hash_len;
    /** The function resetting the sequence to the start */
    hdag_node_seq_reset_fn  reset_fn;
    /** The function retrieving the next node from the sequence */
    hdag_node_seq_next_fn   next_fn;
};

/**
 * Check if a node sequence is valid.
 *
 * @param node_seq  The node sequence to check.
 *
 * @return True if the sequence is valid, false otherwise.
 */
static inline bool
hdag_node_seq_is_valid(const struct hdag_node_seq *node_seq)
{
    return node_seq != NULL &&
        hdag_hash_len_is_valid(node_seq->hash_len) &&
        node_seq->next_fn != NULL;
}

/**
 * Check if a node sequence is resettable (multi-pass).
 *
 * @param node_seq  The node sequence to check.
 *
 * @return True if the sequence is resettable (can be rewound and reused),
 *         false if it's single-pass only.
 */
static inline bool
hdag_node_seq_is_resettable(const struct hdag_node_seq *node_seq)
{
    assert(hdag_node_seq_is_valid(node_seq));
    return node_seq->reset_fn != NULL;
}

/**
 * Reset a node sequence to the start position.
 *
 * @param node_seq  The (resettable) node sequence to reset.
 */
static inline void
hdag_node_seq_reset(struct hdag_node_seq *node_seq)
{
    assert(hdag_node_seq_is_valid(node_seq));
    assert(hdag_node_seq_is_resettable(node_seq));
    node_seq->reset_fn(node_seq);
}

/**
 * Retrieve the next node from a node sequence.
 *
 * @param node_seq          The node sequence to retrieve the next node from.
 * @param phash             Location for the pointer to the retrieved node's
 *                          hash. Only valid until the next call to this
 *                          function. Can be NULL to have the hash pointer not
 *                          output.
 * @param ptarget_hash_seq  Location for the pointer to the sequence of the node's
 *                          target hashes. Valid until the next call of this
 *                          function. Can be NULL to have the sequence pointer
 *                          not output.
 *
 * @return  Zero (HDAG_RES_OK) if the node was retrieved successfully.
 *          A positive number if there were no more nodes.
 *          A negative number (a failure result) if node retrieval has failed.
 */
static inline hdag_res
hdag_node_seq_next(struct hdag_node_seq *node_seq,
                   const uint8_t **phash,
                   struct hdag_hash_seq **ptarget_hash_seq)
{
    const uint8_t *hash;
    struct hdag_hash_seq *target_hash_seq;
    assert(hdag_node_seq_is_valid(node_seq));
    return node_seq->next_fn(
        node_seq,
        phash ? phash : &hash,
        ptarget_hash_seq ? ptarget_hash_seq : &target_hash_seq
    );
}

/** A node sequence resetting function which does nothing */
extern void hdag_node_seq_empty_reset(struct hdag_node_seq *node_seq);

/** A next-node retrieval function which never returns nodes */
[[nodiscard]]
extern hdag_res hdag_node_seq_empty_next(
                struct hdag_node_seq *node_seq,
                const uint8_t **phash,
                struct hdag_hash_seq **ptarget_hash_seq);

/**
 * An empty node sequence initializer.
 *
 * @param _hash_len The length of the (never returned) node hashes in the
 *                  sequence.
 */
#define HDAG_NODE_SEQ_EMPTY(_hash_len) (struct hdag_node_seq){ \
    .hash_len = hdag_hash_len_validate(_hash_len),              \
    .reset_fn = hdag_node_seq_empty_reset,                      \
    .next_fn = hdag_node_seq_empty_next,                        \
}

/**
 * Compare two node sequences with the same hash length.
 *
 * @param seq_a    The first sequence to compare.
 * @param seq_b    The second sequence to compare.
 *
 * @return A universal result code:
 *         1 - seq_a < seq_b,
 *         2 - seq_a == seq_b,
 *         3 - seq_a > seq_b,
 *         or a node or hash retrieval fault.
 */
extern hdag_res hdag_node_seq_cmp(struct hdag_node_seq *seq_a,
                                  struct hdag_node_seq *seq_b);

#endif /* _HDAG_NODE_SEQ_H */
