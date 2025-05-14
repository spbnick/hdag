/*
 * Hash DAG - sequence of nodes read from a text file
 */

#ifndef _HDAG_TXT_NODE_SEQ_H
#define _HDAG_TXT_NODE_SEQ_H

#include <hdag/node_seq.h>
#include <hdag/res.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

/** A text file node sequence */
struct hdag_txt_node_seq {
    /** The base abstract node sequence */
    struct hdag_node_seq        base;
    /** The FILE stream containing the text to parse and load */
    FILE                       *stream;
    /** The buffer to use for node hashes */
    uint8_t                    *hash_buf;
    /** The buffer to use for target hashes */
    uint8_t                    *target_hash_buf;
    /** The returned node's target hash sequence */
    struct hdag_hash_seq        target_hash_seq;
    /** The returned node item */
    struct hdag_node_seq_item   item;
};

/**
 * Check if a text node sequence is valid.
 *
 * @param seq   The text node sequence to check.
 *
 * @return True if the sequence is valid, false otherwise.
 */
static inline bool
hdag_txt_node_seq_is_valid(const struct hdag_txt_node_seq *seq)
{
    return
        seq != NULL &&
        hdag_node_seq_is_valid(&seq->base) &&
        hdag_hash_seq_is_valid(&seq->target_hash_seq) &&
        hdag_node_seq_item_is_valid(&seq->item) &&
        (
            seq->item.hash == NULL ?
                seq->item.target_hash_seq == NULL :
                (
                     seq->item.hash == seq->hash_buf &&
                     seq->item.target_hash_seq ==
                     HDAG_HASH_SEQ_TO_SEQ_CONST(&seq->target_hash_seq)
                )
        ) &&
        (
            hdag_node_seq_is_void(&seq->base) ? (
                seq->stream == NULL &&
                seq->hash_buf == NULL &&
                seq->target_hash_buf == NULL
            ) : (
                seq->stream != NULL &&
                seq->hash_buf != NULL &&
                seq->target_hash_buf != NULL
            )
        );
}

/**
 * Validate a text node sequence.
 *
 * @param seq   The text node sequence to validate.
 *
 * @return The validated sequence.
 */
static inline struct hdag_txt_node_seq*
hdag_txt_node_seq_validate(struct hdag_txt_node_seq *seq)
{
    assert(hdag_txt_node_seq_is_valid(seq));
    return seq;
}

/** Cast an abstract node sequence pointer to a text node sequence pointer */
#define HDAG_TXT_NODE_SEQ_FROM_NODE_SEQ(_seq) \
    hdag_txt_node_seq_validate(                                 \
        HDAG_CONTAINER_OF(struct hdag_txt_node_seq, base, _seq) \
    )

/** Cast an abstract sequence pointer to a text node sequence pointer */
#define HDAG_TXT_NODE_SEQ_FROM_SEQ(_seq) \
    HDAG_TXT_NODE_SEQ_FROM_NODE_SEQ(HDAG_NODE_SEQ_FROM_SEQ(_seq))

/** Cast a text node sequence pointer to an abstract node sequence pointer */
#define HDAG_TXT_NODE_SEQ_TO_NODE_SEQ(_seq) \
    (&hdag_txt_node_seq_validate(_seq)->base)

/** Cast a text node sequence pointer to an abstract sequence pointer */
#define HDAG_TXT_NODE_SEQ_TO_SEQ(_seq) \
    HDAG_NODE_SEQ_TO_SEQ(HDAG_TXT_NODE_SEQ_TO_NODE_SEQ(_seq))

/**
 * Check if a text node sequence is "void", that is never returns items.
 *
 * @param seq   The text node sequence to check.
 *
 * @return True if the sequence is void, false otherwise.
 */
static inline bool
hdag_txt_node_seq_is_void(const struct hdag_txt_node_seq *seq)
{
    assert(hdag_txt_node_seq_is_valid(seq));
    return hdag_node_seq_is_void(
        HDAG_TXT_NODE_SEQ_TO_NODE_SEQ((struct hdag_txt_node_seq *)seq)
    );
}

/**
 * Retrieve the hash length of a text node sequence.
 *
 * @param seq   The sequence to return the hash length of.
 *
 * @return The hash length. Zero if the sequence is void.
 */
static inline size_t
hdag_txt_node_seq_get_hash_len(const struct hdag_txt_node_seq *seq)
{
    assert(hdag_txt_node_seq_is_valid(seq));
    return hdag_node_seq_get_hash_len(
        HDAG_TXT_NODE_SEQ_TO_NODE_SEQ((struct hdag_txt_node_seq *)seq)
    );
}

/** The text node sequence next-item retrieval function */
[[nodiscard]]
extern hdag_res hdag_txt_node_seq_next(struct hdag_seq *base_seq,
                                       void **pitem);
            
/** The text node sequence next target hash retrieval function */
[[nodiscard]]
extern hdag_res hdag_txt_node_seq_hash_seq_next(struct hdag_seq *base_seq,
                                                void **pitem);

/**
 * Create a text node sequence.
 *
 * @param stream            The FILE stream containing the text to parse.
 * @param hash_len          The length of hashes contained in the file.
 * @param hash_buf          The buffer for storing retrieved node hashes.
 *                          Must be at least _hash_len long.
 * @param target_hash_buf   The buffer for storing retrieved target hashes.
 *                          Must be at least _hash_len long.
 *
 * @return The created sequence.
 */
static inline struct hdag_txt_node_seq
hdag_txt_node_seq(FILE *stream, size_t hash_len,
                  uint8_t *hash_buf, uint8_t *target_hash_buf)
{
    assert(hash_len == 0 || hdag_hash_len_is_valid(hash_len));
    assert((bool)hash_len == (bool)stream);
    assert((bool)hash_len == (bool)hash_buf);
    assert((bool)hash_len == (bool)target_hash_buf);
    struct hdag_txt_node_seq seq = {
        .base = hdag_node_seq(hdag_txt_node_seq_next, NULL, hash_len),
        .stream = stream,
        .hash_buf = hash_buf,
        .target_hash_buf = target_hash_buf,
        .target_hash_seq = hdag_hash_seq(hdag_txt_node_seq_hash_seq_next,
                                         false, NULL, hash_len),
    };
    assert(hdag_txt_node_seq_is_valid(&seq));
    return seq;
}

/**
 * An initializer for a text node sequence.
 *
 * @param _stream           The FILE stream containing the text to parse.
 * @param _hash_len         The length of hashes contained in the file.
 * @param _hash_buf         The buffer for storing retrieved node hashes.
 *                          Must be at least _hash_len long.
 * @param _target_hash_buf  The buffer for storing retrieved target hashes.
 *                          Must be at least _hash_len long.
 */
#define HDAG_TXT_NODE_SEQ(_stream, _hash_len, _hash_buf, _target_hash_buf) \
    hdag_txt_node_seq(_stream, _hash_len, _hash_buf, _target_hash_buf)

#endif /* _HDAG_TXT_NODE_SEQ_H */
