/*
 * Hash DAG database miscellaneous operations test
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

    uint8_t hash[] = {0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef};
    char hex_buf[sizeof(hash) * 2 + 1];

    TEST(hdag_bytes_to_hex(hex_buf, hash, sizeof(hash)) == hex_buf);
    TEST(strcmp(hex_buf, "0123456789abcdef") == 0);

    {
        struct hdag_darr darr = HDAG_DARR_EMPTY(1, 1);

        TEST(hdag_darr_alloc(&darr, 0) == NULL);
        TEST(hdag_darr_alloc(&darr, 1) != NULL);
        TEST(hdag_darr_uappend(&darr, 0) == NULL);
        hdag_darr_cleanup(&darr);
    }

    {
        struct hdag_darr darr = HDAG_DARR_EMPTY(0, 16);

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
    }

    {
        struct member {
            int b;
        };
        struct container {
            int a;
            struct member member;
            int c;
        };
        struct container container = {
            .a = 'a',
            .member = {.b = 'b'},
            .c = 'c',
        };
        struct member *pmember = &container.member;
        const struct member *const_pmember = &container.member;
        struct container *pcontainer;
        const struct container *const_pcontainer;

        pcontainer = HDAG_CONTAINER_OF(
            struct container, member, pmember
        );
        const_pcontainer = HDAG_CONTAINER_OF(
            struct container, member, const_pmember
        );
        TEST(pcontainer == &container);
        TEST(const_pcontainer == &container);
#ifdef HDAG_COMPILE_TEST
        /* This should fail */
        pcontainer = HDAG_CONTAINER_OF(
            struct container, member, const_pmember
        );
#endif
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
