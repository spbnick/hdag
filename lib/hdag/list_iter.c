/**
 * Hash DAG - pointer list iterator
 */

#include <hdag/list_iter.h>
#include <hdag/misc.h>
#include <sys/param.h>
#include <string.h>

hdag_res
hdag_list_iter_next(const struct hdag_iter *iter, void **pitem)
{
    struct hdag_list_iter_data *data =
        hdag_list_iter_data_validate(iter->data);
    if (data->idx < data->len) {
        *pitem = data->list[data->idx++];
        return 1;
    }
    return HDAG_RES_OK;
}
