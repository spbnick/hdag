/**
 * Hash DAG (lexicographically) sorted hash array
 */

#include <hdag/hashes.h>
#include <hdag/misc.h>
#include <sys/param.h>
#include <string.h>

bool
hdag_hashes_are_valid(const uint8_t *hashes,
                      uint16_t hash_len, size_t hash_num)
{
    if (hashes == NULL && hash_num != 0) {
        return false;
    }
    if (hash_len == 0) {
        return hash_num == 0;
    }
    if (!hdag_hash_len_is_valid(hash_len)) {
        return false;
    }

    const uint8_t *hash;
    const uint8_t *prev_hash;
    const uint8_t *hashes_end = hashes + hash_len * hash_num;

    for (hash = hashes, prev_hash = hash, hash += hash_len;
         hash < hashes_end;
         prev_hash = hash, hash += hash_len) {
        if (memcmp(prev_hash, hash, hash_len) >= 0) {
            return false;
        }
    }

    return true;
}

bool
hdag_hashes_slice_find(const uint8_t *hashes,
                       uint16_t hash_len,
                       size_t start_idx, size_t end_idx,
                       const uint8_t *hash_ptr,
                       size_t *phash_idx)
{
    int relation;
    size_t middle_idx;
    const uint8_t *middle_hash;

    assert(start_idx <= end_idx);
    assert(hashes != NULL || end_idx == 0);
    assert(hdag_hash_len_is_valid(hash_len));
    assert(hash_ptr != NULL);

    while (start_idx < end_idx) {
        middle_idx = (start_idx + end_idx) >> 1;
        middle_hash = hashes + middle_idx * hash_len;
        relation = memcmp(hash_ptr, middle_hash, hash_len);
        if (relation == 0) {
            start_idx = middle_idx;
            break;
        } if (relation > 0) {
            start_idx = middle_idx + 1;
        } else {
            end_idx = middle_idx;
        }
    }

    if (phash_idx != NULL) {
        *phash_idx = start_idx;
    }
    return relation == 0;
}

hdag_res
hdag_hashes_seq_next(struct hdag_hash_seq *base_seq, const uint8_t **phash)
{
    struct hdag_hashes_seq *seq = HDAG_CONTAINER_OF(
        struct hdag_hashes_seq, base, base_seq
    );
    if (seq->next_idx >= seq->hash_num) {
        return 1;
    }
    *phash = seq->hashes + base_seq->hash_len * seq->next_idx;
    seq->next_idx++;
    return 0;
}

void
hdag_hashes_seq_reset(struct hdag_hash_seq *base_seq)
{
    struct hdag_hashes_seq *seq = HDAG_CONTAINER_OF(
        struct hdag_hashes_seq, base, base_seq
    );
    seq->next_idx = 0;
}
