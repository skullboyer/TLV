/**
 * @file dlist.c
 * @author skull (skull.gu@gmail.com)
 * @brief bidirectional linked list
 * @version 0.1
 * @date 2023-06-17
 *
 * @copyright Copyright (c) 2023
 *
 * +---------------------------------------------------------------+
 * |                                                               |
 * |           +---------+    +---------+           +---------+    |
 * +-> head -> | p_next1 | -> | p_next2 | -> ... -> | p_nextx | ---+
 * +-- head <- | p_prev1 | <- | p_prev2 | <- ... <- | p_prevx | <--+
 * |           | data1   |    | data2   |           | datax   |    |
 * |           +---------+    +---------+           +---------+    |
 * |                                                               |
 * +---------------------------------------------------------------+
 */
#include "dlist.h"
#include "adapter.h"

#define TAG    "DLIST"

int dlist_init(dlist_node_t *p_head)
{
    CHECK(p_head != NULL, -0xFF);

    p_head->p_next = p_head;
    p_head->p_prev = p_head;
    return 0;
}

int dlist_add(dlist_node_t *p_pos, dlist_node_t *p_node)
{
    CHECK(p_pos != NULL, -0xFF);
    CHECK(p_node != NULL, -0xFF);

    p_node->p_prev = p_pos;
    p_node->p_next = p_pos->p_next;
    p_pos->p_next->p_prev = p_node;
    p_pos->p_next = p_node;
    return 0;
}

int dlist_add_head(dlist_node_t *p_head, dlist_node_t *p_node)
{
    return dlist_add(p_head->p_next, p_node);
}

int dlist_add_tail(dlist_node_t *p_head, dlist_node_t *p_node)
{
    return dlist_add(p_head->p_prev, p_node);
}

int dlist_del(dlist_node_t *p_node)
{
    CHECK(p_node != NULL, -0xFF);

    p_node->p_prev->p_next = p_node->p_next;
    p_node->p_next->p_prev = p_node->p_prev;
    p_node->p_next = NULL;
    p_node->p_prev = NULL;
    return 0;
}

dlist_node_t *dlist_prev_get(dlist_node_t *p_pos)
{
    if (p_pos != NULL) {
        return p_pos->p_prev;
    }

    return NULL;
}

dlist_node_t *dlist_next_get(dlist_node_t *p_pos)
{
    if (p_pos != NULL) {
        return p_pos->p_next;
    }
    return NULL;
}

int dlist_foreach(dlist_node_t *p_head, dlist_node_process_t pfn_node_process, void *p_arg)
{
    CHECK(p_head != NULL, -0xFF);
    CHECK(pfn_node_process != NULL, -0xFF);

    dlist_node_t *p_tmp = dlist_next_get(p_head);
    while (p_tmp != p_head) {
        int ret = pfn_node_process(&p_arg, &p_tmp);
        if (!!ret) {
            return ret;
        }
        p_tmp = dlist_next_get(p_tmp);
    }
    return 0;
}

static int dlist_node_count(void **p_arg, dlist_node_t **p_node)
{
    CHECK(*p_node != NULL, -0xFF);

    (void)(*p_node);
    (*(uint16_t *)(*p_arg))++;
    return 0;
}

int dlist_node_count_get(dlist_node_t *p_head)
{
    CHECK(p_head != NULL, -0xFF);

    uint16_t node_count = 0;
    dlist_foreach(p_head, dlist_node_count, &node_count);
    LOGI("dlist node count: %u", node_count);
    return node_count;
}
