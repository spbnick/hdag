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
        /** Any combination of invalid and direct index targets */
        (
            (
                hdag_target_is_dir_idx(targets->first) ||
                targets->first == HDAG_TARGET_INVALID
            ) &&
            (
                hdag_target_is_dir_idx(targets->last) ||
                targets->last == HDAG_TARGET_INVALID
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

#endif /* _HDAG_TARGETS_H */
