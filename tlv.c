/**
 * @file tlv.c
 * @author skull (skull.gu@gmail.com)
 * @brief packaging and unpackaging of tlv format data
 * @version 0.1
 * @date 2023-06-17
 *
 * @copyright Copyright (c) 2023 skull
 *
 */
#include "tlv.h"

#undef TAG
#define TAG    "TLV"

// 添加tlv节点时，container中存在相同节点是否替换
#define CFG_TLV_NODE_REPLACE    0

static int8_t assert_abort(void)
{
    LOGE("xxxxxxxxxxx");
    // for(;;);
    return 0;
}

int tlv_printf(tlv_t *tlv)
{
    CHECK(tlv != NULL, -0xFF);
    LOGV("-*-*-*-*-*-*-*-*-*-*-*-*-*-TLV Encode-*-*-*-*-*-*-*-*-*-*-*-*-*-\r\n");
    for (uint16_t i = 0; i < tlv->length; i++) {
        LOGV("%c ", tlv->value[i]);
    }
    LOGV("\r\n");
    for (uint16_t i = 0; i < tlv->length; i++) {
        LOGV("%#02x ", tlv->value[i]);
    }
    LOGV("\r\n\r\n");
    return 0;
}

int tlv_all_printf(void **p_arg, tlv_list_node_t **p_node)
{
    UNUSED(p_arg);
    CHECK(*p_node != NULL, -0xFF);
    LOGV("-*-*-*-*-*-*-*-*-*-*-*-*-*-TLV Encode-*-*-*-*-*-*-*-*-*-*-*-*-*-\r\n");
    LOGV("node list next: %p, prev: %p\r\n", tlv_list_next_get(*p_node), tlv_list_prev_get(*p_node));
    LOGV("tag: %x, length:%x, data: ", ((tlv_group_t *)*p_node)->data->tag, ((tlv_group_t *)*p_node)->data->length);
    for (uint16_t i = 0; i < ((tlv_group_t *)*p_node)->data->length; i++) {
        LOGV("%c ", ((tlv_group_t *)*p_node)->data->value[i]);
    }
    LOGV("\r\ntag: %x, length:%x, data: ", ((tlv_group_t *)*p_node)->data->tag, ((tlv_group_t *)*p_node)->data->length);
    for (uint16_t i = 0; i < ((tlv_group_t *)*p_node)->data->length; i++) {
        LOGV("%#02x ", ((tlv_group_t *)*p_node)->data->value[i]);
    }
    LOGV("\r\n\r\n");
    return 0;
}

/**
 * @brief  获取tag嵌套数
 *
 */
uint8_t tlv_tag_get_count(uint32_t tag)
{
    return (!!(tag>>24) + !!(tag>>16) + !!(tag>>8) + !!tag);
}

/**
 * @brief  获取最高有效字节，有效字节非最低字节时字节高位置1
 *
 */
uint8_t tlv_tag_get_msb(uint32_t tag)
{
    return (!!(tag>>24) ? tag>>24 | 0x80 : !!(tag>>16) ? tag>>16 | 0x80 : !!(tag>>8) ? tag>>8 | 0x80 : tag) & 0xFF;
}

uint8_t tlv_tag_get_low(uint32_t tag)
{
    return tag & 0xFF;
}

/**
 * @brief  拿取tag指定的字节值
 *
 * @param tag
 * @param index 指定第几个字节（0~sizeof(data-type)）
 * @return uint8_t
 */
static uint8_t tlv_tag_get_byte(uint32_t tag, uint8_t index)
{
    ASSERT(index < sizeof(uint32_t));
    return !!(tag>>(index*8)) ? (tag>>(index*8) | (!!index ? 0x80 : 0x00)) & 0xFF : 0x00;
}

/**
 * @brief  自高字节向下遍历，获取指定tag的下个字节
 *
 */
static uint8_t tlv_tag_get_next_byte(uint32_t type, uint8_t tag)
{
    uint8_t index = tlv_tag_get_count(type);
    while (--index) {
        if (tlv_tag_get_byte(type, index) == tag) {
            break;
        }
    }
    if (index == 0) {
        LOGW("the tag next byte is null");
        return 0;
    }
    return tlv_tag_get_byte(type, --index);
}

uint8_t tlv_tag_restore(uint8_t tag)
{
    return tag & 0x7F;
}

uint8_t tlv_nested_get(uint8_t tag)
{
    return tag & 0x80;
}

uint8_t tlv_nested_number_get(uint32_t type)
{
    return !!(type>>24) ? 4 : !!(type>>16) ? 3 : !!(type>>8) ? 2 : !!type ? 1 : 0;
}

static tlv_group_t *tlv_group_create()
{
    tlv_group_t *tlv_group = (tlv_group_t *)TLV_MALLOC(sizeof(tlv_group_t));
    ASSERT(tlv_group != NULL);
    memset(tlv_group, 0, sizeof(tlv_group_t));
    tlv_list_init(&tlv_group->list);
    return tlv_group;
}

static int find_duplicate_key(void **p_arg, tlv_list_node_t **p_node)
{
    CHECK(*p_arg != NULL, -0xFF);
    CHECK(*p_node != NULL, -0xFF);
    LOGD("parg-tag: %x, pnode-tag: %x, node list next: %p, prev: %p", ((tlv_group_t *)(*p_arg))->data->tag,
        ((tlv_group_t *)*p_node)->data->tag, tlv_list_next_get(*p_node), tlv_list_prev_get(*p_node));
    if (((tlv_group_t *)*p_node)->data->tag == ((tlv_group_t *)(*p_arg))->data->tag) {
        LOGD("hit list node. Tag: %x", tlv_tag_restore(((tlv_group_t *)*p_node)->data->tag));
        // 记录命中的节点
        memcpy(*p_arg, *p_node, sizeof(tlv_list_node_t));
        LOGD("parg: %p, pnode->lsit: %p, pnode->data: %x", (tlv_group_t *)*p_arg, &((tlv_group_t *)*p_arg)->list, *((tlv_group_t *)*p_arg)->data);
        return -1;
    }
    return 0;
}

/**
 * @brief  tlv节点挂入链表
 *
 * @param tlv_group
 * @param tlv_node
 * @return int8_t
 */
static int8_t tlv_group_put(tlv_group_t *tlv_group, tlv_group_t *tlv_node)
{
    CHECK(tlv_node != NULL, -0xFF);
    CHECK(tlv_group != NULL, -0xFF);
    // 检查链表上是否存在与当前tlv节点相同的tag，若存在则不挂载
    if (tlv_list_foreach(&tlv_group->list, find_duplicate_key, tlv_node) != 0) {
        LOGW("tlv-tag already exists.");
        return -1;
    }

    LOGD("check tlv-node addr: node= %p, list= %p", tlv_node, &tlv_node->list);
    return tlv_list_add_tail(&tlv_group->list, &tlv_node->list);
}

static int replace_tlv_node(void **p_arg, tlv_list_node_t **p_node)
{
    CHECK(*p_arg != NULL, -0xFF);
    CHECK(*p_node != NULL, -0xFF);
    if (((tlv_group_t *)*p_node)->data->tag == ((tlv_group_t *)(*p_arg))->data->tag) {
        LOGD("hit list node.");
        // 挂接新节点到本container链表中同tag节点的前面，删除原tag节点
        tlv_list_add(tlv_list_prev_get(*p_node), *p_arg);
        tlv_list_del(*p_node);
        // 返回原有tag对应节点数据长度
        int reval = ((tlv_group_t *)*p_node)->data->length + sizeof(tlv_t);
        tlv_destroy(p_node);
        return reval;
    }
    return 0;
}

/**
 * @brief  替换链表中原有tag对应的节点
 */
static int8_t tlv_group_replace(tlv_container_t *container, tlv_group_t *tlv_node)
{
    CHECK(tlv_node != NULL, -0xFF);
    CHECK(container != NULL, -0xFF);
    // 检查链表上是否存在与当前tlv节点相同的tag，有则替换
    int reval = tlv_list_foreach(&container->tlv_group->list, replace_tlv_node, tlv_node);
    if (reval != 0) {
        LOGW("tlv-tag replace complete. serialized size: %#x, reval: %#x", container->serialized_size, reval);
        // 序列化长度减去原有tag对应的节点长度
        container->serialized_size -= reval;
        return 0;
    }

    LOGD("check tlv-node addr: node= %p, list= %p", tlv_node, &tlv_node->list);
    return tlv_list_add_tail(&container->tlv_group->list, &tlv_node->list);
}

tlv_container_t *tlv_container_create(void)
{
    tlv_container_t *container = (tlv_container_t *)TLV_MALLOC(sizeof(tlv_container_t));
    ASSERT(container != NULL);
    memset(container, 0, sizeof(tlv_container_t));
    container->tlv_group = tlv_group_create();
    container->tlv_nested_count = 0;
    container->serialized_size = 0;
    container->serialized_data = NULL;
    container->container = NULL;
    return container;
}

/**
 * @brief  将tlv节点挂入链表，若存在相同tag则舍弃挂载节点
 *
 */
int8_t tlv_container_new_push(tlv_container_t *container, tlv_t *tlv)
{
    CHECK(tlv != NULL, -0xFF);
    CHECK(container != NULL, -0xFF);
    uint16_t tlv_size = sizeof(tlv_t) + tlv->length;
    uint16_t tlv_group_size = sizeof(tlv_group_t) + tlv_size;
    tlv_group_t *tlv_group = (tlv_group_t *)TLV_MALLOC(tlv_group_size);
    ASSERT(tlv_group != NULL);
    memset(tlv_group, 0, tlv_group_size);
    memcpy(tlv_group->data, tlv, tlv_size);
    LOGD("tlv-node tag: %x, length: %u", tlv_group->data->tag, tlv_group->data->length);
    if (tlv_group_put(container->tlv_group, tlv_group) != 0) {
        LOGW("add tlv node fail.");
        tlv_destroy(&tlv_group);
        return -1;
    }
    container->serialized_size += tlv_size;
    return 0;
}

/**
 * @brief  去除原有tag对应tlv节点，并将新tlv节点挂入链表原位置
 *
 * @param container
 * @param tlv
 * @return int8_t
 */
int8_t tlv_container_fix_push(tlv_container_t *container, tlv_t *tlv)
{
    CHECK(tlv != NULL, -0xFF);
    CHECK(container != NULL, -0xFF);
    uint16_t tlv_size = sizeof(tlv_t) + tlv->length;
    uint16_t tlv_group_size = sizeof(tlv_group_t) + tlv_size;
    tlv_group_t *tlv_group = (tlv_group_t *)TLV_MALLOC(tlv_group_size);
    ASSERT(tlv_group != NULL);
    memset(tlv_group, 0, tlv_group_size);
    memcpy(tlv_group->data, tlv, tlv_size);
    LOGD("tlv-node tag: %x, length: %x", tlv_group->data->tag, tlv_group->data->length);
    tlv_group_replace(container, tlv_group);
    container->serialized_size += tlv_size;
    return 0;
}

int8_t tlv_container_push(tlv_container_t *container, tlv_t *tlv)
{
    CHECK(container != NULL, -0xFF);
    CHECK(tlv != NULL, -0xFF);
#if CFG_TLV_NODE_REPLACE
    return tlv_container_fix_push(container, tlv);
#else
    return tlv_container_new_push(container, tlv);
#endif
}

/**
 * @brief  解出TLV节点中序列化的数据Value
 *
 */
int8_t tlv_container_pop(tlv_container_t **container, tlv_t *tlv)
{
    CHECK(tlv != NULL, -0xFF);
    CHECK(*container == NULL, -0xFF);
    tlv_container_t *tlv_container = tlv_container_create();
    tlv_container->serialized_offset = 0;
    tlv_container->serialized_size = tlv->length;
    tlv_container->serialized_data = tlv->value;
    LOGD("size: %u, data: %x", tlv_container->serialized_size, *tlv_container->serialized_data);
    tlv_container_parse(tlv_container);
    *container = tlv_container;
    //tlv_container->serialized_size = 0;
    tlv_container->serialized_data = NULL;
    return 0;
}

static int structure_convert_bytes(void **p_arg, tlv_list_node_t **p_node)
{
    CHECK(*p_node != NULL, -0xFF);
    CHECK(*p_arg != NULL, -0xFF);
    tlv_t *tlv = ((tlv_group_t *)*p_node)->data;
    memcpy(((tlv_container_t *)*p_arg)->serialized_data + ((tlv_container_t *)*p_arg)->serialized_offset, tlv, sizeof(tlv_t) + tlv->length);
    ((tlv_container_t *)*p_arg)->serialized_offset += sizeof(tlv_t) + tlv->length;
    LOGD("tlv tag: %x, length: %u, serialized offset: %u", tlv->tag, tlv->length, ((tlv_container_t *)*p_arg)->serialized_offset);
    return 0;
}

int8_t tlv_container_serialize(tlv_container_t *container)
{
    CHECK(container != NULL, -0xFF);
    CHECK(container->serialized_data == NULL, -0xFF);
    container->serialized_offset = 0;
    LOGD("container serialized size: %u", container->serialized_size);
    container->serialized_data = (uint8_t *)TLV_MALLOC(container->serialized_size);
    ASSERT(container->serialized_data != NULL);
    memset(container->serialized_data, 0, container->serialized_size);
    tlv_list_foreach(&container->tlv_group->list, structure_convert_bytes, container);

    if (container->serialized_offset != container->serialized_size) {
        LOGE("tlv serialize error.");
        tlv_destroy(&container->serialized_data);
        return -2;
    }

    return 0;
}

/**
 * @brief  单级TLV结构解析
 *
 */
int8_t tlv_container_parse(tlv_container_t *container)
{
    CHECK(container != NULL, -0xFF);
    CHECK(container->serialized_data != NULL, -0xFF);
    tlv_t *tlv = NULL;
    LOGD("offset: %u, size: %#x, data: %#x", container->serialized_offset, container->serialized_size, container->serialized_data[container->serialized_offset]);
    tlv_list_node_t *temp_list = &container->tlv_group->list;
    uint16_t boundary = container->serialized_size;  // 取出循环限值，serialized_size会在container_push接口中修改
    container->serialized_size = 0;
    /* 反序列化，拆解单级tlv到container */
    while (container->serialized_offset < boundary) {
        uint8_t tag = container->serialized_data[container->serialized_offset];
        uint16_t length = *(uint16_t *)(&(container->serialized_data[container->serialized_offset += sizeof(container->tlv_group->data->tag)]));
        void *data = &(container->serialized_data[container->serialized_offset += sizeof(container->tlv_group->data->length)]);
        // package参数直接输入上面表达式的右值会异常，所以做暂存变量中转（很是奇怪）
        tlv_package(&tlv, tag, length, data);
        tlv_container_push(container, tlv);
        tlv_destroy(&tlv);
        LOGD("tag: %x, length: %#x", ((tlv_group_t *)tlv_list_next_get(temp_list))->data->tag, ((tlv_group_t *)tlv_list_next_get(temp_list))->data->length);
        container->serialized_offset += ((tlv_group_t *)tlv_list_next_get(temp_list))->data->length;
        LOGD("parse container serialize data offset: %u, size: %u", container->serialized_offset, container->serialized_size);
        temp_list = tlv_list_next_get(temp_list);
    }

    return 0;
}

int8_t tlv_package(tlv_t **tlv, uint8_t tag, uint16_t length, void *data)
{
    CHECK(*tlv == NULL, -0xFF);
    CHECK(data != NULL, -0xFF);
    *tlv = (tlv_t *)TLV_MALLOC(sizeof(tlv_t) + length);
    ASSERT(*tlv != NULL);
    memset(*tlv, 0, sizeof(tlv_t) + length);
    LOGD("tag: %x, length: %x, data: %s", tag, length, data);
    (*tlv)->tag = tag;
    (*tlv)->length = length;
    memcpy((*tlv)->value, data, length);
    return 0;
}

int8_t tlv_nested_package(tlv_t **tlv, uint8_t tag, tlv_container_t *container)
{
    CHECK(*tlv == NULL, -0xFF);
    CHECK(container != NULL, -0xFF);
    *tlv = (tlv_t *)TLV_MALLOC(sizeof(tlv_t) + container->serialized_size);
    ASSERT(*tlv != NULL);
    memset(*tlv, 0, sizeof(tlv_t) + container->serialized_size);
    (*tlv)->tag = tag;
    (*tlv)->length = container->serialized_size;
    memcpy((*tlv)->value, container->serialized_data, container->serialized_size);
    LOGD("tlv tag: %x, length: %x, value: %x", (*tlv)->tag, (*tlv)->length, *(*tlv)->value);
    return 0;
}

/**
 * @brief  用于往根container结构中挂载修改后的tlv节点
 *
 * @param container_root  根container
 * @param container  要挂载的节点container
 * @param tag  挂接数据的tag
 * @return int8_t
 */
int8_t tlv_group_mount(tlv_container_t *container_root, tlv_container_t *container, uint8_t tag)
{
    CHECK(container_root != NULL, -0xFF);
    CHECK(container != NULL, -0xFF);
    LOGD("tag: %x", tag);
    tlv_group_t *tlv_group = (tlv_group_t *)TLV_MALLOC(sizeof(tlv_group_t) + sizeof(tlv_t) + container->container->serialized_size);
    ASSERT(tlv_group != NULL);
    tlv_list_foreach(&container_root->tlv_group->list, tlv_all_printf, NULL);
    LOGD(">>>>>>>>> mount@before.");
    // 挂接新节点到根container链表中同tag节点的前面（注：此时挂接上去的是空节点，待后面增添内容）
    tlv_list_add(tlv_list_prev_get(&container->tlv_group->list), &tlv_group->list);
    LOGD("tlv group list node, next: %p, prev: %p, container list prev: %p", tlv_list_next_get(&tlv_group->list),
        tlv_list_prev_get(&tlv_group->list), tlv_list_prev_get(&container->tlv_group->list));
    // 复位要挂载的节点container的链表钩子，防止释放container时额外释放其上的链表数据
    tlv_list_init(&container->tlv_group->list);
    tlv_list_foreach(&container_root->tlv_group->list, tlv_all_printf, NULL);  // 此输出可能有乱数据，因为有空节点
    LOGD(">>>>>>>>> mount@after.");
    tlv_group->data->tag = tag;
    tlv_group->data->length = container->container->serialized_size;
    memcpy(tlv_group->data->value, container->container->serialized_data, container->container->serialized_size);
    container_root->serialized_size += tlv_group->data->length - ((tlv_group_t *)tlv_list_next_get(&tlv_group->list))->data->length;
    // 备份要删除的节点，即链表中存在的同tag节点，用于释放对应的内存
    tlv_group_t *tlv_group_record = (tlv_group_t *)tlv_list_next_get(&tlv_group->list);
    LOGD("delete list node: %p", tlv_list_next_get(&tlv_group->list));
    tlv_list_del(tlv_list_next_get(&tlv_group->list));
    tlv_list_foreach(&container_root->tlv_group->list, tlv_all_printf, NULL);
    LOGD(">>>>>>>>> mount@clean.");
    TLV_FREE(tlv_group_record);
    LOGD("tlv tag: %x, length: %u", tlv_group->data->tag, tlv_group->data->length);
    return 0;
}

int8_t tlv_destroy(void **mem)
{
    CHECK(*mem != NULL, -0xFF);
    TLV_FREE(*mem);
    *mem = NULL;
    return 0;
}

/**
 * @brief  删除链表节点，并释放节点内存
 *
 * @param p_arg
 * @param p_node
 * @return int
 */
static int tlv_node_free(void **p_arg, tlv_list_node_t **p_node)
{
    CHECK(*p_node != NULL, -0xFF);
    CHECK(*p_arg != NULL, -0xFF);
    LOGD("tlv-node free: %p", *p_node);
    tlv_list_del(*p_node);
    tlv_destroy(p_node);
    // 更新当前释放节点为头结点，用于链表继续遍历
    *p_node = &((tlv_container_t *)*p_arg)->tlv_group->list;
    return 0;
}

/**
 * @brief  释放tlv-container空间
 *
 */
int8_t tlv_container_destroy(tlv_container_t **container)
{
    CHECK(*container != NULL, -0xFF);
    if ((*container)->serialized_data != NULL) {
        TLV_FREE((*container)->serialized_data);
    }
    tlv_list_foreach(&(*container)->tlv_group->list, tlv_node_free, *container);
    tlv_list_node_count_get(&(*container)->tlv_group->list);
    TLV_FREE((*container)->tlv_group);
    TLV_FREE(*container);
    *container = NULL;
    return 0;
}

static int tlv_show(void **p_arg, tlv_list_node_t **p_node)
{
    UNUSED(p_arg);
    CHECK(*p_node != NULL, -0xFF);
    LOGD("tlv: tag= %u, length= %u, addr@ %p", ((tlv_group_t *)*p_node)->data->tag, ((tlv_group_t *)*p_node)->data->length, *p_node);
    return 0;
}

/**
 * @brief  遍历查看链表各节点信息
 *
 */
int8_t tlv_node_show(tlv_container_t *container)
{
    CHECK(container != NULL, "tlv container no exists.", -0xFF);
    tlv_list_foreach(&container->tlv_group->list, tlv_show, NULL);
    return 0;
}

tlv_container_t *tlv_container_next(tlv_container_t *container)
{
    ASSERT(container != NULL);
    return container->container;
}

tlv_container_t *tlv_container_prev(tlv_container_t *container_root, tlv_container_t *container)
{
    CHECK(container != NULL, -0xFF);
    CHECK(container_root != NULL, -0xFF);
    LOGD("target container: %x", container);
    tlv_container_t *container_temp = container_root;
    while (tlv_container_next(container_root) != NULL) {
        if (container_temp->container == container) {
            LOGD("return container: %x", container_temp);
            return container_temp;
        }
        container_temp = container_temp->container;
    }
    return NULL;
}

/**
 * @brief  获取最后一级container
 *
 */
tlv_container_t **tlv_container_last(tlv_container_t **container)
{
    CHECK(*container != NULL, -0xFF);
    LOGD("%s, @%x", __func__, *container);
    return ((*container)->container != NULL) ? tlv_container_last(&(*container)->container) : container;
}

/**
 * @brief  TLV数据智能组包
 *
 * @param container_root
 * @param type
 * @param length
 * @param data
 * @return int8_t
 *
 * @note
 *  case: 1    2    3  1    4  1  11  12  2  21   5    6  1  11  12 121 122 13  2    7  1    8
 *        TLV  TLV  TL TLV  TL TL TLV TLV TL TLV  TLV  TL TL TLV TL TLV TLV TLV TLV  TL TLV  TLV    序列化
 *        TLV->TLV-->TLV--->TLV------------------>TLV->TLV-------------------------->TLV---->TLV    一级
 *                     |  1   |  1           2           |  1    2                     |  1
 *                     +->TLV +->TLV-------->TLV         +->TLV->TLV                   +->TLV       二级
 *                                 |  1    2   |  1           |  1    2    3
 *                                 +->TLV->TLV +->TLV         +->TLV->TLV->TLV                      三级
 *                                                                      |  1    2
 *                                                                      +->TLV->TLV                 底级
 *  add complex tlv：Type=6123
 *    1. 检索：以Type顶级Tag=6在tlv-container上查找，命中Tag=6的节点
 *    2. 拆解：取下Tag=6的节点，TL TL TLV TL TLV TLV TLV TLV (6  1  11  12 121 122 13  2)
 *      2.1 拆解一级TLV：TL TLVTLTLVTLVTLV (61)，TLV(62)
 *      2.2 以Type次级Tag=1，拆解二级TLV：TLV(611)，TLTLVTLV(612)，TLV(613)
 *      2.2 以Type次次级Tag=2，拆解三级TLV：TLV(6121)，TLV(6122)
 *    3. 融合：将Type底级Tag=3的节点，与拆解后的底级节点融合：TLV(6121)，TLV(6122)，TLV(6123)
 *    4. 序列化：TL TL TLV TL TLV TLV TLV TLV TLV (6  1  11  12 121 122 123 13  2)
 *    5. 重挂载：
 *        1    2    3  1    4  1  11  12  2  21   5    6  1  11  12 121 122 123 13  2    7  1    8
 *        TLV  TLV  TL TLV  TL TL TLV TLV TL TLV  TLV  TL TL TLV TL TLV TLV TLV TLV TLV  TL TLV  TLV
 *        TLV->TLV-->TLV--->TLV------------------>TLV->TLV------------------------------>TLV---->TLV
 *                     |  1   |  1           2           |  1    2                       |  1
 *                     +->TLV +->TLV-------->TLV         +->TLV->TLV                     +->TLV
 *                                 |  1    2   |  1           |  1    2    3
 *                                 +->TLV->TLV +->TLV         +->TLV->TLV->TLV
 *                                                                      |  1    2    3
 *                                                                      +->TLV->TLV->TLV
 */
int8_t tlv_container_handle(tlv_container_t **container_root, uint32_t type, uint16_t length, void *data)
{
    CHECK(type != 0);
    CHECK(length != 0);
    CHECK(data != NULL);

    tlv_t *tlv = NULL;
    tlv_container_t *container = NULL;
    /* 1. 单级TLV处理，直接挂载 */
    if (tlv_tag_get_count(type) == 1) {
        LOGD(">>>>>>>>> single tlv");
        if (*container_root == NULL) {
            LOGI("create tlv container @single.");
            *container_root = tlv_container_create();
        }
        tlv_package(&tlv, tlv_tag_get_low(type), length, data);
        tlv_container_push(*container_root, tlv);
        tlv_destroy(&tlv);
        return 0;
    }

    /* 2. 多级TLV处理 */
    if (*container_root != NULL) {
        tlv_group_t *tlv_group = (tlv_group_t *)TLV_MALLOC(sizeof(tlv_group_t) + sizeof(tlv_t));
        ASSERT(tlv_group != NULL);
        memset(tlv_group, 0, sizeof(tlv_group_t) + sizeof(tlv_t));
        tlv_group->data->tag = tlv_tag_get_msb(type);
        container = *container_root;
        if (tlv_list_foreach(&container->tlv_group->list, find_duplicate_key, tlv_group) == 0) {
#if MLA_ENABLE
            // 为了验证MLA
            TLV_FREE(tlv_group);
            tlv_group = NULL;
#else
            tlv_destroy(&tlv_group);
#endif
            goto TLV_COMPLEX_NEW;
        }

        // -*- 2.1 要挂载的节点tag，根container链表中已存在，故需取下tag对应的数据后合入要挂载的节点，序列化后重新挂载
        LOGD(">>>>>>>>> complex exist tlv");
        tlv_container_t *container_mount = tlv_container_create();
        tlv_container_t **container_next = &container_mount->container;

        /* 以顶级Tag为基准逐级取下节点数据，组合到各级container
           第1步：取下顶级Tag命中的根container上的TLV节点，拆解一级TLV结构到container->group，
           第2步，遍历第1步拆解出的一级TLV链表，以次级Tag继续拆解二级TLV结构到new-container->group，
           第3步，重复第2步，直至拆解到最底级TLV，当前TLV级数最大为type类型即uint32_t(4) */
        for (;;) {
            if ((container != NULL) && tlv_list_foreach(&container->tlv_group->list, find_duplicate_key, tlv_group) != 0) {
                // 记录在根节点的挂接位置
                if (container == *container_root) {
                    memcpy(&container_mount->tlv_group->list, &tlv_group->list, sizeof(tlv_group_t));
                    LOGD("record hit list, node next: %p, prev: %p", tlv_list_next_get(&container_mount->tlv_group->list),
                        tlv_list_prev_get(&container_mount->tlv_group->list));
                }
                // 取下与本次挂载节点tag相同的链表中数据，并拆解到tlv级别
                tlv = (tlv_t *)TLV_MALLOC(sizeof(tlv_t) +
                    ((tlv_group_t *)tlv_list_next_get(tlv_list_prev_get(&tlv_group->list)))->data->length);
                memcpy(tlv, ((tlv_group_t *)tlv_list_next_get(tlv_list_prev_get(&tlv_group->list)))->data,
                    sizeof(tlv_t) + ((tlv_group_t *)tlv_list_next_get(tlv_list_prev_get(&tlv_group->list)))->data->length);
                tlv_container_pop(container_next, tlv);
                tlv_destroy(&tlv);
                LOGD(">>>>>>>>> find tlv.");
                // 记录tlv嵌套层数
                container_mount->tlv_nested_count++;
                tlv_list_foreach(&(*container_next)->tlv_group->list, tlv_all_printf, NULL);
            } else {
#if MLA_ENABLE
                TLV_FREE(tlv_group);
                tlv_group = NULL;
#else
                tlv_destroy(&tlv_group);
#endif
                LOGD("end of tag search. disassembled layers number: %u", container_mount->tlv_nested_count);
                break;
            }
            container = *container_next;
            container_next = &(*container_next)->container;
            tlv_group->data->tag = tlv_tag_get_next_byte(type, tlv_group->data->tag);
            LOGD("tlv_group->data->tag: %#x", tlv_group->data->tag);
            // 已经拆解到最底级Tag，则进行同Tag tlv节点检查
            if (tlv_group->data->tag == tlv_tag_get_low(type)) {
                if (!!tlv_list_foreach(&container->tlv_group->list, find_duplicate_key, tlv_group)) {
                    container_mount->tlv_nested_count++;
                    LOGW("Duplicate tag node data exists in the tlv container.");
                } else {
                    LOGW("The dismantling TLV has reached its lowest level.");
                };
#if MLA_ENABLE
                TLV_FREE(tlv_group);
                tlv_group = NULL;
#else
                tlv_destroy(&tlv_group);
#endif
                LOGD("disassembled layers number: %u", container_mount->tlv_nested_count);
                break;
            }
        }

        // 序列化container数据与顶级tag进行组包后重挂载到根container链表
        container_next = &container_mount->container;
        if (*container_next != NULL) {
            // 挂载新增的tlv数据到最底级的container上
            tlv_package(&tlv, tlv_tag_get_low(type), length, data);
            // 要挂载的tlv数据与挂载点tag级别一致，方可正确挂载（tlv级别要平等）
            uint8_t tlv_nested_count_max = tlv_nested_number_get(type) -1;
            if (container_mount->tlv_nested_count < tlv_nested_count_max) {
                LOGD("process original tlv data.");
                uint8_t process_times = tlv_nested_count_max - container_mount->tlv_nested_count;
                while (process_times) {
                    container = tlv_container_create();
                    tlv_container_push(container, tlv);
                    if (tlv_container_serialize(container) != 0) {
                        LOGE("tlv container serialize fail");
                    }
                    tlv_destroy(&tlv);
                    tlv_nested_package(&tlv, tlv_tag_get_byte(type, tlv_nested_count_max - process_times--), container);
                    tlv_container_destroy(&container);
                }
            } else if (container_mount->tlv_nested_count == tlv_nested_count_max +1) {
                LOGW("Multilevel tlv processing warning. It means that the current tlv node has the same tag on the container!");
            } else if (container_mount->tlv_nested_count > tlv_nested_count_max +1) {
                LOGE("Multilevel tlv processing error. tlv disassembles: %u, type number: %u",
                    container_mount->tlv_nested_count, tlv_nested_number_get(type));
                return -2;
            }
            tlv_container_push(*tlv_container_last(&container_mount), tlv);
            tlv_list_foreach(&(*tlv_container_last(&container_mount))->tlv_group->list, tlv_all_printf, NULL);
            LOGD(">>>>>>>>> combine@0 tlv.");
            tlv_destroy(&tlv);
            // 孙container存在则逐级序列化container
            while (tlv_container_next(tlv_container_next(container_mount)) != NULL) {
                tlv_container_t **container_last = tlv_container_last(&container_mount);
                if (tlv_container_serialize(*container_last) != 0) {
                    LOGE("tlv container serialize fail");
                }
                tlv_nested_package(&tlv, ((tlv_group_t *)tlv_list_next_get(&tlv_container_prev(container_mount, *container_last)->tlv_group->list))->data->tag, *container_last);
                tlv_printf(tlv);
                LOGD("&container_last: %x", *container_last);
                tlv_container_destroy(container_last);
                LOGD("&tlv_container_last(container_mount): %x", *tlv_container_last(&container_mount));
                tlv_container_fix_push(*tlv_container_last(&container_mount), tlv);
                tlv_list_foreach(&(*tlv_container_last(&container_mount))->tlv_group->list, tlv_all_printf, NULL);
                LOGD(">>>>>>>>> combine@1 tlv.");
                tlv_destroy(&tlv);
            }
            // 子container序列化完成后挂载整个节点数据
            if (tlv_container_serialize(*container_next) != 0) {
                LOGE("tlv container serialize fail");
            }
            tlv_group_mount(*container_root, container_mount, tlv_tag_get_msb(type));
            LOGD("tlv destroy: container_next@ %p, container_mount@ %p", container_next, container_mount);
            tlv_container_destroy(container_next);
            tlv_container_destroy(&container_mount);
            return 0;
        } else {
            LOGE("An error was encountered disassembling tlv data on the linked list.");
            return -3;
        }
    }

TLV_COMPLEX_NEW:
    // -*- 2.2 新TLV节点，组包底级TLV
    LOGD(">>>>>>>>> complex new tlv");
    container = tlv_container_create();
    tlv_package(&tlv, tlv_tag_get_low(type), length, data);
    tlv_container_push(container, tlv);
    tlv_destroy(&tlv);
    uint8_t package_index = 1;  // 记录处理TLV起始深度

    // 通过检查Type上个字节来决定当前字节如何处理
    for (uint8_t i = package_index; i < sizeof(uint32_t);) {
        if (!!tlv_tag_get_byte(type, i+1)) {
            tlv_container_t *container_parent = tlv_container_create();
            if (tlv_container_serialize(container) != 0) {
                LOGD("tlv container serialize fail");
            }
            LOGD("package_index: %u, tag: %02x", i, tlv_tag_get_byte(type, i));
            tlv_nested_package(&tlv, tlv_tag_get_byte(type, i), container);
            tlv_container_push(container_parent, tlv);
            tlv_destroy(&tlv);
            tlv_container_destroy(&container);
            container = container_parent;
            package_index = ++i;
            continue;
        }
        break;
    }

    // -*- 挂载顶级TLV到根container
    if (tlv_container_serialize(container) != 0) {
        LOGD("tlv container serialize fail");
    }
    if (*container_root == NULL) {
        LOGI("create tlv container @complex.");
        *container_root = tlv_container_create();
    }
    LOGD("package_index: %u, tag: %02x", package_index, tlv_tag_get_byte(type, package_index));
    tlv_nested_package(&tlv, tlv_tag_get_byte(type, package_index), container);
    tlv_container_push(*container_root, tlv);
    tlv_destroy(&tlv);
    tlv_container_destroy(&container);
    return 0;
}
