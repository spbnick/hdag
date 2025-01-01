/*
 * Hash DAG bundle - universal return code
 */

#include <hdag/res.h>
#include <stdio.h>
#include <string.h>

const char *
hdag_res_str_r(hdag_res res, char *buf, size_t size)
{
    int32_t code;

    assert(hdag_res_is_valid(res));
    assert(buf != NULL || size == 0);

    if (res >= 0) {
        snprintf(buf, size, "Success: %ld", res);
        return buf;
    }

    switch (hdag_res_get_fault_raw(res)) {
    case HDAG_FAULT_ERRNO:
        code = hdag_res_get_code(res);
        snprintf(buf, size, "ERRNO: %s: %s",
                 strerrorname_np(code),
                 strerrordesc_np(code));
        return buf;
    case HDAG_FAULT_GRAPH_CYCLE:
        return "Graph contains a cycle";
    case HDAG_FAULT_NODE_CONFLICT:
        return "A node with matching hash, but different targets detected";
    case HDAG_FAULT_INVALID_FORMAT:
        return "Invalid file format";
    default:
        snprintf(buf, size, "INVALID RESULT: 0x%016lx", res);
        return buf;
    };
}

static char HDAG_RES_STR_BUF[1024];

const char *
hdag_res_str(hdag_res res)
{
    assert(hdag_res_is_valid(res));
    return hdag_res_str_r(res, HDAG_RES_STR_BUF, sizeof(HDAG_RES_STR_BUF));
}
