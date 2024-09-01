/*
 * Hash DAG database file operations test
 */

#include <hdag/file.h>
#include <hdag/rc.h>
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
    struct hdag_file file = HDAG_FILE_CLOSED;
    char pathname[sizeof(file.pathname)];
    uint8_t expected_contents[] = {
        0x48, 0x44, 0x41, 0x47, 0x00, 0x00, 0x20, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    };

    /*
     * In-memory file.
     */
    TEST(!hdag_file_create(&file, "", -1, 0, 256 / 8, HDAG_NODE_SEQ_EMPTY));
    TEST(file.size = sizeof(expected_contents));
    TEST(memcmp(file.contents, expected_contents, file.size) == 0);
    TEST(!hdag_file_close(&file));

    /*
     * Create on-disk file.
     */
    TEST(!hdag_file_create(&file, "test.XXXXXX.hdag", 5,
                           S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP,
                           256 / 8, HDAG_NODE_SEQ_EMPTY));
    TEST(hdag_file_is_open(&file));
    /* Remember created file pathname */
    memcpy(pathname, file.pathname, sizeof(pathname));
    TEST(!hdag_file_sync(&file));
    TEST(file.size == sizeof(expected_contents));
    TEST(memcmp(file.contents, expected_contents, file.size) == 0);
    TEST(!hdag_file_close(&file));
    TEST(!hdag_file_is_open(&file));

    /*
     * Open (the created) on-disk file.
     */
    TEST(!hdag_file_open(&file, pathname));
    TEST(hdag_file_is_open(&file));
    TEST(file.size == sizeof(expected_contents));
    TEST(memcmp(file.contents, expected_contents, file.size) == 0);
    TEST(!hdag_file_close(&file));
    TEST(unlink(pathname) == 0);
    TEST(sizeof(struct hdag_file_header) == 4 + 2 + 2 + 4 + 4);

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
