/*
 * Hash DAG node sequence
 */

#ifndef _HDAG_NODE_SEQ_H
#define _HDAG_NODE_SEQ_H

#include <hdag/hash_seq.h>

/**
 * The prototype for a function returning the information on the next node in
 * a sequence.
 *
 * @param node_seq_data     Node sequence state data.
 * @param phash             Location for the node's hash.
 *                          The length of the hash is expected to be encoded
 *                          in the sequence state data.
 * @param ptarget_hash_seq  Location for the node's sequence of target node
 *                          hashes.
 *
 * @return  Zero if the node was retrieved successfully.
 *          A positive number if there were no more nodes.
 *          A negative number if node retrieval has failed
 *          (and errno was set appropriately).
 */
typedef int (*hdag_node_seq_next)(
    void *node_seq_data,
    uint8_t *phash,
    struct hdag_hash_seq *ptarget_hash_seq
);

/** A next-node retrieval function which always reports no nodes */
extern int hdag_node_seq_next_none(void *node_seq_data, uint8_t *phash,
                                   struct hdag_hash_seq *ptarget_hash_seq);

/** A node sequence */
struct hdag_node_seq {
    /** The function retrieving the next node from the sequence */
    hdag_node_seq_next     next;
    /** Sequence state data */
    void                  *data;
};

/** An empty node sequence initializer */
#define HDAG_NODE_SEQ_EMPTY (struct hdag_node_seq){ \
    .next = hdag_node_seq_next_none                 \
}

#endif /* _HDAG_NODE_SEQ_H */
