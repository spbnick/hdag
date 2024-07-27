/*
 * Hash DAG node target
 */

#ifndef _HDAG_TARGET_H
#define _HDAG_TARGET_H

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

/** An node's (outgoing edge's) target value */
typedef uint32_t    hdag_target;

/** A node's target is absent (and so is the outgoing edge) */
#define HDAG_TARGET_ABSENT    (hdag_target)0

/** A node's unknown target (outgoing edge's target) */
#define HDAG_TARGET_UNKNOWN    (hdag_target)UINT32_MAX

/** Minimum value for a direct target index */
#define HDAG_TARGET_DIR_IDX_MIN   (HDAG_TARGET_ABSENT + 1)
/** Maximum value for a direct target index */
#define HDAG_TARGET_DIR_IDX_MAX   (hdag_target)INT32_MAX

/** Minimum value for an indirect target index */
#define HDAG_TARGET_IND_IDX_MIN   (HDAG_TARGET_DIR_IDX_MAX + 1)
/** Maximum value for an indirect target index */
#define HDAG_TARGET_IND_IDX_MAX   (HDAG_TARGET_UNKNOWN - 1)

/**
 * Check if a node's target is a direct or indirect index.
 *
 * @param target    The target value to check.
 *
 * @return True if the target is an index, false otherwise.
 */
static inline bool
hdag_target_is_idx(hdag_target target)
{
    return target != HDAG_TARGET_ABSENT &&
        target != HDAG_TARGET_UNKNOWN;
}

/**
 * Check if a node's target is a direct index.
 *
 * @param target    The target value to check.
 *
 * @return True if the target is a direct index, false otherwise.
 */
static inline bool
hdag_target_is_dir_idx(hdag_target target)
{
    return target >= HDAG_TARGET_DIR_IDX_MIN &&
        target <= HDAG_TARGET_DIR_IDX_MAX;
}

/**
 * Check if a node's target is an inderect index.
 *
 * @param target    The target value to check.
 *
 * @return True if the target is an indirect index, false otherwise.
 */
static inline bool
hdag_target_is_ind_idx(hdag_target target)
{
    return target >= HDAG_TARGET_IND_IDX_MIN &&
        target <= HDAG_TARGET_IND_IDX_MAX;
}

/**
 * Extract the direct target index from a target value.
 *
 * @param target    The target value to extract the direct index from.
 *
 * @return The direct index.
 */
static inline size_t
hdag_target_to_dir_idx(hdag_target target)
{
    assert(hdag_target_is_dir_idx(target));
    return target - HDAG_TARGET_DIR_IDX_MIN;
}

/**
 * Produce a target value containing a direct target index.
 *
 * @param dir_idx   The direct target index to store in the target.
 *
 * @return The resulting target value.
 */
static inline hdag_target
hdag_target_from_dir_idx(size_t dir_idx)
{
    hdag_target target = dir_idx + HDAG_TARGET_DIR_IDX_MIN;
    assert(hdag_target_is_dir_idx(target));
    return target;
}

/**
 * Extract the indirect target index from a target value.
 *
 * @param target    The target value to extract indirect index from.
 *
 * @return The indirect index.
 */
static inline size_t
hdag_target_to_ind_idx(hdag_target target)
{
    assert(hdag_target_is_ind_idx(target));
    return target - HDAG_TARGET_IND_IDX_MIN;
}

/**
 * Produce a target value containing an indirect target index.
 *
 * @param ind_idx   The indirect target index to store in the target.
 *
 * @return The resulting target value.
 */
static inline hdag_target
hdag_target_from_ind_idx(size_t ind_idx)
{
    hdag_target target = ind_idx + HDAG_TARGET_IND_IDX_MIN;
    assert(hdag_target_is_ind_idx(target));
    return target;
}

#endif /* _HDAG_TARGET_H */
