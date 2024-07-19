/*
 * Hash DAG hash sequence.
 */
#ifndef _HDAG_HASH_SEQ_H
#define _HDAG_HASH_SEQ_H

#include <stdint.h>

/**
 * The prototype for a function returning the next node hash from a sequence.
 *
 * @param hash_seq_data     Hash sequence state data.
 * @param phash             Location for the retrieved hash.
 *                          The length of the hash is expected to be encoded
 *                          in the sequence state data.
 *
 * @return  Zero if the hash was retrieved successfully.
 *          A positive number if there were no more hashes.
 *          A negative number if hash retrieval has failed
 *          (and errno was set appropriately).
 */
typedef int (*hdag_hash_seq_next)(void *hash_seq_data, uint8_t *phash);

/** A hash sequence */
struct hdag_hash_seq {
    /** The function retrieving the next hash from the sequence */
    hdag_hash_seq_next     next;
    /** The sequence state data */
    void                  *data;
};

#endif /* _HDAG_HASH_SEQ_H */
