/*
 * Hash DAG bundle - universal return code
 */

#ifndef _HDAG_RC_H
#define _HDAG_RC_H

#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <stdbool.h>

/**
 * A universal return code.
 *
 * Contains the negated (non-positive) type of return code in the higher 32
 * bits, and the optional signed value in the lower 32 bits. The value
 * presence and meaning is defined by the type.
 *
 * Always non-positive.
 */
typedef int64_t hdag_rc;

/** A return code type */
enum hdag_rc_type {
    /** The operation completed successfully (no value) */
    HDAG_RC_TYPE_OK = 0,
    /** The operation failed due to a libc/system error, the value is errno */
    HDAG_RC_TYPE_ERRNO,
    /** The operation failed because a graph contained a cycle (no value) */
    HDAG_RC_TYPE_GRAPH_CYCLE,
    /** The number of known return code types (not a type itself) */
    HDAG_RC_TYPE_NUM
};

/**
 * Extract the type from a return code without validation.
 *
 * @param rc    The return code to extract the type from.
 *
 * @return The extracted type (as is, not valid).
 */
static inline enum hdag_rc_type
hdag_rc_get_type_raw(hdag_rc rc)
{
    return (-rc) / ((hdag_rc)UINT32_MAX + 1);
}

/**
 * Check if a return code type is valid.
 *
 * @param rc_type   The return code type to check.
 *
 * @return True if the type is valid, false otherwise.
 */
static inline bool
hdag_rc_type_is_valid(enum hdag_rc_type rc_type)
{
    return rc_type >= 0 && rc_type < HDAG_RC_TYPE_NUM;
}

/**
 * Validate a return code type.
 *
 * @param rc_type   The return code type validate.
 *
 * @return The validated return code type.
 */
static inline enum hdag_rc_type
hdag_rc_type_validate(enum hdag_rc_type rc_type)
{
    assert(hdag_rc_type_is_valid(rc_type));
    return rc_type;
}

/**
 * Check if a return code is valid.
 *
 * @param rc    The return code to check.
 *
 * @return True if the code is valid, false otherwise.
 */
static inline bool
hdag_rc_is_valid(hdag_rc rc)
{
    return rc <= 0 && hdag_rc_type_is_valid(hdag_rc_get_type_raw(rc));
}

/**
 * Validate a return code.
 *
 * @param rc    The return code to validate.
 *
 * @return The validated return code.
 */
static inline hdag_rc
hdag_rc_validate(hdag_rc rc)
{
    assert(hdag_rc_is_valid(rc));
    return rc;
}

/**
 * Extract the type from a return code.
 *
 * @param rc    The return code to extract the type from.
 *
 * @return The extracted type (validated).
 */
static inline enum hdag_rc_type
hdag_rc_get_type(hdag_rc rc)
{
    assert(hdag_rc_is_valid(rc));
    return hdag_rc_type_validate(hdag_rc_get_type_raw(rc));
}

/**
 * Extract the value from a return code.
 *
 * @param rc    The return code to extract the value from.
 *
 * @return The extracted value.
 */
static inline int32_t
hdag_rc_get_value(hdag_rc rc)
{
    return (uint32_t)((-rc) % ((hdag_rc)UINT32_MAX + 1));
}

/** Create a return code from a type and a value */
#define HDAG_RC(_type, _value) (-( \
    (hdag_rc)(int32_t)_type * ((hdag_rc)UINT32_MAX + 1) +   \
    (hdag_rc)(uint32_t)(int32_t)(_value)                    \
))

/** The success return code (zero) */
#define HDAG_RC_OK HDAG_RC(HDAG_RC_TYPE_OK, 0)

/** Create a return code from a errno value */
#define HDAG_RC_ERRNO_ARG(_errno) HDAG_RC(HDAG_RC_TYPE_ERRNO, _errno)

/** Create a return code from the current errno value */
#define HDAG_RC_ERRNO HDAG_RC_ERRNO_ARG(errno)

/** The bundle loop error return code */
#define HDAG_RC_GRAPH_CYCLE HDAG_RC(HDAG_RC_TYPE_GRAPH_CYCLE, 0)

/** The invalid return code */
#define HDAG_RC_INVALID INT64_MIN

/**
 * Replace the invalid return code with the current errno return code.
 * Return everything else unchanged.
 */
#define HDAG_RC_ERRNO_IF_INVALID(_rc) \
    ((_rc) == HDAG_RC_INVALID ? HDAG_RC_ERRNO : (_rc))


/**
 * Return a return code as is, but if it's an error also store it at the
 * specified location.
 *
 * @param perror_rc Location for the return code, if it's an error.
 * @param rc        The return code to try.
 *
 * @return The "rc" value.
 */
static inline hdag_rc hdag_rc_catch(hdag_rc *perror_rc, hdag_rc rc)
{
    assert(hdag_rc_is_valid(rc));
    if (rc != HDAG_RC_OK) {
        *perror_rc = rc;
    }
    return rc;
}

/**
 * Check that the specified return code is HDAG_RC_OK, and if not store it in
 * variable called "rc", and go to label "cleanup".
 */
#define HDAG_RC_TRY(_rc) \
    do {                                \
        if (hdag_rc_catch(&rc, _rc)) {  \
            goto cleanup;               \
        }                               \
    } while (0)

/**
 * Create a return code from a type and a value.
 *
 * @param type  The type of the return code being created.
 * @param value The value of the return code being created.
 *
 * @return The created return code.
 */
static inline hdag_rc
hdag_rc_create(enum hdag_rc_type type, int32_t value)
{
    assert(hdag_rc_type_is_valid(type));
    return hdag_rc_validate(HDAG_RC(type, value));
}

/**
 * Return a string describing the error encoded in a return code, possibly
 * outputting it to the provided buffer.
 *
 * @param rc    The return code to describe.
 * @param buf   The buffer to write the (null-terminated) string to.
 *              Can be NULL, if size is zero.
 * @param size  The size of the buffer, bytes. A longer description will be
 *              truncated to fit the buffer.
 *
 * @return The description of the error code: either the provided buffer,
 *         or a constant string.
 */
extern const char *hdag_rc_strerror_r(hdag_rc rc, char *buf, size_t size);

/**
 * Return a string describing the error encoded in a return code, possibly
 * serving it via an internal static buffer.
 *
 * @param rc    The return code to describe.
 *
 * @return The description of the error code.
 */
extern const char *hdag_rc_strerror(hdag_rc rc);

#endif /* _HDAG_RC_H */
