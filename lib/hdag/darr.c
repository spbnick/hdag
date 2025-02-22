/**
 * Hash DAG dynamic array
 */

#include <hdag/darr.h>
#include <limits.h>

void *
hdag_darr_alloc(struct hdag_darr *darr, size_t num)
{
    assert(hdag_darr_is_valid(darr));
    assert(hdag_darr_is_mutable(darr));

    if (num == 0) {
        return NULL;
    }

    size_t new_slots_occupied = darr->slots_occupied + num;
    size_t new_slots_allocated = darr->slots_allocated
        ? darr->slots_allocated
        : darr->slots_preallocate;

    if (new_slots_allocated == 0) {
        new_slots_allocated = new_slots_occupied;
    } else {
        while (new_slots_occupied > new_slots_allocated) {
            new_slots_allocated <<= 1;
        }
    }

    if (new_slots_allocated != darr->slots_allocated) {
        void *new_slots;

        if (darr->slot_size == 0 || new_slots_allocated == 0) {
            if (darr->slots != NULL) {
                free(darr->slots);
            }
            new_slots = NULL;
        } else {
            assert(!hdag_darr_is_void(darr));
            new_slots = reallocarray(darr->slots, new_slots_allocated,
                                     darr->slot_size);
            if (new_slots == NULL) {
                return NULL;
            }
        }

        darr->slots_allocated = new_slots_allocated;
        darr->slots = new_slots;
    }

    return hdag_darr_slot(darr, darr->slots_occupied);
}

bool
hdag_darr_deflate(struct hdag_darr *darr)
{
    assert(hdag_darr_is_valid(darr));
    const size_t new_slots_allocated = darr->slots_occupied;

    if (hdag_darr_is_immutable(darr)) {
        return true;
    }

    if (new_slots_allocated != darr->slots_allocated) {
        void *new_slots;
        if (darr->slot_size == 0 || new_slots_allocated == 0) {
            if (darr->slots != NULL) {
                free(darr->slots);
            }
            new_slots = NULL;
        } else {
            assert(!hdag_darr_is_void(darr));
            new_slots = reallocarray(darr->slots, new_slots_allocated,
                                     darr->slot_size);
            if (new_slots == NULL) {
                return false;
            }
        }
        darr->slots = new_slots;
        darr->slots_allocated = new_slots_allocated;
    }

    return true;
}

bool
hdag_darr_slice_is_sorted_as(const struct hdag_darr *darr,
                             size_t start, size_t end,
                             hdag_darr_cmp_fn cmp, void *data,
                             int cmp_min, int cmp_max)
{
    size_t slot_size;
    uint8_t *start_slot;
    uint8_t *prev_slot;
    uint8_t *slot;
    uint8_t *end_slot;
    int rel;
    int rel_min = cmp_min < 0 ? INT_MIN : cmp_min;
    int rel_max = cmp_max > 0 ? INT_MAX : cmp_max;

    assert(hdag_darr_slice_is_valid(darr, start, end));
    assert(cmp != NULL);
    assert(cmp_min >= -1);
    assert(cmp_max <= 1);
    assert(cmp_max >= cmp_min);

    for (
        slot_size = darr->slot_size,
        start_slot = darr->slots + slot_size * start,
        prev_slot = start_slot,
        end_slot = darr->slots + slot_size * end;

        (slot = prev_slot + slot_size) < end_slot;

        prev_slot = slot
    ) {
        rel = cmp(prev_slot, slot, data);
        if (rel < rel_min || rel > rel_max) {
            return false;
        }
    }

    return true;
}

size_t
hdag_darr_slice_dedup(struct hdag_darr *darr,
                      size_t start, size_t end,
                      hdag_darr_cmp_fn cmp, void *data)
{
    size_t slot_size;
    uint8_t *output_slot;
    uint8_t *start_slot;
    uint8_t *prev_slot;
    uint8_t *slot;
    uint8_t *end_slot;

    assert(hdag_darr_slice_is_valid(darr, start, end));
    assert(hdag_darr_is_mutable(darr));
    assert(cmp != NULL);

    for (
        slot_size = darr->slot_size,
        start_slot = darr->slots + slot_size * start,
        prev_slot = start_slot,
        end_slot = darr->slots + slot_size * end,
        output_slot = start_slot;

        (slot = prev_slot + slot_size) < end_slot;

        prev_slot = slot
    ) {
        /* Keep previous element if it's different */
        if (cmp(prev_slot, slot, data) != 0) {
            output_slot += slot_size;
        }
        /* Output current element if it's moved */
        if (output_slot < slot) {
            memcpy(output_slot, slot, slot_size);
        }
    }

    return hdag_darr_slot_idx(darr, output_slot + slot_size);
}
