/**
 * @file test.c
 * @author skull (skull.gu@gmail.com)
 * @brief
 * @version 0.1
 * @date 2023-06-17
 *
 * @copyright Copyright (c) 2023 skull
 *
 */
#include "tlv.h"

#undef TAG
#define TAG    "TEST-skullboyer"

/* TLV-tag定义规则：*/
// 高字节为0且低字节非0表示非嵌套，即有效范围0x01~0xff 计255个
// 高字节非0且低字节为0表示嵌套标记，即有效范围0x01~0x7f 计127个
// 高字节非0且低字节非0表示多级级嵌套
// 高字节为0且低字节为0暂时保留
// 高字节通常赋予功能模块意义，低字节通常赋予模块下事件意义
#define TEST_TAG_RESERVE    0x00
#define TEST_TAG01          0x01
#define TEST_TAG02          0x02
#define TEST_TAG03          0x03
#define TEST_TAG04          0x04
#define TEST_TAG05          0x05
#define TEST_TAG06          0x06
#define TEST_TAG07          0x07
#define TEST_TAG08          0x08
#define TEST_TAG09          0x09
#define TEST_TAG0a          0x0A
#define TEST_TAG0b          0x0B
#define TEST_TAG0c          0x0C
#define TEST_TAG0d          0x0D
#define TEST_TAG0e          0x0e
#define TEST_TAG0f          0x0f
#define TEST_TAGa0          (TEST_TAG0a<<8)
#define TEST_TAGa1          TEST_TAGa0 + TEST_TAG01
#define TEST_TAGa2          TEST_TAGa0 + TEST_TAG02
#define TEST_TAGb0          (TEST_TAG0b<<8)
#define TEST_TAGb1          TEST_TAGb0 + TEST_TAG01
#define TEST_TAGb2          TEST_TAGb0 + TEST_TAG01
#define TEST_TAGc0          (TEST_TAG0c<<8)
#define TEST_TAGc00         (TEST_TAGc0<<8)
#define TEST_TAGc1          TEST_TAGc0 + TEST_TAG01
#define TEST_TAGca1         TEST_TAGc00 + TEST_TAGa0 + TEST_TAG01
#define TEST_TAGca2         TEST_TAGc00 + TEST_TAGa0 + TEST_TAG02
#define TEST_TAGcb1         TEST_TAGc00 + TEST_TAGb0 + TEST_TAG01
#define TEST_TAGd0          (TEST_TAG0d<<8)
#define TEST_TAGd1          TEST_TAGd0 + TEST_TAG01

#define TLV_PACKET_HEAD             0x7F5A
#define TLV_PACKET_HEAD_SIZE        2
#define TLV_PACKET_LENGTH_SIZE      2
#define TLV_PACKET_CHECKSUM_SIZE    2


// Test case: TLV TLV TLV  TL TLV TLV  TL TL TLV TLV TL TLV  TL TLV

__attribute__((weak)) int8_t assert_abort(void)
{
    LOGE("###########");
    for(;;);
    return 0;
}

static uint16_t check_sum(uint8_t *data, uint16_t length)
{
    CHECK(data != NULL, 0);

    uint16_t check_sum = 0;
    for (uint16_t i = 0; i < length; i++) {
        check_sum += data[i];
    }

    LOGD("check sum: %x", check_sum);
    return check_sum;
}

static int8_t packet_verify(uint8_t *data)
{
    CHECK(data != NULL, -0xFF);

    if (*(uint16_t *)data != TLV_PACKET_HEAD) {
        LOGE("verify error @head: %x", *(uint16_t *)data);
        return -1;
    }

    uint16_t length = *(uint16_t *)(data + TLV_PACKET_HEAD_SIZE);
    uint16_t packet_check_sum = check_sum(data + TLV_PACKET_HEAD_SIZE + TLV_PACKET_LENGTH_SIZE, length);
    if (packet_check_sum != *(uint16_t *)(data + TLV_PACKET_HEAD_SIZE + TLV_PACKET_LENGTH_SIZE + length)) {
        LOGE("verify error @check-sum: %x", *(uint16_t *)(data + TLV_PACKET_HEAD_SIZE + TLV_PACKET_LENGTH_SIZE + length));
        return -2;
    }

    LOGD("verify pass");
    return 0;
}

static int8_t package_data(uint8_t **data, tlv_container_t *container_root)
{
    // TLV组包：HEAD + LENGTH + DATA + CHECK-SUM
    uint16_t malloc_size = container_root->serialized_size + TLV_PACKET_HEAD_SIZE + TLV_PACKET_LENGTH_SIZE + TLV_PACKET_CHECKSUM_SIZE;
    *data = (uint8_t *)TLV_MALLOC(malloc_size);
    ASSERT(data);
    memset(*data, 0, malloc_size);
    *(uint16_t *)(*data) = TLV_PACKET_HEAD;
    memcpy(*data + TLV_PACKET_HEAD_SIZE, &container_root->serialized_size, TLV_PACKET_LENGTH_SIZE);
    memcpy(*data + TLV_PACKET_HEAD_SIZE + TLV_PACKET_LENGTH_SIZE, container_root->serialized_data, container_root->serialized_size);
    uint16_t tlv_check_sum = check_sum(container_root->serialized_data, container_root->serialized_size);
    memcpy(*data + TLV_PACKET_HEAD_SIZE + TLV_PACKET_LENGTH_SIZE + container_root->serialized_size, &tlv_check_sum, TLV_PACKET_CHECKSUM_SIZE);
    uint16_t size = TLV_PACKET_HEAD_SIZE + TLV_PACKET_LENGTH_SIZE + TLV_PACKET_CHECKSUM_SIZE + *(uint16_t *)(*data + TLV_PACKET_HEAD_SIZE);
    LOGD("packet size: %u", size);
    for (int16_t i = 0; i < size; i++) {
        LOGV("%02x ", (*data)[i]);
    }
    LOGV("\r\n\r\n");
}

// All work and no play makes Jack a dull boy.
int8_t tlv_producer_manual(uint8_t **data)
{
    CHECK(*data == NULL, -0xFF);

    tlv_container_t *container_root = tlv_container_create();
    tlv_t *tlv = NULL;

    // -*- single: TLV TLV TLV -*-
    char *string = "TLV";
    tlv_package(&tlv, tlv_tag_get_low(TEST_TAG01), strlen(string), string);
    tlv_container_push(container_root, tlv);
    tlv_destroy(&tlv);

    string = "@";
    tlv_package(&tlv, tlv_tag_get_low(TEST_TAG02), strlen(string), string);
    tlv_container_push(container_root, tlv);
    tlv_destroy(&tlv);

    string = "skull";
    tlv_package(&tlv, tlv_tag_get_low(TEST_TAG03), strlen(string), string);
    tlv_container_push(container_root, tlv);
    tlv_destroy(&tlv);

    string = "boyer";
    tlv_package(&tlv, tlv_tag_get_low(TEST_TAG03), strlen(string), string);
    tlv_container_push(container_root, tlv);
    tlv_destroy(&tlv);
    LOGD(">>>>>>>>> TLV TLV TLV");

    // -*- multi: TL TLV TLV -*-
    tlv_container_t *container = tlv_container_create();
    string = "Half a loaf is better than no bread.";
    tlv_package(&tlv, tlv_tag_get_low(TEST_TAGa1), strlen(string), string);
    tlv_container_push(container, tlv);
    tlv_destroy(&tlv);

    string = "--Dedicate to oneself";
    tlv_package(&tlv, tlv_tag_get_low(TEST_TAGa2), strlen(string), string);
    tlv_container_push(container, tlv);
    tlv_destroy(&tlv);

    if (tlv_container_serialize(container) != 0) {
        LOGE("tlv container serialize fail");
    }
    tlv_nested_package(&tlv, tlv_tag_get_msb(TEST_TAGa0), container);
    tlv_container_push(container_root, tlv);
    tlv_destroy(&tlv);
    tlv_container_destroy(&container);
    LOGD(">>>>>>>>> TL TLV TLV");

    // -*- multi: TL TL TLV TLV TL TLV -*-
    container = tlv_container_create();
    string = "All work and";
    tlv_package(&tlv, tlv_tag_get_low(TEST_TAGca1), strlen(string), string);
    tlv_container_push(container, tlv);
    tlv_destroy(&tlv);

    string = "no play";
    tlv_package(&tlv, tlv_tag_get_low(TEST_TAGca2), strlen(string), string);
    tlv_container_push(container, tlv);
    tlv_destroy(&tlv);

    string = "skullboyer";
    tlv_package(&tlv, tlv_tag_get_low(TEST_TAGca2), strlen(string), string);
    tlv_container_push(container, tlv);
    tlv_destroy(&tlv);

    tlv_container_t *container1 = tlv_container_create();
    if (tlv_container_serialize(container) != 0) {
        LOGE("tlv container serialize fail");
    }
    tlv_nested_package(&tlv, tlv_tag_get_msb(TEST_TAGa0), container);
    tlv_container_push(container1, tlv);
    tlv_destroy(&tlv);
    tlv_container_destroy(&container);

    container = tlv_container_create();
    string = "makes Jack a dull boy.";
    tlv_package(&tlv, tlv_tag_get_low(TEST_TAGcb1), strlen(string), string);
    tlv_container_push(container, tlv);
    tlv_destroy(&tlv);

    if (tlv_container_serialize(container) != 0) {
        LOGE("tlv container serialize fail");
    }
    tlv_nested_package(&tlv, tlv_tag_get_msb(TEST_TAGb0), container);
    tlv_container_push(container1, tlv);
    tlv_destroy(&tlv);
    tlv_container_destroy(&container);

    uint32_t value = 0xA9A4E5;  // 天
    tlv_package(&tlv, tlv_tag_get_low(TEST_TAGc1), sizeof(value), &value);
    tlv_container_push(container1, tlv);
    tlv_destroy(&tlv);

    if (tlv_container_serialize(container1) != 0) {
        LOGE("tlv container serialize fail");
    }
    tlv_nested_package(&tlv, tlv_tag_get_msb(TEST_TAGc00), container1);
    tlv_container_push(container_root, tlv);
    tlv_destroy(&tlv);
    tlv_container_destroy(&container1);
    LOGD(">>>>>>>>> TL TL TLV TLV TL TLV");

    // -*- multi: TL TLV -*-
    container = tlv_container_create();
    value = 0xB581E7;  // 灵
    tlv_package(&tlv, tlv_tag_get_low(TEST_TAGb1), sizeof(value), &value);
    tlv_container_push(container, tlv);
    tlv_destroy(&tlv);

    if (tlv_container_serialize(container) != 0) {
        LOGE("tlv container serialize fail");
    }
    tlv_nested_package(&tlv, tlv_tag_get_msb(TEST_TAGb0), container);
    tlv_container_push(container_root, tlv);
    tlv_destroy(&tlv);
    tlv_container_destroy(&container);

    container = tlv_container_create();
    value = 0x969BE7;  // 盖
    tlv_package(&tlv, tlv_tag_get_low(TEST_TAGd1), sizeof(value), &value);
    tlv_container_push(container, tlv);
    tlv_destroy(&tlv);

    if (tlv_container_serialize(container) != 0) {
        LOGE("tlv container serialize fail");
    }
    tlv_nested_package(&tlv, tlv_tag_get_msb(TEST_TAGd0), container);
    tlv_container_push(container_root, tlv);
    tlv_destroy(&tlv);
    tlv_container_destroy(&container);
    LOGD(">>>>>>>>> TL TLV");

    tlv_node_show(container_root);
    dlist_node_count_get(&container_root->tlv_group->list);

    // -*- serialize -*-
    if (tlv_container_serialize(container_root) != 0) {
        LOGE("tlv container_root serialize fail");
    }
    LOGD("tlv serialize length: %u, offset: %u", container_root->serialized_size, container_root->serialized_offset);

    LOGD("tlv serialize string: ");
    for (int16_t i = 0; i < container_root->serialized_size; i++) {
        LOGV("%02x ", container_root->serialized_data[i]);
    }
    LOGV("\r\n\r\n");

    package_data(data, container_root);
    tlv_container_destroy(&container_root);

    return 0;
}

int8_t tlv_producer_smart(uint8_t **data)
{
    CHECK(*data == NULL, -0xFF);

    tlv_container_t *container_root = NULL;
    char *string = "TLV";
    LOGD(">>>>>>>>> tlv process: \"%s\"", string);
    tlv_container_handle(&container_root, TEST_TAG01, strlen(string), string);
    string = "@";
    LOGD(">>>>>>>>> tlv process: \"%s\"", string);
    tlv_container_handle(&container_root, TEST_TAG02, strlen(string), string);
    string = "skull";
    LOGD(">>>>>>>>> tlv process: \"%s\"", string);
    tlv_container_handle(&container_root, TEST_TAG03, strlen(string), string);
    string = "boyer";
    LOGD(">>>>>>>>> tlv process: \"%s\"", string);
    tlv_container_handle(&container_root, TEST_TAG03, strlen(string), string);
    string = "Half a loaf is better than no bread.";
    LOGD(">>>>>>>>> tlv process: \"%s\"", string);
    tlv_container_handle(&container_root, TEST_TAGa1, strlen(string), string);
    string = "All work and";
    LOGD(">>>>>>>>> tlv process: \"%s\"", string);
    tlv_container_handle(&container_root, TEST_TAGca1, strlen(string), string);
    string = "no play";
    LOGD(">>>>>>>>> tlv process: \"%s\"", string);
    tlv_container_handle(&container_root, TEST_TAGca2, strlen(string), string);
    string = "skullboyer";
    LOGD(">>>>>>>>> tlv process: \"%s\"", string);
    tlv_container_handle(&container_root, TEST_TAGca2, strlen(string), string);
    uint32_t value = 0xB581E7;  // 灵
    LOGD(">>>>>>>>> tlv process: \"%#x\"", value);
    tlv_container_handle(&container_root, TEST_TAGb1, sizeof(value), &value);
    string = "makes Jack a dull boy.";
    LOGD(">>>>>>>>> tlv process: \"%s\"", string);
    tlv_container_handle(&container_root, TEST_TAGcb1, strlen(string), string);
    string = "--Dedicate to oneself";
    LOGD(">>>>>>>>> tlv process: \"%s\"", string);
    tlv_container_handle(&container_root, TEST_TAGa2, strlen(string), string);
    value = 0x969BE7;  // 盖
    LOGD(">>>>>>>>> tlv process: \"%#x\"", value);
    tlv_container_handle(&container_root, TEST_TAGd1, sizeof(value), &value);
    value = 0xA9A4E5;  // 天
    LOGD(">>>>>>>>> tlv process: \"%#x\"", value);
    tlv_container_handle(&container_root, TEST_TAGc1, sizeof(value), &value);

    tlv_list_foreach(&container_root->tlv_group->list, tlv_all_printf, NULL);

    if (tlv_container_serialize(container_root) != 0) {
        LOGE("tlv container_root serialize fail");
    }
    LOGD("tlv serialize length: %u, offset: %u", container_root->serialized_size, container_root->serialized_offset);

    LOGD(">>>>>>>>> tlv serialize string: ");
    for (int16_t i = 0; i < container_root->serialized_size; i++) {
        LOGV("%02x ", container_root->serialized_data[i]);
    }
    LOGV("\r\n\r\n");
    package_data(data, container_root);
    tlv_container_destroy(&container_root);
    return 0;
}

int print_tlv_info(uint8_t *data, uint16_t size)
{
    CHECK(data != NULL, -0xFF);

    LOGV("-*-*-*-*-*-*-*-*-*-*-*-*-*-TLV Decode-*-*-*-*-*-*-*-*-*-*-*-*-*-\r\n");
    while(size--) {
        LOGV("%c", *(data++));
    }
    LOGV("\r\n\r\n");

    return 0;
}

int module_handle_simple(void **p_arg, tlv_list_node_t **p_node)
{
    UNUSED(p_arg);
    CHECK(*p_node != NULL, -0xFF);

    LOGI("tlv tag: %x, length: %u", ((tlv_group_t *)(*p_node))->data->tag, ((tlv_group_t *)(*p_node))->data->length);
    LOGD("\r\nTLV: %s", ((tlv_group_t *)(*p_node))->data->value);

    uint16_t length = ((tlv_group_t *)(*p_node))->data->length;
    print_tlv_info(((tlv_group_t *)(*p_node))->data->value, length);

    return 0;
}

int module_event_process(void **p_arg, tlv_list_node_t **p_node)
{
    UNUSED(p_arg);
    CHECK(*p_node != NULL, -0xFF);

    tlv_container_t *container = NULL;
    uint8_t tag = ((tlv_group_t *)(*p_node))->data->tag;
    uint8_t tlv_nested = tlv_nested_get(tag);
    if (!!tlv_nested) {
        tlv_container_pop(&container, ((tlv_group_t *)(*p_node))->data);
    } else {
        LOGD("%%%% event: %02X", TEST_TAG_RESERVE);
        module_handle_simple(NULL, p_node);
        return 0;
    }

    LOGD(">>>>>>>>> module: %02X", tlv_tag_restore(tag));
    switch(tlv_tag_restore(tag)) {
        case TEST_TAG0a:
        case TEST_TAG0b:
        case TEST_TAG0d:
            LOGD(">>>>>>>>> event start");
            tlv_list_foreach(&container->tlv_group->list, module_handle_simple, NULL);
            LOGD("<<<<<<<<< event end");
            break;
        case TEST_TAG0c:
            tlv_list_foreach(&container->tlv_group->list, module_event_process, NULL);
            break;
        default: break;
    }
    tlv_container_destroy(&container);
    return 0;
}

int8_t tlv_consumer(uint8_t **data)
{
    CHECK(*data != NULL, -0xFF);

    tlv_container_t *container = tlv_container_create();
    container->serialized_offset = 0;
    container->serialized_size = *(uint16_t *)(*data + TLV_PACKET_HEAD_SIZE);
    container->serialized_data = (uint8_t *)TLV_MALLOC(container->serialized_size);
    memcpy(container->serialized_data, *data + TLV_PACKET_HEAD_SIZE + TLV_PACKET_LENGTH_SIZE, container->serialized_size);
    tlv_destroy(data);
    LOGD("size: %u, data: %x", container->serialized_size, *container->serialized_data);
    tlv_container_parse(container);
    // 查看收到的内容
    tlv_list_foreach(&container->tlv_group->list, tlv_all_printf, NULL);
    tlv_list_foreach(&container->tlv_group->list, module_event_process, NULL);
    tlv_container_destroy(&container);
    return 0;
}

void test(void)
{
    LOGV("\r\n-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-\r\n");
    LOGD("This is test."); usleep(15*1000);
    LOGI("This is test."); usleep(25*1000);
    LOGW("This is test."); usleep(35*1000);
    LOGE("This is test.");
}

int main(int argc, char const *argv[])
{
    uint8_t *tlv_packet = NULL;
    if (argc < 2) {
        test();
        return 0;
    }

    if (!strcmp(argv[1], "manual")) {
        tlv_producer_manual(&tlv_packet);
    } else if (!strcmp(argv[1], "smart")) {
        tlv_producer_smart(&tlv_packet);
    } else {
        LOGE("Option parameter error!");
        return -1;
    }
    LOGD("-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-");
    packet_verify(tlv_packet);
    LOGD("-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-");
    tlv_consumer(&tlv_packet);

    return 0;
}
