/**
 * Hash DAG - size_t iterator properties
 */
#ifndef _HDAG_ITER_PROP_H
#define _HDAG_ITER_PROP_H

#include <stdbool.h>

/**
 * IDs of size_t iterator properties.
 */
enum hdag_iter_prop_id {
    /** No property (invalid ID) */
    HDAG_ITER_PROP_ID_NONE,
    /** The number of size_t property IDs, not an ID itself */
    HDAG_ITER_PROP_ID_NUM
};

/**
 * Check if a size_t iterator property ID is valid.
 *
 * @param id    The ID to check.
 *
 * @return True if the ID is valid, false otherwise.
 */
static inline bool
hdag_iter_prop_id_is_valid(enum hdag_iter_prop_id id)
{
    return id >= 0 && id < HDAG_ITER_PROP_ID_NUM;
}

#endif /* _HDAG_ITER_PROP_H */
