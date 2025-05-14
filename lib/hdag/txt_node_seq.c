/*
 * Hash DAG - sequence of nodes read from a text file
 */

#include <hdag/txt_node_seq.h>
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
hdag_txt_node_seq_read_hash(FILE *stream, uint8_t *hash_buf, uint16_t hash_len,
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
hdag_txt_node_seq_hash_seq_next(struct hdag_seq *base_seq, void **pitem)
{
    hdag_res                    res;
    struct hdag_txt_node_seq   *seq = HDAG_CONTAINER_OF(
        struct hdag_txt_node_seq, target_hash_seq,
        HDAG_HASH_SEQ_FROM_SEQ(base_seq)
    );
    size_t                      hash_len = hdag_txt_node_seq_get_hash_len(seq);
    FILE                       *stream = seq->stream;

    assert(!hdag_txt_node_seq_is_void(seq));

    /* Skip whitespace (excluding newlines) and read a hash */
    res = hdag_txt_node_seq_read_hash(stream, seq->target_hash_buf,
                                      hash_len, false);
    /* If we failed reading */
    if (hdag_res_is_failure(res)) {
        return res;
    }
    /* If we didn't get a hash */
    if ((size_t)res >= hash_len) {
        /* Signal end of sequence */
        return 0;
    }
    /* Output the hash pointer */
    *pitem = seq->target_hash_buf;
    /* Signal we got a hash */
    return 1;
}

hdag_res
hdag_txt_node_seq_next(struct hdag_seq *base_seq, void **pitem)
{
    hdag_res                    res;
    struct hdag_txt_node_seq   *seq = HDAG_TXT_NODE_SEQ_FROM_SEQ(base_seq);
    size_t                      hash_len = hdag_txt_node_seq_get_hash_len(seq);
    FILE                       *stream = seq->stream;

    assert(!hdag_txt_node_seq_is_void(seq));

    /* Skip whitespace (including newlines) and read a hash */
    res = hdag_txt_node_seq_read_hash(stream, seq->hash_buf, hash_len, true);
    /* If we failed reading */
    if (hdag_res_is_failure(res)) {
        return res;
    }
    /* If we got no hash */
    if ((size_t)res >= hash_len) {
        /* Signal end of sequence */
        return 0;
    }
    /* Return the item */
    seq->item.target_hash_seq = HDAG_HASH_SEQ_TO_SEQ(&seq->target_hash_seq);
    seq->item.hash = seq->hash_buf;
    *pitem = &seq->item;
    /* Signal we got a node */
    return 1;
}
