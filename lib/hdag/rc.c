/*
 * Hash DAG bundle - universal return code
 */

#include <hdag/rc.h>
#include <stdio.h>
#include <string.h>

const char *
hdag_rc_strerror_r(hdag_rc rc, char *buf, size_t size)
{
    int32_t value;

    assert(hdag_rc_is_valid(rc));
    assert(buf != NULL || size == 0);

    switch (hdag_rc_get_type_raw(rc)) {
    case HDAG_RC_TYPE_OK:
        return "Success";
    case HDAG_RC_TYPE_ERRNO:
        value = hdag_rc_get_value(rc);
        snprintf(buf, size, "ERRNO: %s: %s",
                 strerrorname_np(value),
                 strerrordesc_np(value));
        return buf;
    case HDAG_RC_TYPE_GRAPH_CYCLE:
        return "Graph contains a cycle";
    default:
        snprintf(buf, size, "INVALID RC: 0x%08lx", rc);
        return buf;
    };
}

static char HDAG_RC_STRERROR_BUF[1024];

const char *
hdag_rc_strerror(hdag_rc rc)
{
    assert(hdag_rc_is_valid(rc));
    return hdag_rc_strerror_r(
        rc,
        HDAG_RC_STRERROR_BUF,
        sizeof(HDAG_RC_STRERROR_BUF)
    );
}
