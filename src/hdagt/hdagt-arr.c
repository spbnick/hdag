/*
 * Hash DAG database dynamic array tests
 */

#include <hdag/misc.h>
#include <hdag/arr.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#define TEST(_expr) \
    do {                                                \
        if (!(_expr)) {                                 \
            fprintf(stderr, "%s:%u: Test failed: %s\n", \
                    __FILE__, __LINE__, #_expr);        \
            failed++;                                   \
        }                                               \
    } while(0)

static size_t
test(void)
{
    size_t failed = 0;
    struct hdag_arr arr;

    /* Allocating boundary number of elements */
    arr = HDAG_ARR_EMPTY(1, 1);
    TEST(hdag_arr_alloc(&arr, 0) == NULL);
    TEST(hdag_arr_alloc(&arr, 1) != NULL);
    TEST(hdag_arr_uappend(&arr, 0) == NULL);
    hdag_arr_cleanup(&arr);

    /* Void array operations */
    arr = HDAG_ARR_EMPTY(0, 16);
    TEST(hdag_arr_is_valid(&arr));
    TEST(hdag_arr_is_void(&arr));
    TEST(hdag_arr_is_clean(&arr));
    TEST(hdag_arr_slots_occupied(&arr) == 0);
    TEST(hdag_arr_size_occupied(&arr) == 0);
    TEST(hdag_arr_slots_allocated(&arr) == 0);
    TEST(hdag_arr_size_allocated(&arr) == 0);
    TEST(hdag_arr_is_empty(&arr));
    hdag_arr_empty(&arr);
    TEST(hdag_arr_is_valid(&arr));
    TEST(hdag_arr_is_void(&arr));
    TEST(hdag_arr_is_empty(&arr));
    hdag_arr_cleanup(&arr);
    TEST(hdag_arr_is_valid(&arr));
    TEST(hdag_arr_is_void(&arr));
    TEST(hdag_arr_is_clean(&arr));
    TEST(hdag_arr_alloc(&arr, 0) == NULL);
    TEST(hdag_arr_deflate(&arr));
    TEST(hdag_arr_uappend(&arr, 0) == NULL);
    hdag_arr_cleanup(&arr);

    /* Sort/dedup case */
    struct sd_case {
        const char *input;
        bool input_sorted;
        bool input_deduped;
        const char *sorted;
        const char *deduped;
    };
    /* Sort/dedup cases */
    const struct sd_case sd_cases[] = {
        {"", true, true, "", ""},
        {"1", true, true, "1", "1"},
        {"12", true, true, "12", "12"},
        {"21", false, true, "12", "12"},
        {"22", true, false, "22", "2"},
        {"212", false, false, "122", "12"},
        {"333", true, false, "333", "3"},
        {"123454321", false, false, "112233445", "12345"},
        {NULL,},
    };
    const struct sd_case *sd_case;

    /* Verify sort/dedup cases */
    for (sd_case = sd_cases; sd_case->input != NULL; sd_case++) {
        fprintf(
            stderr,
            "SORT/DEDUP CASE: \"%s\" (%s, %s) -> \"%s\" -> \"%s\"\n",
            sd_case->input,
            (sd_case->input_sorted ? "sorted" : "UNsorted"),
            (sd_case->input_deduped ? "deduped": "UNdeduped"),
            sd_case->sorted,
            sd_case->deduped
        );

        /* Fill and check */
        arr = HDAG_ARR_EMPTY(1, 0);
        assert((hdag_arr_append(
            &arr, sd_case->input, strlen(sd_case->input
        )) == NULL) == (*sd_case->input == '\0'));
        TEST(hdag_arr_is_sorted(
            &arr, hdag_cmp_mem, (void *)(uintptr_t)arr.slot_size
        ) == sd_case->input_sorted);
        TEST(hdag_arr_is_sorted_and_deduped(
            &arr, hdag_cmp_mem, (void *)(uintptr_t)arr.slot_size
        ) == (sd_case->input_sorted && sd_case->input_deduped));

        /* Sort and check */
        hdag_arr_sort(&arr, hdag_cmp_mem,
                      (void *)(uintptr_t)arr.slot_size);
        TEST(arr.slots_occupied == strlen(sd_case->sorted));
        TEST(memcmp(arr.slots, sd_case->sorted, arr.slots_occupied) == 0);
        TEST(hdag_arr_is_sorted(&arr, hdag_cmp_mem,
                                (void *)(uintptr_t)arr.slot_size));
        TEST(hdag_arr_is_sorted_and_deduped(
            &arr, hdag_cmp_mem, (void *)(uintptr_t)arr.slot_size
        ) == sd_case->input_deduped);

        /* Dedup and check */
        TEST(hdag_arr_dedup(
            &arr, hdag_cmp_mem, (void *)(uintptr_t)arr.slot_size
        ) == strlen(sd_case->deduped));
        TEST(arr.slots_occupied == strlen(sd_case->deduped));
        TEST(memcmp(arr.slots, sd_case->deduped, arr.slots_occupied) == 0);
        TEST(hdag_arr_is_sorted(&arr, hdag_cmp_mem,
                                (void *)(uintptr_t)arr.slot_size));
        TEST(hdag_arr_is_sorted_and_deduped(&arr, hdag_cmp_mem,
                                            (void *)(uintptr_t)arr.slot_size));

        /* Cleanup */
        hdag_arr_cleanup(&arr);
    }

    return failed;
}

int
main(void)
{
    size_t failed = test();
    if (failed) {
        fprintf(stderr, "%zu tests failed.\n", failed);
    }
    return failed != 0;
}
