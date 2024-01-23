/**
 * @file dlist.h
 * @author skull (skull.gu@gmail.com)
 * @brief
 * @version 0.1
 * @date 2023-06-17
 *
 * @copyright Copyright (c) 2023 skull
 *
 */
#pragma once
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

typedef struct _dlist_node {
    struct _dlist_node *p_next;
    struct _dlist_node *p_prev;
} dlist_node_t;

// 遍历链表时的处理函数
typedef int (*dlist_node_process_t)(void **p_arg, dlist_node_t **p_node);

int dlist_init(dlist_node_t *p_node);
int dlist_add(dlist_node_t *p_pos, dlist_node_t *p_node);
int dlist_add_head(dlist_node_t *p_head, dlist_node_t *p_node);
int dlist_add_tail(dlist_node_t *p_head, dlist_node_t *p_node);
int dlist_del(dlist_node_t *p_node);

dlist_node_t *dlist_prev_get(dlist_node_t *p_pos);
dlist_node_t *dlist_next_get(dlist_node_t *p_pos);
int dlist_foreach(dlist_node_t *p_head, dlist_node_process_t pfn_node_process, void *p_arg);
int dlist_node_count_get(dlist_node_t *p_head);