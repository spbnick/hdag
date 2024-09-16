/*
 * Hash DAG hash sequence.
 */
#ifndef _HDAG_HASH_SEQ_H
#define _HDAG_HASH_SEQ_H

#include <hdag/hash.h>
#include <hdag/res.h>
#include <stdint.h>
#include <stdbool.h>

/* The forward declaration of the node sequence */
struct hdag_hash_seq;

/**
 * The prototype for a function returning the next node hash from a sequence.
 *
 * @param hash_seq  Hash sequence being traversed.
 * @param phash     Location for the retrieved hash.
 *                  The length of the hash is defined in hash_seq.
 *
 * @return  Zero (HDAG_RES_OK) if the hash was retrieved successfully.
 *          A positive number if there were no more hashes.
 *          A negative number (a failure result) if hash retrieval has failed.
 */
typedef hdag_res (*hdag_hash_seq_next_fn)(const struct hdag_hash_seq *hash_seq,
                                          uint8_t *phash);

/** A hash sequence */
struct hdag_hash_seq {
    /** The length of returned hashes, bytes */
    uint16_t                hash_len;
    /** The function retrieving the next hash from the sequence */
    hdag_hash_seq_next_fn   next_fn;
    /** The sequence state data */
    void                   *data;
};

/**
 * Check if the hash sequence is valid.
 *
 * @param hash_seq  The hash sequence to check.
 *
 * @return True if the sequence is valid, false otherwise.
 */
static inline bool
hdag_hash_seq_is_valid(const struct hdag_hash_seq *hash_seq)
{
    return hash_seq != NULL &&
        hdag_hash_len_is_valid(hash_seq->hash_len) &&
        hash_seq->next_fn != NULL;
}

/** A next-hash retrieval function which never returns hashes */
extern hdag_res hdag_hash_seq_empty_next_fn(
                    const struct hdag_hash_seq *hash_seq,
                    uint8_t *phash);

/**
 * An initializer for an empty hash sequence
 *
 * @param _hash_len The length of the hashes in the sequence
 *                  (if they were returned).
 */
#define HDAG_HASH_SEQ_EMPTY(_hash_len) (struct hdag_hash_seq){ \
    .hash_len = (_hash_len),                                        \
    .next_fn = hdag_hash_seq_empty_next_fn                          \
}

#endif /* _HDAG_HASH_SEQ_H */
