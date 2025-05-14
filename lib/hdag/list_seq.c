/**
 * Hash DAG - pointer list sequence
 */

#include <hdag/list_seq.h>
#include <hdag/misc.h>
#include <sys/param.h>
#include <string.h>

hdag_res
hdag_list_seq_next(struct hdag_seq *base_seq, void **pitem)
{
    struct hdag_list_seq *seq = HDAG_LIST_SEQ_FROM_SEQ(base_seq);
    if (seq->idx < seq->len) {
        *pitem = seq->list[seq->idx++];
        return 1;
    }
    return HDAG_RES_OK;
}

hdag_res
hdag_list_seq_reset(struct hdag_seq *base_seq)
{
    struct hdag_list_seq *seq = HDAG_LIST_SEQ_FROM_SEQ(base_seq);
    seq->idx = 0;
    return HDAG_RES_OK;
}
