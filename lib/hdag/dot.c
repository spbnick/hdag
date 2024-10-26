/*
 * Hash DAG - DOT output
 */

#include <hdag/dot.h>
#include <hdag/misc.h>
#include <cgraph.h>

/**
 * Create the destination agnode and the agedge for a particular source node
 * and its direct-index target, unless the target is absent.
 *
 * @param bundle        The bundle the nodes and targets belong to.
 * @param src_node      The source node.
 * @param target        The target.
 * @param dst_hash_buf  The buffer for formatting the destination node hash
 *                      in. Must be big enough for the hash length and
 *                      terminating zero.
 * @param agraph        The output agraph.
 * @param src_agnode    The output source node.
 *
 * @return A void universal result.
 */
[[nodiscard]]
static hdag_res
hdag_dot_write_bundle_dir_target(const struct hdag_bundle *bundle,
                                 const struct hdag_node *src_node,
                                 hdag_target target,
                                 char *dst_hash_buf,
                                 Agraph_t *agraph,
                                 Agnode_t *src_agnode)
{
    /* Destination node */
    const struct hdag_node *dst_node;
    /* Output graph destination node */
    Agnode_t               *dst_agnode;

    assert(hdag_bundle_is_valid(bundle));
    assert(hdag_node_is_valid(src_node));
    assert(hdag_target_is_dir_idx(target) || target == HDAG_TARGET_ABSENT);
    assert(src_node->targets.first == target ||
           src_node->targets.last == target);
    assert(dst_hash_buf != NULL);
    assert(agraph != NULL);
    assert(src_agnode != NULL);

    (void)src_node;

    if (target == HDAG_TARGET_ABSENT) {
        return HDAG_RES_OK;
    }

    /* Fetch destination node (we won't change it) */
    dst_node = HDAG_DARR_ELEMENT_UNSIZED(&bundle->nodes,
                                         struct hdag_node,
                                         hdag_target_to_dir_idx(target));

    /* Create/fetch destination agnode */
    hdag_bytes_to_hex(dst_hash_buf, dst_node->hash, bundle->hash_len);
    dst_agnode = agnode(agraph, dst_hash_buf, true);
    if (dst_agnode == NULL) {
        return HDAG_RES_ERRNO;
    }
    /* Create/fetch the edge */
    if (agedge(agraph, src_agnode, dst_agnode, "", true) == NULL) {
        return HDAG_RES_ERRNO;
    }

    return HDAG_RES_OK;
}

/**
 * Create the destination agnode's and agedge's for a particular source node
 * and all its indirect-index targets.
 *
 * @param bundle        The bundle the nodes and targets belong to.
 * @param src_node      The source node.
 * @param dst_hash_buf  The buffer for formatting the destination node hash
 *                      in. Must be big enough for the hash length and
 *                      terminating zero.
 * @param agraph        The output agraph.
 * @param src_agnode    The output source node.
 *
 * @return A void universal result.
 */
[[nodiscard]]
static hdag_res
hdag_dot_write_bundle_ind_targets(const struct hdag_bundle *bundle,
                                  const struct hdag_node *src_node,
                                  char *dst_hash_buf,
                                  Agraph_t *agraph,
                                  Agnode_t *src_agnode)
{
    /* Target index */
    size_t                  idx;
    /* Outgoing edge */
    const struct hdag_edge *edge;
    /* Destination node */
    const struct hdag_node *dst_node;
    /* Destination hash */
    const uint8_t          *dst_hash;
    /* Output graph destination node */
    Agnode_t               *dst_agnode;

    assert(hdag_bundle_is_valid(bundle));
    assert(hdag_node_is_valid(src_node));
    assert(hdag_targets_are_indirect(&src_node->targets));
    assert(dst_hash_buf != NULL);
    assert(agraph != NULL);
    assert(src_agnode != NULL);

    /* For each indirect index */
    for (idx = hdag_node_get_first_ind_idx(src_node);
         idx <= hdag_node_get_last_ind_idx(src_node);
         idx++) {
        /* If indirect indices are pointing to the "extra edges" */
        if (hdag_darr_occupied_slots(&bundle->extra_edges) != 0) {
            edge = HDAG_DARR_ELEMENT(
                &bundle->extra_edges, struct hdag_edge, idx
            );
            dst_node = HDAG_DARR_ELEMENT_UNSIZED(
                &bundle->nodes, struct hdag_node, edge->node_idx
            );
            dst_hash = dst_node->hash;
        /* Else, indirect indices are pointing to the "target hashes" */
        } else {
            dst_hash = HDAG_DARR_ELEMENT_UNSIZED(
                &bundle->target_hashes, uint8_t, idx
            );
        }
        /* Create/fetch destination agnode */
        hdag_bytes_to_hex(dst_hash_buf, dst_hash, bundle->hash_len);
        dst_agnode = agnode(agraph, dst_hash_buf, true);
        if (dst_agnode == NULL) {
            return HDAG_RES_ERRNO;
        }
        /* Create/fetch the edge */
        if (agedge(agraph, src_agnode, dst_agnode, "", true) == NULL) {
            return HDAG_RES_ERRNO;
        }
    }

    return HDAG_RES_OK;
}

hdag_res
hdag_dot_write_bundle(const struct hdag_bundle *bundle,
                      const char *name, FILE *stream)
{
    hdag_res            res = HDAG_RES_INVALID;
    /* The output graph */
    Agraph_t           *agraph = NULL;
    /* Output graph source node */
    Agnode_t           *src_agnode;
    /* The "style" attribute symbol */
    Agsym_t            *style_sym;
    /* The index of the currently-traversed node */
    ssize_t             idx;
    /* Currently traversed source node */
    const struct hdag_node *src_node;
    /* The buffer for source node hash hex representation */
    char               *src_hash_buf = NULL;
    /* The buffer for destination node hash hex representation */
    char               *dst_hash_buf = NULL;

    assert(hdag_bundle_is_valid(bundle));
    assert(name != NULL);
    assert(stream != NULL);

    /* Allocate buffers for hash hex representations */
    src_hash_buf = malloc(bundle->hash_len * 2 + 1);
    dst_hash_buf = malloc(bundle->hash_len * 2 + 1);
    if (src_hash_buf == NULL || dst_hash_buf == NULL) {
        goto cleanup;
    }

    /* Create the graph (they won't change the "name") */
    agraph = agopen((char *)name, Agdirected, NULL);
    if (agraph == NULL) {
        goto cleanup;
    }

    /* Define the "style" node attribute */
    style_sym = agattr(agraph, AGNODE, "style", "solid");
    if (style_sym == NULL) {
        goto cleanup;
    }

    /* Set the default "shape" node attribute */
    if (agattr(agraph, AGNODE, "shape", "box") == NULL) {
        goto cleanup;
    }

    /* For each node */
    HDAG_DARR_ITER_FORWARD(&bundle->nodes, idx, src_node, (void)0, (void)0) {
        /* Create or fetch the node */
        hdag_bytes_to_hex(src_hash_buf,
                          src_node->hash, bundle->hash_len);
        src_agnode = agnode(agraph, src_hash_buf, true);
        if (src_agnode == NULL) {
            goto cleanup;
        }
        /* If the node has no targets */
        if (hdag_targets_are_absent(&src_node->targets)) {
            continue;
        }
        /* If the node's targets are unknown */
        if (hdag_targets_are_unknown(&src_node->targets)) {
            agxset(src_agnode, style_sym, "dashed");
        /* Else, if the node targets are node indices */
        } else if (hdag_targets_are_direct(&src_node->targets)) {
            /* Output first target and edge, if any */
            HDAG_RES_TRY(hdag_dot_write_bundle_dir_target(
                            bundle, src_node, src_node->targets.first,
                            dst_hash_buf, agraph, src_agnode));
            /* Output last (second) target and edge, if any */
            HDAG_RES_TRY(hdag_dot_write_bundle_dir_target(
                            bundle, src_node, src_node->targets.last,
                            dst_hash_buf, agraph, src_agnode));
        /* Else, node targets are indirect target hash/extra edge indices */
        } else {
            /* Output indirect target nodes and edges */
            HDAG_RES_TRY(hdag_dot_write_bundle_ind_targets(
                            bundle, src_node, dst_hash_buf,
                            agraph, src_agnode));
        }
    }

    /* Write the graph */
    if (agwrite(agraph, stream) != 0) {
        goto cleanup;
    }

    /* Report success */
    res = HDAG_RES_OK;

cleanup:
    if (agraph != NULL) {
        agclose(agraph);
    }
    free(dst_hash_buf);
    free(src_hash_buf);
    return HDAG_RES_ERRNO_IF_INVALID(res);
}
