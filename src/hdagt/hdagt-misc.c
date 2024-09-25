/*
 * Hash DAG database miscellaneous operations test
 */

#include <hdag/misc.h>
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
