/*
 * Hash DAG bundle - universal result
 */

#ifndef _HDAG_RES_H
#define _HDAG_RES_H

#include <hdag/fault.h>
#include <hdag/misc.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <stdbool.h>

/**
 * A universal result.
 *
 * Negative values signify failure and encode a fault (failure cause/type)
 * and an optional signed error code.
 * Non-negative values signify success and store a return value.
 *
 * A universal result which is not expected to store a meaningful return
 * value, beside the fact of success, is called a "void result".
 */
typedef int64_t hdag_res;

/**
 * Extract the fault from a result without validation.
 *
 * @param res    The result to extract the fault from (not validated).
 *
 * @return The extracted fault (as is, not validated).
 */
static inline enum hdag_fault
hdag_res_get_fault_raw(hdag_res res)
{
    return (-res) / ((hdag_res)UINT32_MAX + 1);
}

/**
 * Check if a result is valid.
 *
 * @param res    The result to check.
 *
 * @return True if the result is valid, false otherwise.
 */
static inline bool
hdag_res_is_valid(hdag_res res)
{
    return res >= 0 || hdag_fault_is_valid(hdag_res_get_fault_raw(res));
}

/**
 * Check if a result is a success.
 *
 * @param res    The result to check.
 *
 * @return True if the result is a success, false otherwise.
 */
static inline bool
hdag_res_is_success(hdag_res res)
{
    assert(hdag_res_is_valid(res));
    return res >= 0;
}

/**
 * Check if a result is a failure.
 *
 * @param res    The result to check.
 *
 * @return True if the result is a failure, false otherwise.
 */
static inline bool
hdag_res_is_failure(hdag_res res)
{
    assert(hdag_res_is_valid(res));
    return res < 0;
}

/**
 * Validate a result.
 *
 * @param res    The result to validate.
 *
 * @return The validated result.
 */
static inline hdag_res
hdag_res_validate(hdag_res res)
{
    assert(hdag_res_is_valid(res));
    return res;
}

/**
 * Extract the fault from a failure result.
 *
 * @param res   The result to extract the fault from.
 *              Must be a failure.
 *
 * @return The extracted fault (validated).
 */
static inline enum hdag_fault
hdag_res_get_fault(hdag_res res)
{
    assert(hdag_res_is_valid(res));
    assert(hdag_res_is_failure(res));
    return hdag_fault_validate(hdag_res_get_fault_raw(res));
}

/**
 * Extract the (error) code from a failure result.
 *
 * @param res   The result to extract the code from.
 *              Must be a failure.
 *
 * @return The extracted code.
 */
static inline int32_t
hdag_res_get_code(hdag_res res)
{
    assert(hdag_res_is_valid(res));
    assert(hdag_res_is_failure(res));
    return (uint32_t)((-res) % ((hdag_res)UINT32_MAX + 1));
}

/** The void successful result - success without return value */
#define HDAG_RES_OK ((hdag_res)0)

/** Create a failure result from a fault and a code */
#define HDAG_RES_FAILURE(_fault, _code) (-( \
    (hdag_res)(int32_t)_fault * ((hdag_res)UINT32_MAX + 1) +    \
    (hdag_res)(uint32_t)(int32_t)(_code)                        \
))

/** Create a failure result from a errno code */
#define HDAG_RES_ERRNO_ARG(_errno) HDAG_RES_FAILURE(HDAG_FAULT_ERRNO, _errno)

/** Create a failure result from the current errno code */
#define HDAG_RES_ERRNO HDAG_RES_ERRNO_ARG(errno)

/** The graph cycle failure result */
#define HDAG_RES_GRAPH_CYCLE HDAG_RES_FAILURE(HDAG_FAULT_GRAPH_CYCLE, 0)

/** The invalid (file) format failure result */
#define HDAG_RES_INVALID_FORMAT HDAG_RES_FAILURE(HDAG_FAULT_INVALID_FORMAT, 0)

/** The conflicting node info failure result */
#define HDAG_RES_NODE_CONFLICT HDAG_RES_FAILURE(HDAG_FAULT_NODE_CONFLICT, 0)

/** The invalid result */
#define HDAG_RES_INVALID INT64_MIN

/**
 * Replace the invalid result with the current errno result.
 * Return everything else unchanged.
 */
#define HDAG_RES_ERRNO_IF_INVALID(_res) ({ \
    hdag_res __res = (_res);                \
    if (__res == HDAG_RES_INVALID) {        \
        __res = HDAG_RES_ERRNO;             \
    }                                       \
    __res;                                  \
})

/**
 * Check that the specified result is a success.
 * If it is, return the result.
 * If it's not, store it in the variable called
 * "res", and go to label "cleanup".
 */
#define HDAG_RES_TRY(_res) ({ \
    hdag_res __res = (_res);            \
    if (hdag_res_is_failure(__res)) {   \
        res = __res;                    \
        goto cleanup;                   \
    }                                   \
    __res;                              \
})

/**
 * Create a failure result from a fault and a code.
 *
 * @param fault The fault of the result being created.
 * @param code  The code of the result being created.
 *
 * @return The created (and validated) failure result.
 */
static inline hdag_res
hdag_res_failure(enum hdag_fault fault, int32_t code)
{
    hdag_res res = HDAG_RES_FAILURE(hdag_fault_validate(fault), code);
    assert(hdag_res_is_valid(res));
    assert(hdag_res_is_failure(res));
    return res;
}

/**
 * Return a string describing a result, possibly
 * outputting it to the provided buffer.
 *
 * @param res   The result to describe.
 * @param buf   The buffer to write the (null-terminated) string to.
 *              Can be NULL, if size is zero.
 * @param size  The size of the buffer, bytes. A longer description will be
 *              truncated to fit the buffer.
 *
 * @return The description of the result: either the provided buffer,
 *         or a constant string.
 */
extern const char *hdag_res_str_r(hdag_res res, char *buf, size_t size);

/**
 * Return the string describing a result, possibly serving it via an internal
 * static buffer.
 *
 * @param res    The result to describe.
 *
 * @return The description of the result.
 */
extern const char *hdag_res_str(hdag_res res);

/** Comparison results representable in a universal result */
enum hdag_res_cmp {
    /** The first value is less than the second */
    HDAG_RES_CMP_LT = 0,
    /** The first value is equal to the second */
    HDAG_RES_CMP_EQ = 1,
    /** The first value is greater than the second */
    HDAG_RES_CMP_GT = 2,
    /** The number of non-fault comparison results (not a result itself) */
    HDAG_RES_CMP_NUM
};

/**
 * Check if a universal comparison result is valid.
 *
 * @param res   The result to check.
 *
 * @return True if the result is valid, false otherwise.
 */
static inline bool
hdag_res_cmp_is_valid(hdag_res res)
{
    return res < HDAG_RES_CMP_NUM;
}

/**
 * Validate a universal comparison result.
 *
 * @param res   The result to validate.
 *
 * @return The validated result.
 */
static inline hdag_res
hdag_res_cmp_validate(hdag_res res)
{
    assert(hdag_res_cmp_is_valid(res));
    return res;
}

/**
 * Convert a universal comparison result to a bare comparison result.
 *
 * @param res   The result to convert. Must not be a fault.
 *
 * @return The normal bare comparison result, in [-1, 1] range.
 */
static inline int
hdag_res_cmp_to_cmp(hdag_res res)
{
    assert(hdag_res_is_success(res));
    assert(hdag_res_cmp_is_valid(res));
    return hdag_cmp_verify_normal(res - 1);
}

/**
 * Convert a bare comparison result to a universal comparison result.
 *
 * @param cmp   The result to convert.
 *
 * @return The universal comparison result.
 */
static inline hdag_res
hdag_res_cmp_from_cmp(int cmp)
{
    hdag_res res = hdag_cmp_normalize(cmp) + 1;
    assert(hdag_res_is_success(res));
    assert(hdag_res_cmp_is_valid(res));
    return res;
}

/**
 * The prototype for an abstract value comparison function returning a
 * universal result.
 *
 * @param first     The first value to compare.
 * @param second    The second value to compare.
 * @param data      The function's private data.
 *
 * @return A universal result code:
 *         HDAG_RES_CMP_LT - 0 - first < second,
 *         HDAG_RES_CMP_EQ - 1 - first == second,
 *         HDAG_RES_CMP_GT - 2 - first > second,
 *         or a fault.
 */
typedef hdag_res (*hdag_res_cmp_fn)(const void *first, const void *second,
                                    void *data);

/**
 * Compare two abstract values using memcmp,
 * return universal comparison result.
 *
 * @param first     The first value to compare.
 * @param second    The second value to compare.
 * @param data      The size of each value (uintptr_t).
 *
 * @return Universal comparison result.
 */
extern hdag_res hdag_res_cmp_mem(const void *first, const void *second, void *data);

#endif /* _HDAG_RES_H */
