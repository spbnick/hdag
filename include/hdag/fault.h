/*
 * Hash DAG bundle - a fault (failure cause/type)
 */

#ifndef _HDAG_FAULT_H
#define _HDAG_FAULT_H

#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <stdbool.h>

/** A fault (failure type/cause) */
enum hdag_fault {
    /** No fault (success) */
    HDAG_FAULT_NONE = 0,
    /** The operation failed due to a libc/system error, the value is errno */
    HDAG_FAULT_ERRNO,
    /** The operation failed because a graph contained a cycle (no value) */
    HDAG_FAULT_GRAPH_CYCLE,
    /** The operation failed because conflicting node data was encountered */
    HDAG_FAULT_NODE_CONFLICT,
    /** The operation failed because a duplicate node has been detected */
    HDAG_FAULT_NODE_DUPLICATE,
    /** The operation failed because a duplicate edge has been detected */
    HDAG_FAULT_EDGE_DUPLICATE,
    /** The input (file) had invalid format (no value) */
    HDAG_FAULT_INVALID_FORMAT,
    /** The number of known faults (not a fault itself) */
    HDAG_FAULT_NUM
};

/**
 * Check if a fault is valid.
 *
 * @param fault   The fault to check.
 *
 * @return True if the type is valid, false otherwise.
 */
static inline bool
hdag_fault_is_valid(enum hdag_fault fault)
{
    return fault >= 0 && fault < HDAG_FAULT_NUM;
}

/**
 * Validate a fault.
 *
 * @param fault   The fault validate.
 *
 * @return The validated fault.
 */
static inline enum hdag_fault
hdag_fault_validate(enum hdag_fault fault)
{
    assert(hdag_fault_is_valid(fault));
    return fault;
}

#endif /* _HDAG_FAULT_H */
