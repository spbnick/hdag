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
 * The prototype for a function returning the information on the next node in
 * a sequence.
 *
 * @param node_seq          The node sequence being traversed.
 * @param phash             Location for the node's hash. The length of the
 *                          hash is specified in the sequence.
 *                          Can be modified even in case of failure.
 * @param ptarget_hash_seq  Location for the node's sequence of target node
 *                          hashes. Can be modified even in case of failure.
 *
 * @return  Zero (HDAG_RES_OK) if the node was retrieved successfully.
 *          A positive number if there were no more nodes.
 *          A negative number (a failure result) if node retrieval has failed.
 */
typedef hdag_res (*hdag_node_seq_next_fn)(
    const struct hdag_node_seq *node_seq,
    uint8_t                    *phash,
    struct hdag_hash_seq       *ptarget_hash_seq
);

/** A node sequence */
struct hdag_node_seq {
    /** The length of the node hashes in the sequence, bytes */
    uint16_t                hash_len;
    /** The function retrieving the next node from the sequence */
    hdag_node_seq_next_fn   next_fn;
    /** Sequence state data */
    void                   *data;
};

/** A next-node retrieval function which never returns nodes */
extern hdag_res hdag_node_seq_empty_next_fn(
                const struct hdag_node_seq *node_seq,
                uint8_t *phash,
                struct hdag_hash_seq *ptarget_hash_seq);

/**
 * Check if the node sequence is valid.
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
 * An empty node sequence initializer.
 *
 * @param _hash_len The length of the (never returned) node hashes in the
 *                  sequence.
 */
#define HDAG_NODE_SEQ_EMPTY(_hash_len) (struct hdag_node_seq){ \
    .hash_len = hdag_hash_len_validate(_hash_len),              \
    .next_fn = hdag_node_seq_empty_next_fn                      \
}

#endif /* _HDAG_NODE_SEQ_H */
