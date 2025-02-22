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

    /* Test hdag_bytes_to_hex */
    TEST(hdag_bytes_to_hex(hex_buf, hash, sizeof(hash)) == hex_buf);
    TEST(strcmp(hex_buf, "0123456789abcdef") == 0);
    TEST(hdag_bytes_to_hex(hex_buf, NULL, 0) == hex_buf);

    /* Test HDAG_CONTAINER_OF() */
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
