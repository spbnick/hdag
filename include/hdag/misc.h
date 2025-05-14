/**
 * Hash DAG miscellaneous definitions.
 */

#ifndef _HDAG_MISC_H
#define _HDAG_MISC_H

#include <stdlib.h>
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <time.h>

/** Get the 11th argument passed */
#define HDAG_MACRO_11TH_ARG( \
    _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, ARG, ...   \
) ARG

#define HDAG_EXPAND_WITH_1_ARG_0_TIMES(_macro, ...)
#define HDAG_EXPAND_WITH_1_ARG_1_TIMES(_macro, arg, ...)   _macro(arg)
#define HDAG_EXPAND_WITH_1_ARG_2_TIMES(_macro, arg, ...) \
    _macro(arg) HDAG_EXPAND_WITH_1_ARG_1_TIMES(_macro, __VA_ARGS__)
#define HDAG_EXPAND_WITH_1_ARG_3_TIMES(_macro, arg, ...) \
    _macro(arg) HDAG_EXPAND_WITH_1_ARG_2_TIMES(_macro, __VA_ARGS__)
#define HDAG_EXPAND_WITH_1_ARG_4_TIMES(_macro, arg, ...) \
    _macro(arg) HDAG_EXPAND_WITH_1_ARG_3_TIMES(_macro, __VA_ARGS__)
#define HDAG_EXPAND_WITH_1_ARG_5_TIMES(_macro, arg, ...) \
    _macro(arg) HDAG_EXPAND_WITH_1_ARG_4_TIMES(_macro, __VA_ARGS__)
#define HDAG_EXPAND_WITH_1_ARG_6_TIMES(_macro, arg, ...) \
    _macro(arg) HDAG_EXPAND_WITH_1_ARG_5_TIMES(_macro, __VA_ARGS__)
#define HDAG_EXPAND_WITH_1_ARG_7_TIMES(_macro, arg, ...) \
    _macro(arg) HDAG_EXPAND_WITH_1_ARG_6_TIMES(_macro, __VA_ARGS__)
#define HDAG_EXPAND_WITH_1_ARG_8_TIMES(_macro, arg, ...) \
    _macro(arg) HDAG_EXPAND_WITH_1_ARG_7_TIMES(_macro, __VA_ARGS__)
#define HDAG_EXPAND_WITH_1_ARG_9_TIMES(_macro, arg, ...) \
    _macro(arg) HDAG_EXPAND_WITH_1_ARG_8_TIMES(_macro, __VA_ARGS__)
#define HDAG_EXPAND_WITH_1_ARG_10_TIMES(_macro, arg, ...) \
    _macro(arg) HDAG_EXPAND_WITH_1_ARG_9_TIMES(_macro, __VA_ARGS__)

#define HDAG_EXPAND_WITH_2_ARGS_0_TIMES(_macro, ...)
#define HDAG_EXPAND_WITH_2_ARGS_1_TIMES(_macro, arg1, arg2, ...) \
    _macro(arg1, arg2)
#define HDAG_EXPAND_WITH_2_ARGS_2_TIMES(_macro, arg1, arg2, ...) \
    _macro(arg1, arg2)                                              \
    HDAG_EXPAND_WITH_2_ARGS_1_TIMES(_macro, arg1, __VA_ARGS__)
#define HDAG_EXPAND_WITH_2_ARGS_3_TIMES(_macro, arg1, arg2, ...) \
    _macro(arg1, arg2)                                              \
    HDAG_EXPAND_WITH_2_ARGS_2_TIMES(_macro, arg1, __VA_ARGS__)
#define HDAG_EXPAND_WITH_2_ARGS_4_TIMES(_macro, arg1, arg2, ...) \
    _macro(arg1, arg2)                                              \
    HDAG_EXPAND_WITH_2_ARGS_3_TIMES(_macro, arg1, __VA_ARGS__)
#define HDAG_EXPAND_WITH_2_ARGS_5_TIMES(_macro, arg1, arg2, ...) \
    _macro(arg1, arg2)                                              \
    HDAG_EXPAND_WITH_2_ARGS_4_TIMES(_macro, arg1, __VA_ARGS__)
#define HDAG_EXPAND_WITH_2_ARGS_6_TIMES(_macro, arg1, arg2, ...) \
    _macro(arg1, arg2)                                              \
    HDAG_EXPAND_WITH_2_ARGS_5_TIMES(_macro, arg1, __VA_ARGS__)
#define HDAG_EXPAND_WITH_2_ARGS_7_TIMES(_macro, arg1, arg2, ...) \
    _macro(arg1, arg2)                                              \
    HDAG_EXPAND_WITH_2_ARGS_6_TIMES(_macro, arg1, __VA_ARGS__)
#define HDAG_EXPAND_WITH_2_ARGS_8_TIMES(_macro, arg1, arg2, ...) \
    _macro(arg1, arg2)                                              \
    HDAG_EXPAND_WITH_2_ARGS_7_TIMES(_macro, arg1, __VA_ARGS__)
#define HDAG_EXPAND_WITH_2_ARGS_9_TIMES(_macro, arg1, arg2, ...) \
    _macro(arg1, arg2)                                              \
    HDAG_EXPAND_WITH_2_ARGS_8_TIMES(_macro, arg1, __VA_ARGS__)
#define HDAG_EXPAND_WITH_2_ARGS_10_TIMES(_macro, arg1, arg2, ...) \
    _macro(arg1, arg2)                                              \
    HDAG_EXPAND_WITH_2_ARGS_9_TIMES(_macro, arg1, __VA_ARGS__)

/**
 * Expand to the size of the specified structure's member
 *
 * @param _struct   The name of the structure (without "struct").
 * @param _member   The name of the structure member.
 */
#define HDAG_STRUCT_MEMBER_SIZEOF(_struct, _member) \
    sizeof(((struct _struct*)0)->_member)

/**
 * Expand to the size of the specified structure's member,
 * followed by the plus operator.
 *
 * @param _struct   The name of the structure (without "struct").
 * @param _member   The name of the structure member.
 */
#define HDAG_STRUCT_MEMBER_SIZEOF_PLUS(_struct, _member) \
    HDAG_STRUCT_MEMBER_SIZEOF(_struct, _member) +

/**
 * Expand a macro for each argument (up to 10)
 *
 * @param _macro        The name of the macro to expand.
 * @param ...           The arguments to supply to each separate expansion.
 *                      Number of invocation is the number of these arguments.
 */
#define HDAG_EXPAND_FOR_EACH(_macro, ...) \
    HDAG_MACRO_11TH_ARG(                    \
        _, ##__VA_ARGS__,                   \
        HDAG_EXPAND_WITH_1_ARG_9_TIMES,     \
        HDAG_EXPAND_WITH_1_ARG_8_TIMES,     \
        HDAG_EXPAND_WITH_1_ARG_7_TIMES,     \
        HDAG_EXPAND_WITH_1_ARG_6_TIMES,     \
        HDAG_EXPAND_WITH_1_ARG_5_TIMES,     \
        HDAG_EXPAND_WITH_1_ARG_4_TIMES,     \
        HDAG_EXPAND_WITH_1_ARG_3_TIMES,     \
        HDAG_EXPAND_WITH_1_ARG_2_TIMES,     \
        HDAG_EXPAND_WITH_1_ARG_1_TIMES,     \
        HDAG_EXPAND_WITH_1_ARG_0_TIMES      \
    )(_macro, ##__VA_ARGS__)

/**
 * Expand a macro for each argument (up to 10), with a shared first argument.
 *
 * @param _macro        The name of the macro to expand.
 * @param _shared_arg   The first argument to supply to each macro expansion.
 * @param ...           The arguments to supply second to each separate
 *                      expansion. Number of invocation is the number of these
 *                      arguments.
 */
#define HDAG_EXPAND_FOR_EACH_WITH(_macro, _shared_arg, ...) \
    HDAG_MACRO_11TH_ARG(                                    \
        _, ##__VA_ARGS__,                                   \
        HDAG_EXPAND_WITH_2_ARGS_9_TIMES,                    \
        HDAG_EXPAND_WITH_2_ARGS_8_TIMES,                    \
        HDAG_EXPAND_WITH_2_ARGS_7_TIMES,                    \
        HDAG_EXPAND_WITH_2_ARGS_6_TIMES,                    \
        HDAG_EXPAND_WITH_2_ARGS_5_TIMES,                    \
        HDAG_EXPAND_WITH_2_ARGS_4_TIMES,                    \
        HDAG_EXPAND_WITH_2_ARGS_3_TIMES,                    \
        HDAG_EXPAND_WITH_2_ARGS_2_TIMES,                    \
        HDAG_EXPAND_WITH_2_ARGS_1_TIMES,                    \
        HDAG_EXPAND_WITH_2_ARGS_0_TIMES,                    \
    )(_macro, _shared_arg, ##__VA_ARGS__)

/**
 * Expand to a compile-time expression evaluating to true, if the specified
 * structure size is equal to the sum of the sizes of its (specified) members.
 *
 * @param _struct   The name of the structure (without "struct").
 * @param ...       The structure member names.
 */
#define HDAG_STRUCT_MEMBERS_PACKED(_struct, ...) \
    sizeof(struct _struct) == (                                     \
        HDAG_EXPAND_FOR_EACH_WITH(                                  \
            HDAG_STRUCT_MEMBER_SIZEOF_PLUS, _struct, ##__VA_ARGS__  \
        ) 0                                                         \
    )

/**
 * Assert statically, that the specified structure size is equal to the sum of
 * the sizes of its (specified) members.
 *
 * @param _struct   The name of the structure (without "struct").
 * @param ...       The structure member names.
 */
#define HDAG_ASSERT_STRUCT_MEMBERS_PACKED(_struct, ...) \
    static_assert(                                              \
        HDAG_STRUCT_MEMBERS_PACKED(_struct, ##__VA_ARGS__),     \
        "The " #_struct " structure is not packed"              \
    )

/**
 * Convert bytes to a hexadecimal string.
 *
 * @param hex_ptr   The buffer to write hexadecimal string to.
 *                  Must be at least bytes_num * 2 + 1 long.
 *                  Will be zero-terminated.
 * @param bytes_ptr The pointer to the bytes to convert.
 * @param bytes_num The number of bytes to convert.
 *
 * @return The "buf_ptr".
 */
extern char *hdag_bytes_to_hex(char *hex_ptr,
                               const void *bytes_ptr,
                               size_t bytes_num);

/**
 * Subtract one timespec from another.
 *
 * @param _a    The timespec value to subtract from.
 * @param _b    The timespec value to subtract.
 *
 * @return The result of the subtraction.
 */
#define HDAG_TIMESPEC_SUB(_a, _b) ({ \
    struct timespec _result;                                            \
    _result.tv_sec = (_a).tv_sec - (_b).tv_sec;                         \
    if ((_a).tv_nsec < (_b).tv_nsec) {                                  \
        _result.tv_nsec = 1000000000L + (_a).tv_nsec - (_b).tv_nsec;    \
        _result.tv_sec--;                                               \
    } else {                                                            \
        _result.tv_nsec = (_a).tv_nsec - (_b).tv_nsec;                  \
    }                                                                   \
    _result;                                                            \
})

/**
 * Time the execution of a statement, and print the result to stderr.
 *
 * @param _action       The string with the action gerund.
 * @param _statement    The statement to execute and time.
 */
#define HDAG_PROFILE_TIME(_action, _statement) \
    do {                                                \
        struct timespec     _before;                    \
        struct timespec     _after;                     \
        struct timespec     _elapsed;                   \
        fprintf(stderr, _action "...");                 \
        clock_gettime(CLOCK_MONOTONIC, &_before);       \
        _statement;                                     \
        clock_gettime(CLOCK_MONOTONIC, &_after);        \
        _elapsed = HDAG_TIMESPEC_SUB(_after, _before);  \
        fprintf(stderr, "done in %ld.%09lds\n",         \
                _elapsed.tv_sec, _elapsed.tv_nsec);     \
    } while (0)

/**
 * Get the length of an array (number of elements).
 *
 * @param _arr  The array to get the number of elements for.
 *
 * @return The array length (number of elements).
 */
#define HDAG_ARR_LEN(_arr) (sizeof(_arr) / sizeof((_arr)[0]))

/**
 * Cast a member pointer to the containing type.
 *
 * @param _container_type       The specifier of container type to cast to.
 * @param _member_identifier    The identifier (name) of the container's
 *                              member being cast.
 * @param _member               The pointer to a member to cast.
 *
 * @return The cast pointer. Const, if _member is const.
 */
#define HDAG_CONTAINER_OF(_container_type, _member_identifier, _member) \
    _Generic(                                                           \
        _member,                                                        \
        const typeof(*(_member)) *:                                     \
            (const _container_type *)(                                  \
                (const void *)(_member) -                               \
                offsetof(_container_type, _member_identifier)           \
            ),                                                          \
        default:                                                        \
            (_container_type *)(                                        \
                (void *)(_member) -                                     \
                offsetof(_container_type, _member_identifier)           \
            )                                                           \
    )

/**
 * Compare two size_t values. Can be used with qsort.
 *
 * @param a     Pointer to the first size_t to compare.
 * @param b     Pointer to the second size_t to compare.
 *
 * @return -1, if a < b
 *          0, if a == b
 *          1, if a > b
 */
extern int hdag_size_t_cmp(const void *a, const void *b);

/**
 * Compare two reversed size_t values. Can be used with qsort.
 *
 * @param a     Pointer to the first size_t to compare.
 * @param b     Pointer to the second size_t to compare.
 *
 * @return -1, if a > b
 *          0, if a == b
 *          1, if a < b
 */
extern int hdag_size_t_rcmp(const void *a, const void *b);

/**
 * Generate a 64-bit "hash" from a 64-bit number using SplitMix64.
 * Source: https://prng.di.unimi.it/splitmix64.c (used in Java PRNG).
 *
 * @param x The number to "hash".
 *
 * @return The "hash".
 */
static inline uint64_t hdag_splitmix64_hash(uint64_t x)
{
	x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9;
	x = (x ^ (x >> 27)) * 0x94d049bb133111eb;
	return x ^ (x >> 31);
}

/**
 * The prototype for an abstract value comparison function.
 *
 * @param first     The first value to compare.
 * @param second    The second value to compare.
 * @param data      The function's private data.
 *
 * @return The (non-normalized) bare (standard) comparison result:
 *          < 0, if first < second
 *         == 0, if first == second
 *          > 0, if first > second
 */
typedef int (*hdag_cmp_fn)(const void *first, const void *second, void *data);

/**
 * Check if a comparison result is normal (must be in [-1, 1] range).
 *
 * @param cmp   The comparison result to check.
 *
 * @return True if the comparison result is valid, false otherwise.
 */
static inline bool
hdag_cmp_is_normal(int cmp)
{
    return (unsigned int)(cmp + 1) <= 2;
}

/**
 * Verify that a comparison result is normal (in [-1, 1] range).
 *
 * @param cmp   The comparison result to verify.
 *
 * @return The verified comparison result.
 */
static inline int
hdag_cmp_verify_normal(int cmp)
{
    assert(hdag_cmp_is_normal(cmp));
    return cmp;
}

/**
 * Normalize a comparison result (from any value to [-1, 1] range).
 *
 * @param cmp   The comparison result to normalize.
 *
 * @return The normalized comparison result.
 */
static inline int
hdag_cmp_normalize(int cmp)
{
    return (cmp > 0) - (cmp < 0);
}

/**
 * Compare two abstract values using memcmp.
 *
 * @param first     The first value to compare.
 * @param second    The second value to compare.
 * @param data      The size of each value (uintptr_t).
 *
 * @return Non-normalized comparison result.
 */
extern int hdag_cmp_mem(const void *first, const void *second, void *data);

#endif /* _HDAG_MISC_H */
