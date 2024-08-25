/*
 * Hash DAG node targets structure
 */

#ifndef _HDAG_TARGETS_H
#define _HDAG_TARGETS_H

#include <hdag/target.h>
#include <assert.h>

/** A reference to targets of a node */
struct hdag_targets {
    /** The first target */
    hdag_target    first;
    /** The last target */
    hdag_target    last;
};

/** An initializer for unknown targets */
#define HDAG_TARGETS_UNKNOWN (struct hdag_targets){ \
    HDAG_TARGET_UNKNOWN, HDAG_TARGET_UNKNOWN        \
}

/** An initializer for absent targets */
#define HDAG_TARGETS_ABSENT (struct hdag_targets){ \
    HDAG_TARGET_ABSENT, HDAG_TARGET_ABSENT         \
}

/** An initializer for indirect targets (with defined evaluation order) */
#define HDAG_TARGETS_INDIRECT(_first, _last) ( \
    *hdag_targets_validate(&(struct hdag_targets){  \
        .first = hdag_target_from_ind_idx(_first),  \
        .last = hdag_target_from_ind_idx(_last),    \
    })                                              \
)

/** An initializer for one direct target */
#define HDAG_TARGETS_DIRECT_ONE(_target) ( \
    *hdag_targets_validate(&(struct hdag_targets){  \
        .first = hdag_target_from_dir_idx(_target), \
        .last = HDAG_TARGET_ABSENT                  \
    })                                              \
)

/** An initializer for two direct targets (with defined evaluation order) */
#define HDAG_TARGETS_DIRECT_TWO(_first, _second) ( \
    *hdag_targets_validate(&(struct hdag_targets){  \
        .first = hdag_target_from_dir_idx(_first),  \
        .last = hdag_target_from_dir_idx(_second),  \
    })                                              \
)

/**
 * Check if targets are valid.
 *
 * @param targets   The targets to check.
 *
 * @return True if the targets are valid, false otherwise.
 */
static inline bool
hdag_targets_are_valid(const struct hdag_targets *targets)
{
    return targets != NULL && (
        /** Node's targets are unknown */
        (
            targets->first == HDAG_TARGET_UNKNOWN &&
            targets->last == HDAG_TARGET_UNKNOWN
        ) ||
        /** Any combination of absent and direct index targets */
        (
            (
                hdag_target_is_dir_idx(targets->first) ||
                targets->first == HDAG_TARGET_ABSENT
            ) &&
            (
                hdag_target_is_dir_idx(targets->last) ||
                targets->last == HDAG_TARGET_ABSENT
            )
        ) ||
        (
            /** Indirect index span */
            hdag_target_is_ind_idx(targets->first) &&
            hdag_target_is_ind_idx(targets->last) &&
            targets->first <= targets->last
        )
    );
}

/**
 * Validate targets.
 *
 * @param targets   The targets to validate.
 *
 * @return The validated targets.
 */
static inline const struct hdag_targets*
hdag_targets_validate(const struct hdag_targets *targets)
{
    assert(hdag_targets_are_valid(targets));
    return targets;
}

/**
 * Create targets containing one direct target.
 *
 * @param idx   The target's direct index.
 *
 * @return The created direct targets.
 */
static inline struct hdag_targets
hdag_targets_direct_one(size_t idx)
{
    assert(hdag_target_idx_is_valid(idx));
    return *hdag_targets_validate(&(struct hdag_targets){
        .first = hdag_target_from_dir_idx(idx),
        .last = HDAG_TARGET_ABSENT,
    });
}

/**
 * Create two direct targets.
 *
 * @param first The first target's direct index.
 * @param last  The last target's direct index.
 *
 * @return The created direct targets.
 */
static inline struct hdag_targets
hdag_targets_direct_two(size_t first, size_t last)
{
    assert(hdag_target_idx_is_valid(first));
    assert(hdag_target_idx_is_valid(last));
    assert(first <= last);
    return *hdag_targets_validate(&(struct hdag_targets){
        .first = hdag_target_from_dir_idx(first),
        .last = hdag_target_from_dir_idx(last),
    });
}

/**
 * Create indirect targets.
 *
 * @param first The first target's indirect index.
 * @param last  The last target's indirect index.
 *
 * @return The created indirect targets.
 */
static inline struct hdag_targets
hdag_targets_indirect(size_t first, size_t last)
{
    assert(hdag_target_idx_is_valid(first));
    assert(hdag_target_idx_is_valid(last));
    assert(first <= last);
    return *hdag_targets_validate(&(struct hdag_targets){
        .first = hdag_target_from_ind_idx(first),
        .last = hdag_target_from_ind_idx(last),
    });
}

/**
 * Check if targets are unknown.
 *
 * @param targets   The targets to check.
 *
 * @return True if the targets are unknown, false otherwise.
 */
static inline bool
hdag_targets_are_unknown(const struct hdag_targets *targets)
{
    assert(hdag_targets_are_valid(targets));
    return targets->first == HDAG_TARGET_UNKNOWN;
}

/**
 * Check if targets are known.
 *
 * @param targets   The targets to check.
 *
 * @return True if the targets are known, false otherwise.
 */
static inline bool
hdag_targets_are_known(const struct hdag_targets *targets)
{
    assert(hdag_targets_are_valid(targets));
    return targets->first != HDAG_TARGET_UNKNOWN;
}

/**
 * Check if targets are indirect.
 *
 * @param targets   The targets to check.
 *
 * @return True if the targets are indirect, false otherwise.
 */
static inline bool
hdag_targets_are_indirect(const struct hdag_targets *targets)
{
    assert(hdag_targets_are_valid(targets));
    return hdag_target_is_ind_idx(targets->first);
}

/**
 * Check if any targets are direct.
 *
 * @param targets   The targets to check.
 *
 * @return True if any of the targets are direct, false otherwise.
 */
static inline bool
hdag_targets_are_direct(const struct hdag_targets *targets)
{
    assert(hdag_targets_are_valid(targets));
    return hdag_target_is_dir_idx(targets->first) ||
        hdag_target_is_dir_idx(targets->last);
}

/**
 * Check if all targets are absent (HDAG_TARGET_ABSENT).
 *
 * @param targets   The targets to check.
 *
 * @return True if all targets are absent.
 */
static inline bool
hdag_targets_are_absent(const struct hdag_targets *targets)
{
    assert(hdag_targets_are_valid(targets));
    return targets->first == HDAG_TARGET_ABSENT &&
        targets->last == HDAG_TARGET_ABSENT;
}

#endif /* _HDAG_TARGETS_H */
