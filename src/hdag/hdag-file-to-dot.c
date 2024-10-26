/*
 * Command-line tool dumping a hash DAG database file into DOT format.
 */
#include <hdag/file.h>
#include <hdag/dot.h>
#include <stdio.h>
#include <stdlib.h>

/**
 * Output command-line usage information to specified stream.
 *
 * @param stream    The stream to output the usage information to.
 */
void
usage(FILE *stream)
{
    fprintf(stream,
            "Usage: %s FILE\n"
            "Dump an HDAG file into a DOT file\n",
            program_invocation_short_name);
}

int
main(int argc, const char **argv)
{
    hdag_res res = HDAG_RES_INVALID;
    struct hdag_file file = HDAG_FILE_CLOSED;
    struct hdag_bundle bundle = HDAG_BUNDLE_EMPTY(4);
    const char *pathname;

    (void)argv;

    if (argc != 2) {
        fprintf(stderr, "Invalid number of arguments\n");
        usage(stderr);
        return 1;
    }

    pathname = argv[1];

    HDAG_RES_TRY(hdag_file_open(&file, pathname));
    HDAG_RES_TRY(hdag_file_to_bundle(&bundle, &file));
    HDAG_RES_TRY(hdag_dot_write_bundle(&bundle, "", stdout));
    HDAG_RES_TRY(hdag_file_close(&file));

    res = HDAG_RES_OK;
cleanup:
    hdag_bundle_cleanup(&bundle);
    (void)hdag_file_close(&file);
    if (hdag_res_is_ok(res)) {
        return 0;
    }
    fprintf(stderr, "ERROR: %s\n", hdag_res_str(res));
    return 1;
}
