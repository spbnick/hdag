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

    /* Test allocating boundary number of elements */
    darr = HDAG_DARR_EMPTY(1, 1);
    TEST(hdag_darr_alloc(&darr, 0) == NULL);
    TEST(hdag_darr_alloc(&darr, 1) != NULL);
    TEST(hdag_darr_uappend(&darr, 0) == NULL);
    hdag_darr_cleanup(&darr);

    /* Test void array operations */
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

    /* Test sorting and deduping empty array */
    darr = HDAG_DARR_EMPTY(1, 16);
    TEST(hdag_darr_is_sorted(&darr, hdag_darr_mem_cmp,
                             (void *)(uintptr_t)darr.slot_size));
    TEST(hdag_darr_is_sorted_and_deduped(&darr, hdag_darr_mem_cmp,
                                         (void *)(uintptr_t)darr.slot_size));
    hdag_darr_sort(&darr, hdag_darr_mem_cmp,
                   (void *)(uintptr_t)darr.slot_size);
    TEST(hdag_darr_is_sorted(&darr, hdag_darr_mem_cmp,
                             (void *)(uintptr_t)darr.slot_size));
    TEST(hdag_darr_dedup(&darr, hdag_darr_mem_cmp,
                         (void *)(uintptr_t)darr.slot_size) == 0);
    TEST(hdag_darr_is_sorted_and_deduped(&darr, hdag_darr_mem_cmp,
                                         (void *)(uintptr_t)darr.slot_size));


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
