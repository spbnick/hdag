/*
 * Hash DAG miscellaneous definitions.
 */

#ifndef _HDAG_MISC_H
#define _HDAG_MISC_H

#include <stdlib.h>
#include <stdint.h>
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
#define TIMESPEC_SUB(_a, _b) ({ \
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
#define PROFILE_TIME(_action, _statement) \
    do {                                                \
        struct timespec     _before;                    \
        struct timespec     _after;                     \
        struct timespec     _elapsed;                   \
        fprintf(stderr, _action "...");                 \
        clock_gettime(CLOCK_MONOTONIC, &_before);       \
        _statement;                                     \
        clock_gettime(CLOCK_MONOTONIC, &_after);        \
        _elapsed = TIMESPEC_SUB(_after, _before);       \
        fprintf(stderr, "done in %ld.%09lds\n",         \
                _elapsed.tv_sec, _elapsed.tv_nsec);     \
    } while (0)

#endif /* _HDAG_MISC_H */
