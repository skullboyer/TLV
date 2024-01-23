/**
 * @file adapter.h
 * @author skull (skull.gu@gmail.com)
 * @brief
 * @version 0.2
 * @date 2023-06-17
 *
 * @copyright Copyright (c) 2023 skull
 *
 */
#pragma once

#include <sys/time.h>
#include <time.h>
#include "dlist.h"

#undef DEBUG
#define DEBUG    1

#define tlv_list_init                dlist_init
#define tlv_list_foreach             dlist_foreach
#define tlv_list_add                 dlist_add
#define tlv_list_add_tail            dlist_add_tail
#define tlv_list_next_get            dlist_next_get
#define tlv_list_prev_get            dlist_prev_get
#define tlv_list_del                 dlist_del
#define tlv_list_node_count_get      dlist_node_count_get
typedef dlist_node_t    tlv_list_node_t;

// V: view, VO: only view
enum {LOG_LEVEL, V, D, I, W, E, NO, VO, DO, IO, WO, EO};

#define FILTER     V  // log filtering level (exclude oneself)
#define LOGV(...)    LOG(V, __VA_ARGS__)
#define LOGD(...)    LOG(D, __VA_ARGS__)
#define LOGI(...)    LOG(I, __VA_ARGS__)
#define LOGW(...)    LOG(W, __VA_ARGS__)
#define LOGE(...)    LOG(E, __VA_ARGS__)

#define OUTPUT    printf

// Level - Date Time - {Tag} - <func: line> - message
// E>09/16 11:17:33.990 {TEST-sku} <test: 373> This is test.
#if DEBUG
#define LOG(level, ...) \
    do { \
        if (level + NO == FILTER) { \
        } else if (level < FILTER) { \
            break; \
        } \
        level == V ? : OUTPUT(#level">%s " "{%.8s} " "<%s: %u> ", getCurrentTime(), TAG, __FILENAME__, __LINE__); \
        OUTPUT(__VA_ARGS__); \
        level == V ? : OUTPUT("\r\n"); \
    } while(0)

#define __FILENAME__    (strrchr(__FILE__, '\\') ? (strrchr(__FILE__, '\\') + 1) : __FILE__)
#define UNUSED(x)    (void)(x)
#define ASSERT(expr)    (void)((!!(expr)) || (OUTPUT("%s assert fail: \"%s\" @ %s, %u\r\n", TAG, #expr, __FILE__, __LINE__), assert_abort()))
#define CHECK_MSG(expr, msg, ...) \
    do { \
        if (!(expr)) { \
            LOGE("check fail: \"%s\". %s @ %s, %u", #expr, msg, __FILE__, __LINE__); \
            return __VA_ARGS__; \
        } \
    } while(0)
#define CHECK_MSG_PRE(expr, ...)    CHECK_MSG(expr, "Error occurred", __VA_ARGS__)
#define VA_NUM_ARGS_IMPL(_1,_2,__N,...)    __N
#define GET_MACRO(...)    VA_NUM_ARGS_IMPL(__VA_ARGS__, CHECK_MSG, CHECK_MSG_PRE)
#define CHECK(expr, ...)    GET_MACRO(__VA_ARGS__)(expr, __VA_ARGS__)
#else
#define LOG(...)        (void)0
#define ASSERT(expr)    (void)0
#endif
#define TLV_MALLOC(size)    malloc(size)
#define TLV_FREE(addr)      free(addr)

static char *getCurrentTime(void)
{
    static char currentTime[20];
    struct timeval now;
    gettimeofday(&now, NULL);
    struct tm* date = localtime(&now);
    strftime(currentTime, sizeof(currentTime), "%m/%d %H:%M:%S", date);
    sprintf(currentTime + 14, ".%03ld", now.tv_usec / 1000);

    return currentTime;
}
