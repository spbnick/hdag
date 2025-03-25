/**
 * Hash DAG - array
 */

#include <hdag/arr.h>
#include <limits.h>

void *
hdag_arr_alloc(struct hdag_arr *arr, size_t num)
{
    assert(hdag_arr_is_valid(arr));

    if (num == 0) {
        return NULL;
    }

    assert(hdag_arr_is_movable(arr));

    size_t new_slots_occupied = arr->slots_occupied + num;
    size_t new_slots_allocated = arr->slots_allocated
        ? arr->slots_allocated
        : (arr->flags_slots_preallocate &
           HDAG_ARR_FSP_SLOTS_PREALLOCATE_MASK);

    if (new_slots_allocated == 0) {
        new_slots_allocated = new_slots_occupied;
    } else {
        while (new_slots_occupied > new_slots_allocated) {
            new_slots_allocated <<= 1;
        }
    }

    if (new_slots_allocated != arr->slots_allocated) {
        void *new_slots;

        if (arr->slot_size == 0 || new_slots_allocated == 0) {
            if (arr->slots != NULL) {
                free(arr->slots);
            }
            new_slots = NULL;
        } else {
            assert(!hdag_arr_is_void(arr));
            new_slots = reallocarray(arr->slots, new_slots_allocated,
                                     arr->slot_size);
            if (new_slots == NULL) {
                return NULL;
            }
        }

        arr->slots_allocated = new_slots_allocated;
        arr->slots = new_slots;
    }

    return hdag_arr_slot(arr, arr->slots_occupied);
}

bool
hdag_arr_deflate(struct hdag_arr *arr)
{
    assert(hdag_arr_is_valid(arr));
    const size_t new_slots_allocated = arr->slots_occupied;

    if (hdag_arr_is_pinned(arr)) {
        return true;
    }

    if (new_slots_allocated != arr->slots_allocated) {
        void *new_slots;
        if (arr->slot_size == 0 || new_slots_allocated == 0) {
            if (arr->slots != NULL) {
                free(arr->slots);
            }
            new_slots = NULL;
        } else {
            assert(!hdag_arr_is_void(arr));
            new_slots = reallocarray(arr->slots, new_slots_allocated,
                                     arr->slot_size);
            if (new_slots == NULL) {
                return false;
            }
        }
        arr->slots = new_slots;
        arr->slots_allocated = new_slots_allocated;
    }

    return true;
}

bool
hdag_arr_is_sorted_as(const struct hdag_arr *arr,
                      hdag_cmp_fn cmp, void *data,
                      int cmp_min, int cmp_max)
{
    size_t slot_size;
    uint8_t *prev_slot;
    uint8_t *slot;
    uint8_t *end_slot;
    int rel;
    int rel_min = cmp_min < 0 ? INT_MIN : cmp_min;
    int rel_max = cmp_max > 0 ? INT_MAX : cmp_max;

    assert(hdag_arr_is_valid(arr));
    assert(cmp != NULL);
    assert(hdag_arr_sort_is_valid(cmp_min, cmp_max));

    for (
        slot_size = arr->slot_size,
        prev_slot = arr->slots,
        end_slot = arr->slots + slot_size * arr->slots_occupied;

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
hdag_arr_dedup(struct hdag_arr *arr,
               hdag_cmp_fn cmp, void *data)
{
    size_t slot_size;
    uint8_t *output_slot;
    uint8_t *prev_slot;
    uint8_t *slot;
    uint8_t *end_slot;

    assert(hdag_arr_is_valid(arr));
    assert(hdag_arr_is_variable(arr));
    assert(hdag_arr_is_dynamic(arr));
    assert(cmp != NULL);

    if (arr->slots_occupied > 1) {
        for (
            slot_size = arr->slot_size,
            prev_slot = arr->slots,
            end_slot = arr->slots + slot_size * arr->slots_occupied,
            output_slot = arr->slots;

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

        arr->slots_occupied = (output_slot - (uint8_t *)arr->slots) /
                                arr->slot_size + 1;
    }

    return arr->slots_occupied;
}

int
hdag_arr_mem_cmp(const void *a, const void *b, void *len)
{
    assert(a != NULL);
    assert(b != NULL);
    assert((uintptr_t)len <= SIZE_MAX);
    return memcmp(a, b, (uintptr_t)len);
}

bool
hdag_arr_bsearch(const struct hdag_arr *arr,
                 hdag_cmp_fn cmp, void *data,
                 const void *value,
                 size_t *pidx)
{
    int relation;
    size_t start;
    size_t middle;
    size_t end;

    assert(hdag_arr_is_valid(arr));
    assert(!hdag_arr_is_void(arr));
    assert(cmp != NULL);
    assert(hdag_arr_is_sorted(arr, cmp, data));
    assert(value != NULL);

    for (start = 0, end = arr->slots_occupied; start < end;) {
        middle = (start + end) >> 1;
        relation = cmp(value, hdag_arr_slot_const(arr, middle), data);
        if (relation == 0) {
            start = middle;
            break;
        } if (relation > 0) {
            start = middle + 1;
        } else {
            end = middle;
        }
    }

    if (pidx != NULL) {
        *pidx = start;
    }
    return relation == 0;
}
