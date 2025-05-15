/*
 * Hash DAG - iterator of nodes read from a text file
 */

#ifndef _HDAG_TXT_NODE_ITER_H
#define _HDAG_TXT_NODE_ITER_H

#include <hdag/node_iter.h>
#include <hdag/hash.h>
#include <hdag/res.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

/** Text file node iterator private data */
struct hdag_txt_node_iter_data {
    /** The length of hashes in the stream */
    size_t                      hash_len;
    /** The FILE stream containing the text to parse and load */
    FILE                       *stream;
    /** The buffer to use for node hashes */
    uint8_t                    *hash_buf;
    /** The buffer to use for target hashes */
    uint8_t                    *target_hash_buf;
    /** The returned node item */
    struct hdag_node_iter_item  item;
};

/**
 * Check if text node iterator data is valid.
 *
 * @param data   The text node iterator data to check.
 *
 * @return True if the iterator data is valid, false otherwise.
 */
static inline bool
hdag_txt_node_iter_data_is_valid(const struct hdag_txt_node_iter_data *data)
{
    return
        data != NULL &&
        hdag_hash_len_is_valid(data->hash_len) &&
        hdag_node_iter_item_is_valid(&data->item) &&
        data->item.hash == data->hash_buf &&
        (data->stream == NULL) == (data->hash_buf == NULL) &&
        (data->stream == NULL) == (data->target_hash_buf == NULL);
}

/**
 * Validate text node iterator data.
 *
 * @param data   The text node iterator data to validate.
 *
 * @return The validated iterator data.
 */
static inline struct hdag_txt_node_iter_data*
hdag_txt_node_iter_data_validate(struct hdag_txt_node_iter_data *data)
{
    assert(hdag_txt_node_iter_data_is_valid(data));
    return data;
}

/** The text node iterator next-item retrieval function */
[[nodiscard]]
extern hdag_res hdag_txt_node_iter_next(
                    const struct hdag_iter *iter, void **pitem);

/** The text node iterator property retrieval function */
[[nodiscard]]
extern bool hdag_txt_node_iter_get_prop(
                    const struct hdag_iter *iter,
                    enum hdag_iter_prop_id id,
                    hdag_type type,
                    void *pvalue);

/** The text node iterator next target hash retrieval function */
[[nodiscard]]
extern hdag_res hdag_txt_node_iter_target_hash_iter_next(
                    const struct hdag_iter *iter, void **pitem);

/**
 * Create a text node iterator.
 *
 * @param hash_len          The length of hashes contained in the file.
 * @param data              The location for the private iterator data.
 * @param stream            The FILE stream containing the text to parse.
 *                          Can be NULL to signify a void iterator.
 * @param hash_buf          The buffer for storing retrieved node hashes.
 *                          Must be at least hash_len long.
 *                          Must be NULL if stream is NULL.
 * @param target_hash_buf   The buffer for storing retrieved target hashes.
 *                          Must be at least hash_len long.
 *                          Must be NULL if stream is NULL.
 *
 * @return The created iterator.
 */
static inline struct hdag_iter
hdag_txt_node_iter(size_t hash_len, struct hdag_txt_node_iter_data *data,
                   FILE *stream, uint8_t *hash_buf, uint8_t *target_hash_buf)
{
    assert(hdag_hash_len_is_valid(hash_len));
    assert(data != NULL);
    assert((stream == NULL) == (hash_buf == NULL));
    assert((stream == NULL) == (target_hash_buf == NULL));
    *data = (struct hdag_txt_node_iter_data){
        .hash_len = hash_len,
        .stream = stream,
        .hash_buf = hash_buf,
        .target_hash_buf = target_hash_buf,
        .item = {
            .hash = hash_buf,
            .target_hash_iter = hdag_iter(
                hdag_txt_node_iter_target_hash_iter_next,
                NULL,
                HDAG_TYPE_ARR(HDAG_TYPE_ID_UINT8, hash_len),
                true,
                NULL
            ),
        },
    };
    return hdag_iter(
        hdag_txt_node_iter_next,
        hdag_txt_node_iter_get_prop,
        HDAG_NODE_ITER_ITEM_TYPE,
        true,
        hdag_txt_node_iter_data_validate(data)
    );
}

#endif /* _HDAG_TXT_NODE_ITER_H */
