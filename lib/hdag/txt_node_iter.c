/*
 * Hash DAG - iterator of nodes read from a text file
 */

#include <hdag/txt_node_iter.h>
#include <string.h>
#include <ctype.h>

/**
 * Skip initial whitespace and read a hash of specified length from a stream.
 *
 * @param stream            The stream to read the hash from.
 * @param hash_buf          The buffer for the read hash (right-aligned).
 *                          Can be modified even on failure.
 * @param hash_len          The length of the hash to read,
 *                          as well as of its buffer, bytes.
 * @param skip_linebreaks   Include linebreaks into skipped initial
 *                          whitespace, if true. Assume there's no hash, if
 *                          false and an initial linebreak is encountered.
 *
 * @return  A non-negative number specifying how many hash bytes have been
 *          unfilled in the buffer, on success, or hash_len, if there was no
 *          hash.
 *          A negative number (a failure result) if hash retrieval has failed,
 *          including HDAG_RES_INVALID_FORMAT, if the file format is invalid,
 *          and HDAG_RES_ERRNO's in case of libc errors.
 */
[[nodiscard]]
static hdag_res
hdag_txt_node_iter_read_hash(FILE *stream, uint8_t *hash_buf, uint16_t hash_len,
                             bool skip_linebreaks)
{
    uint16_t                    rem_hash_len = hash_len;
    uint8_t                    *hash_ptr = hash_buf;
    int                         c;
    bool                        high_nibble = true;
    uint8_t                     nibble;

    assert(stream != NULL);
    assert(hash_buf != NULL);
    assert(hdag_hash_len_is_valid(hash_len));

    /* Skip whitespace */
    do {
        c = getc(stream);
        if (c == EOF) {
            if (ferror(stream)) {
                return HDAG_RES_ERRNO;
            }
            goto output;
        }
        if (!skip_linebreaks && (c == '\r' || c == '\n')) {
            goto output;
        }
    } while (isspace(c));

    /* Read the node hash until whitespace */
    do {
        /* If this is an invalid character, or the hash is too long */
        if (!isxdigit(c) || rem_hash_len == 0) {
            return HDAG_RES_INVALID_FORMAT;
        }
        nibble = c >= 'A' ? ((c & ~0x20) - ('A' - 0xA)) : (c - '0');
        if (high_nibble) {
            *hash_ptr = nibble << 4;
            high_nibble = false;
        } else {
            *hash_ptr |= nibble;
            high_nibble = true;
            rem_hash_len--;
            hash_ptr++;
        }
        c = getc(stream);
        if (c == EOF) {
            if (ferror(stream)) {
                return HDAG_RES_ERRNO;
            }
            break;
        }
    } while (!isspace(c));

    /* If we didn't get a low nibble (got odd number of digits) */
    if (!high_nibble) {
        return HDAG_RES_INVALID_FORMAT;
    }

output:
    /* Move the hash to the right */
    memmove(hash_buf + rem_hash_len, hash_buf, hash_len - rem_hash_len);
    /* Fill initial missing hash bytes with zeroes */
    memset(hash_buf, 0, rem_hash_len);

    /* Put back the terminating whitespace */
    assert(c == EOF || isspace(c));
    if (c != EOF && ungetc(c, stream) == EOF) {
        return HDAG_RES_ERRNO;
    }

    return rem_hash_len;
}

/**
 * Return the next target hash from an adjacency list text file.
 */
hdag_res
hdag_txt_node_iter_target_hash_iter_next(const struct hdag_iter *iter,
                                         void **pitem)
{
    hdag_res res;
    struct hdag_txt_node_iter_data *data = hdag_txt_node_iter_data_validate(
        HDAG_CONTAINER_OF(
            struct hdag_txt_node_iter_data, item,
            hdag_node_iter_item_validate(
                /* Only the iterator is const, the parent item is not */
                (struct hdag_node_iter_item *)HDAG_CONTAINER_OF(
                    struct hdag_node_iter_item, target_hash_iter, iter
                )
            )
        )
    );
    assert(data->stream != NULL);

    /* Skip whitespace (excluding newlines) and read a hash */
    res = hdag_txt_node_iter_read_hash(data->stream, data->target_hash_buf,
                                       data->hash_len, false);
    /* If we failed reading */
    if (hdag_res_is_failure(res)) {
        return res;
    }
    /* If we didn't get a hash */
    if ((size_t)res >= data->hash_len) {
        /* Signal end of iterator */
        return 0;
    }
    /* Output the hash pointer */
    *pitem = data->target_hash_buf;
    /* Signal we got a hash */
    return 1;
}

hdag_res
hdag_txt_node_iter_next(const struct hdag_iter *iter, void **pitem)
{
    hdag_res res;
    struct hdag_txt_node_iter_data *data =
        hdag_txt_node_iter_data_validate(iter->data);
    assert(data->stream != NULL);

    /* Skip whitespace (including newlines) and read a hash */
    res = hdag_txt_node_iter_read_hash(data->stream, data->hash_buf,
                                       data->hash_len, true);
    /* If we failed reading */
    if (hdag_res_is_failure(res)) {
        return res;
    }
    /* If we got no hash */
    if ((size_t)res >= data->hash_len) {
        /* Signal end of iterator */
        return 0;
    }
    /* Return the item */
    *pitem = &data->item;
    /* Signal we got a node */
    return 1;
}

bool
hdag_txt_node_iter_get_prop(const struct hdag_iter *iter,
                            enum hdag_iter_prop_id id,
                            hdag_type type,
                            void *pvalue)
{
    assert(hdag_iter_is_valid(iter));
    assert(hdag_iter_prop_id_is_valid(id));
    assert(hdag_type_is_valid(type));
    struct hdag_txt_node_iter_data *data =
        hdag_txt_node_iter_data_validate(iter->data);

    if (id == HDAG_ITER_PROP_ID_HASH_LEN && type == HDAG_TYPE_SIZE) {
        if (pvalue != NULL) {
            *(size_t *)pvalue = data->hash_len;
        }
        return true;
    }
    return false;
}
