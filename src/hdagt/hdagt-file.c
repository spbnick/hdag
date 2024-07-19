/*
 * Hash DAG database file operations test
 */

#include <hdag/file.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

int
main(void)
{
    struct hdag_file file = HDAG_FILE_CLOSED;
    char pathname[sizeof(file.pathname)];
    uint8_t expected_contents[] = {
        0x48, 0x44, 0x41, 0x47, 0x00, 0x00, 0x20, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    };

    /*
     * In-memory file.
     */
    if (!hdag_file_create(&file, "", -1, 0, 256 / 8, HDAG_NODE_SEQ_EMPTY)) {
        printf("Failed creating in-memory file: %s\n", strerror(errno));
        return 1;
    }

    if (file.size != sizeof(expected_contents)) {
        printf("Unexpected file size: %zu != %zu\n", file.size,
                sizeof(expected_contents));
        return 1;
    }

    if (memcmp(file.contents, expected_contents, file.size) != 0) {
        printf("Unexpected file contents\n");
        return 1;
    }

    if (!hdag_file_close(&file)) {
        printf("Failed closing in-memory file: %s\n", strerror(errno));
        return 1;
    }

    /*
     * Create on-disk file.
     */
    if (!hdag_file_create(&file, "test.XXXXXX.hdag", 5,
                          S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP,
                          256 / 8, HDAG_NODE_SEQ_EMPTY)) {
        printf("Failed creating on-disk file: %s\n", strerror(errno));
        return 1;
    }

    if (!hdag_file_is_open(&file)) {
        printf("File is considered closed after creating\n");
        return 1;
    }

    /* Remember created file pathname */
    memcpy(pathname, file.pathname, sizeof(pathname));

    if (!hdag_file_sync(&file)) {
        printf("Failed syncing to on-disk file: %s\n", strerror(errno));
        return 1;
    }

    if (file.size != sizeof(expected_contents)) {
        printf("Unexpected file size: %zu != %zu\n", file.size,
                sizeof(expected_contents));
        return 1;
    }

    if (memcmp(file.contents, expected_contents, file.size) != 0) {
        printf("Unexpected file contents\n");
        return 1;
    }

    if (!hdag_file_close(&file)) {
        printf("Failed closing created on-disc file: %s\n", strerror(errno));
        return 1;
    }

    if (hdag_file_is_open(&file)) {
        printf("File is considered open after closing\n");
        return 1;
    }

    /*
     * Open (the created) on-disk file.
     */
    if (!hdag_file_open(&file, pathname)) {
        printf("Failed opening on-disk file \"%s\": %s\n",
               pathname, strerror(errno));
        return 1;
    }

    if (!hdag_file_is_open(&file)) {
        printf("File is considered closed after opening\n");
        return 1;
    }

    if (file.size != sizeof(expected_contents)) {
        printf("Unexpected file size: %zu != %zu\n", file.size,
                sizeof(expected_contents));
        return 1;
    }

    if (memcmp(file.contents, expected_contents, file.size) != 0) {
        printf("Unexpected file contents\n");
        return 1;
    }

    if (!hdag_file_close(&file)) {
        printf("Failed closing opened on-disc file: %s\n", strerror(errno));
        return 1;
    }

    if (unlink(pathname) != 0) {
        printf("Failed to unlink the \"%s\" file: %s\n",
               pathname, strerror(errno));
        return 1;
    }

    return !(sizeof(struct hdag_file_header) == 4 + 2 + 2 + 4 + 4);
}
