/*
 * Command-line tool creating a hash DAG database file from a text adjacency
 * list
 */
#include <hdag/file.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

/**
 * Output command-line usage information to specified stream.
 *
 * @param stream    The stream to output the usage information to.
 */
void
usage(FILE *stream)
{
    fprintf(stream,
            "Usage: %s HASH_LEN\n"
            "Create an HDAG file from an adjacency list text file\n",
            program_invocation_short_name);
}

int
main(int argc, const char **argv)
{
    hdag_res res = HDAG_RES_INVALID;
    struct hdag_file file = HDAG_FILE_CLOSED;
    unsigned long hash_len;
    char *end;

    (void)argv;

    if (argc != 2) {
        fprintf(stderr, "Invalid number of arguments\n");
        usage(stderr);
        return 1;
    }

    if ((hash_len = strtoul(argv[1], &end, 10)) >= UINT16_MAX ||
        end == argv[1] || *end != '\0' ||
        !hdag_hash_len_is_valid((uint16_t)hash_len)) {
        fprintf(stderr, "Invalid HASH_LEN: \"%s\"\n", argv[1]);
        usage(stderr);
        return 1;
    }

    HDAG_RES_TRY(hdag_file_from_txt(&file, NULL, -1, 0,
                                    stdin, (uint16_t)hash_len));
    if (fwrite(file.contents, file.size, 1, stdout) != 1) {
        res = HDAG_RES_ERRNO;
        goto cleanup;
    }
    HDAG_RES_TRY(hdag_file_close(&file));

    res = HDAG_RES_OK;
cleanup:
    (void)hdag_file_close(&file);
    if (hdag_res_is_ok(res)) {
        return 0;
    }
    fprintf(stderr, "ERROR: %s\n", hdag_res_str(res));
    return 1;
}
