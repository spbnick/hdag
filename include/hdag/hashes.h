/*
 * Hash DAG (lexicographically) sorted hash array
 */

#ifndef _HDAG_HASHES_H
#define _HDAG_HASHES_H

#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <assert.h>
#include <hdag/hash_seq.h>

/** Hash array sequence (resettable) */
struct hdag_hashes_seq {
    /** The base abstract hash sequence */
    struct hdag_hash_seq base;
    /** The array of hashes */
    const uint8_t *hashes;
    /** Number of hashes in the array */
    size_t hash_num;
    /** The index of the next hash to return */
    size_t next_idx;
};

/** The next-hash retrieval function of a hash array sequence */
[[nodiscard]]
extern hdag_res hdag_hashes_seq_next(
                    struct hdag_hash_seq *hash_seq,
                    uint8_t *phash);

/** The reset function of a hash array sequence */
extern void hdag_hashes_seq_reset(
                    struct hdag_hash_seq *hash_seq);

/**
 * Initialize a (resettable) hash array sequence.
 *
 * @param pseq      The location of the hash array sequence to initialize.
 * @param hash_len  The length of each hash in the array.
 * @param hashes Pointer to the hash array to iterate over.
 * @param hash_num The number of hashes in the array.
 *
 * @return The abstract sequence pointer.
 */
static inline struct hdag_hash_seq *
hdag_hashes_seq_init(struct hdag_hashes_seq *pseq,
                     uint16_t hash_len,
                     const uint8_t *hashes,
                     size_t hash_num)
{
    assert(pseq != NULL);
    assert(hdag_hash_len_is_valid(hash_len));
    assert(hashes != NULL || hash_num == 0);
    *pseq = (struct hdag_hashes_seq){
        .base = {
            .hash_len = hash_len,
            .reset_fn = hdag_hashes_seq_reset,
            .next_fn = hdag_hashes_seq_next,
        },
        .hashes = hashes,
        .hash_num = hash_num,
        .next_idx = 0,
    };
    return &pseq->base;
}

#endif /* _HDAG_HASHES_H */
