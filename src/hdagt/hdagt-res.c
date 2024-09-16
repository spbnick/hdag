/*
 * Hash DAG database bundle operations test
 */

#include <hdag/res.h>
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

    TEST(hdag_fault_is_valid(HDAG_FAULT_NONE));
    TEST(hdag_fault_is_valid(HDAG_FAULT_GRAPH_CYCLE));
    TEST(hdag_fault_is_valid(HDAG_FAULT_NUM - 1));
    TEST(!hdag_fault_is_valid(HDAG_FAULT_NUM));
    TEST(!hdag_fault_is_valid((enum hdag_fault)-1));
    TEST(!hdag_fault_is_valid((enum hdag_fault)UINT32_MAX));
    TEST(HDAG_RES_FAILURE(0, 0) == 0);
    TEST(hdag_res_get_fault_raw(HDAG_RES_FAILURE(0, 0)) == 0);
    TEST(hdag_res_get_fault_raw(0) == 0);
    TEST(hdag_res_get_fault_raw(HDAG_RES_FAILURE(INT32_MAX, 0)) == INT32_MAX);
    TEST(hdag_res_is_valid(HDAG_RES_OK));
    TEST(hdag_res_is_valid(HDAG_RES_FAILURE(HDAG_FAULT_NONE, 0)));
    TEST(hdag_res_is_valid(HDAG_RES_FAILURE(HDAG_FAULT_NONE, 1)));
    TEST(hdag_res_is_valid(HDAG_RES_FAILURE(HDAG_FAULT_NONE, -1)));

    TEST(hdag_res_get_fault(HDAG_RES_FAILURE(HDAG_FAULT_ERRNO, 0)) ==
         HDAG_FAULT_ERRNO);
    TEST(hdag_res_get_code(HDAG_RES_FAILURE(HDAG_FAULT_ERRNO, 0)) == 0);

    TEST(hdag_res_get_fault(HDAG_RES_FAILURE(HDAG_FAULT_ERRNO, 1)) ==
         HDAG_FAULT_ERRNO);
    TEST(hdag_res_get_code(HDAG_RES_FAILURE(HDAG_FAULT_ERRNO, 1)) == 1);

    TEST(hdag_res_get_fault(HDAG_RES_FAILURE(HDAG_FAULT_ERRNO, -1)) ==
         HDAG_FAULT_ERRNO);
    TEST(hdag_res_get_code(HDAG_RES_FAILURE(HDAG_FAULT_ERRNO, -1)) == -1);

    TEST(hdag_res_get_fault(HDAG_RES_FAILURE(HDAG_FAULT_ERRNO, INT32_MAX)) ==
         HDAG_FAULT_ERRNO);
    TEST(hdag_res_get_code(HDAG_RES_FAILURE(HDAG_FAULT_ERRNO, INT32_MAX)) ==
         INT32_MAX);

    TEST(hdag_res_get_fault(HDAG_RES_FAILURE(HDAG_FAULT_ERRNO, INT32_MIN)) ==
         HDAG_FAULT_ERRNO);
    TEST(hdag_res_get_code(HDAG_RES_FAILURE(HDAG_FAULT_ERRNO, INT32_MIN)) ==
         INT32_MIN);

    errno = INT32_MAX;
    TEST(hdag_res_get_fault(HDAG_RES_ERRNO) == HDAG_FAULT_ERRNO);
    TEST(hdag_res_get_code(HDAG_RES_ERRNO) == INT32_MAX);
    errno = INT32_MIN;
    TEST(hdag_res_get_fault(HDAG_RES_ERRNO) == HDAG_FAULT_ERRNO);
    TEST(hdag_res_get_code(HDAG_RES_ERRNO) == INT32_MIN);
    errno = 0;
    TEST(hdag_res_get_fault(HDAG_RES_ERRNO) == HDAG_FAULT_ERRNO);
    TEST(hdag_res_get_code(HDAG_RES_ERRNO) == 0);

    TEST(hdag_res_get_fault(HDAG_RES_GRAPH_CYCLE) == HDAG_FAULT_GRAPH_CYCLE);

#define TEST_STRERROR(_res, _str) \
    do {                                                            \
        char _buf[256];                                             \
        printf("%s\n", hdag_res_str_r(_res, _buf, sizeof(_buf)));   \
        TEST(strcmp(hdag_res_str_r(_res, _buf, sizeof(_buf)),       \
                    _str) == 0);                                    \
        printf("%s\n", hdag_res_str(_res));                         \
        TEST(strcmp(hdag_res_str(_res), _str) == 0);                \
    } while(0)

    TEST_STRERROR(HDAG_RES_OK, "Success: 0");
    TEST_STRERROR(HDAG_RES_ERRNO_ARG(EINVAL),
                  "ERRNO: EINVAL: Invalid argument");
    errno = EEXIST;
    TEST_STRERROR(HDAG_RES_ERRNO, "ERRNO: EEXIST: File exists");
    errno = 0;
    TEST_STRERROR(HDAG_RES_GRAPH_CYCLE, "Graph contains a cycle");

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
