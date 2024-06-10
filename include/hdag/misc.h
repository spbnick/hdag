/*
 * Hash DAG miscellaneous definitions.
 */

#ifndef _HDAG_MISC_H
#define _HDAG_MISC_H

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

#endif /* _HDAG_MISC_H */
