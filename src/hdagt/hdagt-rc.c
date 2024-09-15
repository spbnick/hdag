/*
 * Hash DAG database bundle operations test
 */

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

    TEST(hdag_rc_type_is_valid(HDAG_RC_TYPE_OK));
    TEST(hdag_rc_type_is_valid(HDAG_RC_TYPE_GRAPH_CYCLE));
    TEST(hdag_rc_type_is_valid(HDAG_RC_TYPE_NUM - 1));
    TEST(!hdag_rc_type_is_valid(HDAG_RC_TYPE_NUM));
    TEST(!hdag_rc_type_is_valid((enum hdag_rc_type)-1));
    TEST(!hdag_rc_type_is_valid((enum hdag_rc_type)UINT32_MAX));
    TEST(HDAG_RC(0, 0) == 0);
    TEST(hdag_rc_get_type_raw(HDAG_RC(0, 0)) == 0);
    TEST(hdag_rc_get_type_raw(0) == 0);
    TEST(hdag_rc_get_type_raw(HDAG_RC(INT32_MAX, 0)) == INT32_MAX);
    TEST(hdag_rc_is_valid(HDAG_RC_OK));
    TEST(hdag_rc_is_valid(HDAG_RC(HDAG_RC_TYPE_OK, 0)));
    TEST(hdag_rc_is_valid(HDAG_RC(HDAG_RC_TYPE_OK, 1)));
    TEST(hdag_rc_is_valid(HDAG_RC(HDAG_RC_TYPE_OK, -1)));

    TEST(hdag_rc_get_type(HDAG_RC(HDAG_RC_TYPE_ERRNO, 0)) ==
         HDAG_RC_TYPE_ERRNO);
    TEST(hdag_rc_get_value(HDAG_RC(HDAG_RC_TYPE_ERRNO, 0)) == 0);

    fprintf(stderr, "%ld\n", HDAG_RC(HDAG_RC_TYPE_ERRNO, 1));
    fprintf(stderr, "%d\n", hdag_rc_get_type(HDAG_RC(HDAG_RC_TYPE_ERRNO, 1)));
    TEST(hdag_rc_get_type(HDAG_RC(HDAG_RC_TYPE_ERRNO, 1)) ==
         HDAG_RC_TYPE_ERRNO);
    TEST(hdag_rc_get_value(HDAG_RC(HDAG_RC_TYPE_ERRNO, 1)) == 1);

    TEST(hdag_rc_get_type(HDAG_RC(HDAG_RC_TYPE_ERRNO, -1)) ==
         HDAG_RC_TYPE_ERRNO);
    TEST(hdag_rc_get_value(HDAG_RC(HDAG_RC_TYPE_ERRNO, -1)) == -1);

    TEST(hdag_rc_get_type(HDAG_RC(HDAG_RC_TYPE_ERRNO, INT32_MAX)) ==
         HDAG_RC_TYPE_ERRNO);
    TEST(hdag_rc_get_value(HDAG_RC(HDAG_RC_TYPE_ERRNO, INT32_MAX)) ==
         INT32_MAX);

    TEST(hdag_rc_get_type(HDAG_RC(HDAG_RC_TYPE_ERRNO, INT32_MIN)) ==
         HDAG_RC_TYPE_ERRNO);
    TEST(hdag_rc_get_value(HDAG_RC(HDAG_RC_TYPE_ERRNO, INT32_MIN)) ==
         INT32_MIN);

    errno = INT32_MAX;
    TEST(hdag_rc_get_type(HDAG_RC_ERRNO) == HDAG_RC_TYPE_ERRNO);
    TEST(hdag_rc_get_value(HDAG_RC_ERRNO) == INT32_MAX);
    errno = INT32_MIN;
    TEST(hdag_rc_get_type(HDAG_RC_ERRNO) == HDAG_RC_TYPE_ERRNO);
    TEST(hdag_rc_get_value(HDAG_RC_ERRNO) == INT32_MIN);
    errno = 0;
    TEST(hdag_rc_get_type(HDAG_RC_ERRNO) == HDAG_RC_TYPE_ERRNO);
    TEST(hdag_rc_get_value(HDAG_RC_ERRNO) == 0);

    TEST(hdag_rc_get_type(HDAG_RC_GRAPH_CYCLE) == HDAG_RC_TYPE_GRAPH_CYCLE);

#define TEST_STRERROR(_rc, _str) \
    do {                                                                \
        char _buf[256];                                                 \
        printf("%s\n", hdag_rc_strerror_r(_rc, _buf, sizeof(_buf)));    \
        TEST(strcmp(hdag_rc_strerror_r(_rc, _buf, sizeof(_buf)),        \
                    _str) == 0);                                        \
        printf("%s\n", hdag_rc_strerror(_rc));                          \
        TEST(strcmp(hdag_rc_strerror(_rc), _str) == 0);                 \
    } while(0)

    TEST_STRERROR(HDAG_RC_OK, "Success");
    TEST_STRERROR(HDAG_RC_ERRNO_ARG(EINVAL),
                  "ERRNO: EINVAL: Invalid argument");
    errno = EEXIST;
    TEST_STRERROR(HDAG_RC_ERRNO, "ERRNO: EEXIST: File exists");
    errno = 0;
    TEST_STRERROR(HDAG_RC_GRAPH_CYCLE, "Graph contains a cycle");

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
