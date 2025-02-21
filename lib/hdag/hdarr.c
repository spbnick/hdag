/**
 * Hash DAG (lexicographically) sorted hash array
 */

#include <hdag/hdarr.h>
#include <hdag/misc.h>
#include <sys/param.h>
#include <string.h>

bool
hdag_hdarr_slice_find(const struct hdag_darr *hdarr,
                      size_t start, size_t end,
                      const uint8_t *hash,
                      size_t *phash_idx)
{
    int relation;
    size_t middle;

    assert(hdag_hdarr_slice_is_valid(hdarr, start, end));
    assert(hdag_hdarr_slice_is_sorted_and_deduped(hdarr, start, end));
    assert(hash != NULL);

    while (start < end) {
        middle = (start + end) >> 1;
        relation = memcmp(hash,
                          hdag_darr_slot_const(hdarr, middle),
                          hdarr->slot_size);
        if (relation == 0) {
            start = middle;
            break;
        } if (relation > 0) {
            start = middle + 1;
        } else {
            end = middle;
        }
    }

    if (phash_idx != NULL) {
        *phash_idx = start;
    }
    return relation == 0;
}

hdag_res
hdag_hdarr_seq_next(struct hdag_hash_seq *base_seq, const uint8_t **phash)
{
    struct hdag_hdarr_seq *seq = HDAG_CONTAINER_OF(
        struct hdag_hdarr_seq, base, base_seq
    );
    if (seq->next_idx >= seq->hdarr->slots_occupied) {
        return 1;
    }
    *phash = hdag_darr_slot(seq->hdarr, seq->next_idx);
    seq->next_idx++;
    return 0;
}

void
hdag_hdarr_seq_reset(struct hdag_hash_seq *base_seq)
{
    struct hdag_hdarr_seq *seq = HDAG_CONTAINER_OF(
        struct hdag_hdarr_seq, base, base_seq
    );
    seq->next_idx = 0;
}
