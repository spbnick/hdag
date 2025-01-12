/*
 * Hash DAG dynamic array
 */

#include <hdag/darr.h>

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
