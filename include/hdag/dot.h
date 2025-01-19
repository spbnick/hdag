/**
 * Hash DAG - DOT output
 */

#ifndef _HDAG_DOT_H
#define _HDAG_DOT_H

#include <hdag/bundle.h>

/**
 * Write a Graphviz DOT representation of the graph in a bundle to a file.
 *
 * @param bundle    The bundle to write the representation of.
 * @param name      The name to give the output digraph.
 * @param stream    The FILE stream to write the representation to.
 *
 * @return A void universal result.
 */
[[nodiscard]]
extern hdag_res hdag_dot_write_bundle(const struct hdag_bundle *bundle,
                                      const char *name, FILE *stream);

#endif /* _HDAG_DOT_H */
