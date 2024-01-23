/**
 * @file tlv.h
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
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "adapter.h"

#pragma pack(1)
typedef struct {
    uint8_t tag;  // The most significant bit set to 1 indicates a nested tlv
    uint16_t length;
    uint8_t value[0];
} tlv_t;

typedef struct {
    tlv_list_node_t list;
    tlv_t data[0];
} tlv_group_t;

typedef struct _tlv_container {
    tlv_group_t *tlv_group;
    struct _tlv_container *container;
    uint8_t tlv_nested_count;
    uint16_t serialized_size;
    uint16_t serialized_offset;
    uint8_t *serialized_data;
} tlv_container_t;
#pragma pack()

int tlv_all_printf(void **p_arg, tlv_list_node_t **p_node);

uint8_t tlv_tag_get_msb(uint32_t type);
uint8_t tlv_tag_get_low(uint32_t type);
uint8_t tlv_tag_restore(uint8_t tag);
uint8_t tlv_nested_get(uint8_t tag);
int8_t tlv_package(tlv_t **tlv, uint8_t tag, uint16_t length, void *data);
int8_t tlv_nested_package(tlv_t **tlv, uint8_t tag, tlv_container_t *container);
int8_t tlv_destroy(void **mem);
tlv_container_t *tlv_container_create(void);
int8_t tlv_container_push(tlv_container_t *container, tlv_t *tlv);
int8_t tlv_container_serialize(tlv_container_t *container);
int8_t tlv_container_pop(tlv_container_t **container, tlv_t *tlv);
int8_t tlv_container_destroy(tlv_container_t **container);
int8_t tlv_node_show(tlv_container_t *container);
int8_t tlv_container_parse(tlv_container_t *container);
int8_t tlv_container_handle(tlv_container_t **container_root, uint32_t type, uint16_t length, void *data);
