/*
 * Hash DAG database dynamic array tests
 */

#include <hdag/misc.h>
#include <hdag/darr.h>
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
    struct hdag_darr darr;

    /* Allocating boundary number of elements */
    darr = HDAG_DARR_EMPTY(1, 1);
    TEST(hdag_darr_alloc(&darr, 0) == NULL);
    TEST(hdag_darr_alloc(&darr, 1) != NULL);
    TEST(hdag_darr_uappend(&darr, 0) == NULL);
    hdag_darr_cleanup(&darr);

    /* Void array operations */
    darr = HDAG_DARR_EMPTY(0, 16);
    TEST(hdag_darr_is_valid(&darr));
    TEST(hdag_darr_is_void(&darr));
    TEST(hdag_darr_is_clean(&darr));
    TEST(hdag_darr_occupied_slots(&darr) == 0);
    TEST(hdag_darr_occupied_size(&darr) == 0);
    TEST(hdag_darr_allocated_slots(&darr) == 0);
    TEST(hdag_darr_allocated_size(&darr) == 0);
    TEST(hdag_darr_is_empty(&darr));
    hdag_darr_empty(&darr);
    TEST(hdag_darr_is_valid(&darr));
    TEST(hdag_darr_is_void(&darr));
    TEST(hdag_darr_is_empty(&darr));
    hdag_darr_cleanup(&darr);
    TEST(hdag_darr_is_valid(&darr));
    TEST(hdag_darr_is_void(&darr));
    TEST(hdag_darr_is_clean(&darr));
    TEST(hdag_darr_alloc(&darr, 0) == NULL);
    TEST(hdag_darr_deflate(&darr));
    TEST(hdag_darr_uappend(&darr, 0) == NULL);
    hdag_darr_cleanup(&darr);

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
        darr = HDAG_DARR_EMPTY(1, 0);
        assert((hdag_darr_append(
            &darr, sd_case->input, strlen(sd_case->input
        )) == NULL) == (*sd_case->input == '\0'));
        TEST(hdag_darr_is_sorted(
            &darr, hdag_darr_mem_cmp, (void *)(uintptr_t)darr.slot_size
        ) == sd_case->input_sorted);
        TEST(hdag_darr_is_sorted_and_deduped(
            &darr, hdag_darr_mem_cmp, (void *)(uintptr_t)darr.slot_size
        ) == (sd_case->input_sorted && sd_case->input_deduped));

        /* Sort and check */
        hdag_darr_sort(&darr, hdag_darr_mem_cmp,
                       (void *)(uintptr_t)darr.slot_size);
        TEST(darr.slots_occupied == strlen(sd_case->sorted));
        TEST(memcmp(darr.slots, sd_case->sorted, darr.slots_occupied) == 0);
        TEST(hdag_darr_is_sorted(&darr, hdag_darr_mem_cmp,
                                 (void *)(uintptr_t)darr.slot_size));
        TEST(hdag_darr_is_sorted_and_deduped(
            &darr, hdag_darr_mem_cmp, (void *)(uintptr_t)darr.slot_size
        ) == sd_case->input_deduped);

        /* Dedup and check */
        TEST(hdag_darr_dedup(
            &darr, hdag_darr_mem_cmp, (void *)(uintptr_t)darr.slot_size
        ) == strlen(sd_case->deduped));
        TEST(darr.slots_occupied == strlen(sd_case->deduped));
        TEST(memcmp(darr.slots, sd_case->deduped, darr.slots_occupied) == 0);
        TEST(hdag_darr_is_sorted(&darr, hdag_darr_mem_cmp,
                                 (void *)(uintptr_t)darr.slot_size));
        TEST(hdag_darr_is_sorted_and_deduped(&darr, hdag_darr_mem_cmp,
                                             (void *)(uintptr_t)darr.slot_size));

        /* Cleanup */
        hdag_darr_cleanup(&darr);
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
