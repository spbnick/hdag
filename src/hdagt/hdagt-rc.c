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
    TEST(!hdag_rc_type_is_valid(HDAG_RC_TYPE_NUM));
    TEST(!hdag_rc_type_is_valid((enum hdag_rc_type)-1));
    TEST(!hdag_rc_type_is_valid((enum hdag_rc_type)UINT32_MAX));
    TEST(hdag_rc_get_type_raw((enum hdag_rc_type)UINT32_MAX) == UINT32_MAX);
    TEST(HDAG_RC(HDAG_RC_TYPE_ERRNO, 0) == 0x0000000000000001UL);
    TEST(HDAG_RC(HDAG_RC_TYPE_ERRNO, 1) == 0x0000000100000001UL);
    TEST(HDAG_RC(HDAG_RC_TYPE_ERRNO, -1) == 0xffffffff00000001UL);
    TEST(HDAG_RC(HDAG_RC_TYPE_ERRNO, INT32_MAX) == 0x7fffffff00000001UL);
    TEST(HDAG_RC(HDAG_RC_TYPE_ERRNO, INT32_MIN) == 0x8000000000000001UL);
    errno = INT32_MAX;
    TEST(HDAG_RC_ERRNO == 0x7fffffff00000001UL);
    errno = INT32_MIN;
    TEST(HDAG_RC_ERRNO == 0x8000000000000001UL);
    errno = 0;
    TEST(HDAG_RC_ERRNO == 0x0000000000000001UL);
    TEST(HDAG_RC_GRAPH_CYCLE == 2);

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
