/**
 * Hash DAG edge
 */

#ifndef _HDAG_EDGE_H
#define _HDAG_EDGE_H

#include <stdint.h>

/** An edge */
struct hdag_edge {
    /** The index of the edge's target node in an outside array */
    uint32_t    node_idx;
};

#endif /* _HDAG_EDGE_H */
