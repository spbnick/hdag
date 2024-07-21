/*
 * Hash DAG dynamic array
 */

#include <hdag/darr.h>

void *
hdag_darr_alloc(struct hdag_darr *darr, size_t num)
{
    assert(hdag_darr_is_valid(darr));

    if (num != 0) {
        size_t new_slots_occupied = darr->slots_occupied + num;
        size_t new_slots_allocated = darr->slots_allocated
            ? darr->slots_allocated
            : darr->slots_preallocate;
        void *new_slots;

        while (new_slots_occupied > new_slots_allocated) {
            new_slots_allocated <<= 1;
        }

        new_slots = realloc(darr->slots,
                            new_slots_allocated * darr->slot_size);
        if (new_slots == NULL) {
            return NULL;
        }

        darr->slots_allocated = new_slots_allocated;
        darr->slots = new_slots;
    }

    return hdag_darr_slot(darr, darr->slots_occupied);
}

void *
hdag_darr_append(struct hdag_darr *darr, void *elements, size_t num)
{
    assert(hdag_darr_is_valid(darr));

    void *appended_slots;

    appended_slots = hdag_darr_alloc(darr, num);
    if (appended_slots != NULL) {
        memcpy(appended_slots, elements, darr->slot_size * num);
        darr->slots_occupied += num;
    }

    return appended_slots;
}

bool
hdag_darr_deflate(struct hdag_darr *darr)
{
    assert(hdag_darr_is_valid(darr));
    const size_t new_slots_allocated = darr->slots_occupied;

    if (new_slots_allocated != darr->slots_allocated) {
        void *new_slots = realloc(darr->slots,
                                  darr->slot_size * new_slots_allocated);
        if (new_slots == NULL && new_slots_allocated != 0) {
            return false;
        }
        darr->slots = new_slots;
        darr->slots_allocated = new_slots_allocated;
    }

    return true;
}

