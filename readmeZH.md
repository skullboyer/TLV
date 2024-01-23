
介绍
---
TLV节点说明，tag字段高位表示有无嵌套
```
+-------------------------------------------------------------------------------------------+
|  Single     |  Multi                                                                      |
+-------------------------------------------------------------------------------------------+
|  TLV        |  TL TLV TLV                     |  TL TL TLV TLV                            |
|-----------------------------------------------|-------------------------------------------|
|  0xxx_xxxx  |  1xxx_xxxx 0xxx_xxxx 0xxx_xxxx  |  1xxx_xxxx 1xxx_xxxx 0xxx_xxxx 0xxx_xxxx  |
+-------------------------------------------------------------------------------------------+
```
使用链表来管理tlv节点
```
=>create                     =>add                     =>add                     =>add
+-------------------+        +----------------+        +----------------+        +----------------+
|  +-------------+  |        |  +----------+  |        |  +----------+  |        |  +----------+  |
|  |    head     |  |  <-->  |  |   list   |  |  <-->  |  |   list   |  |  <-->  |  |   list   |  |
|  |-------------|  |        |  |   data   |  |        |  |   data   |  |        |  |   data   |  |
|  |     /\      |  |        |  +----------+  |        |  +----------+  |        |  +----------+  |
|  |  tlv_group  |  |        |       /\       |        |       /\       |        |       /\       |
|  +-------------+  |        |    tlv_group1  |        |   tlv_group2   |        |   tlv_groupx   |
|        /\         |        +----------------+        +----------------+        +----------------+
|     container     |
+-------------------+
```
序列化，即二进制字节流
```
+-------------------------------------------------+
|   TLVTLVTLVTLTLVTLVTLTLTLVTLVTLTLVTLVTLTLV      |
+-------------------------------------------------+
|   1010101010000101111010010100111000100001'b    |
+-------------------------------------------------+
```

快速开始
---
你可以使用本项目提供的脚本 `do.sh` 来快速的使用本项目<br />
>可以使用帮助命令方便查看 `./do.sh help`<br />
```bash
-*- help -*-
usage: ./do.sh [make] [exec] [mla] [clean] [help]

Example usage of the TLV mechanism
$ ./do.sh make

Check the TLV code for memory leaks
$ ./do.sh mla ON/OFF
$ ./do.sh make

Execute the program to view the results
$ ./do.sh exec (manual/smart)

Remove unnecessary code
$ ./do.sh clean
```
>可以使用编译、执行命令来快速熟悉本项目的用法<br />
```bash
$ ./do.sh make
$ ./do.sh exec manual/smart
```

>可以使用MLA命令来检查本项目内存泄漏情况（详见：https://github.com/skullboyer/MLA）<br />
```bash
$ ./do.sh mla ON/OFF
$ ./do.sh make
$ ./do.sh exec manual/smart
```

用法
---
在使用上分为手动与智能，区别在时间与空间的平衡，以及使用者的便利性，通过处理下述tlv节点来演示两种方式的使用规则
```
+-------------------------+
|  TLV  || TLV ||   TLV   |
|-------------------------|
|  TLV      @      skull  |
+-------------------------+
+-------------------------------------------------+
|  TL | TLV                               || TLV  |
|-------------------------------------------------|
|       Half a loaf is better than no bread.  天  |
+-------------------------------------------------+
+------------------------------------------------------------ ----------+
|  TL || TL | TLV         | TLV    || TL | TLV                  || TLV  |
|-----------------------------------------------------------------------|
|             All work and  no play        makes Jack a dull boy.  灵   |
+-----------------------------------------------------------------------+
+------------+
|  TL | TLV  |
|------------|
|       盖   |
+------------+
```

#### 手动（执行顺序即为序列化结果序）：
```c
// test.c
int8_t tlv_producer_manual(uint8_t **data)
{
    MAIN_ASSERT(*data == NULL);

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

    // -*- multi: TL TLV TLV -*-
    tlv_container_t *container = tlv_container_create();
    string = "Half a loaf is better than no bread.";
    tlv_package(&tlv, tlv_tag_get_low(TEST_TAGa1), strlen(string), string);
    tlv_container_push(container, tlv);
    tlv_destroy(&tlv);

    uint32_t value = 0xA9A4E5;
    tlv_package(&tlv, tlv_tag_get_low(TEST_TAGa2), sizeof(value), &value);
    tlv_container_push(container, tlv);
    tlv_destroy(&tlv);

    if (tlv_container_serialize(container) != 0) {
        LOGD("tlv container serialize fail");
    }
    tlv_nested_package(&tlv, tlv_tag_get_high(TEST_TAGa0), container);
    tlv_container_push(container_root, tlv);
    tlv_destroy(&tlv);
    tlv_container_destroy(&container);

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

    tlv_container_t *container1 = tlv_container_create();
    if (tlv_container_serialize(container) != 0) {
        LOGD("tlv container serialize fail");
    }
    tlv_nested_package(&tlv, tlv_tag_get_high(TEST_TAGa0), container);
    tlv_container_push(container1, tlv);
    tlv_destroy(&tlv);
    tlv_container_destroy(&container);

    container = tlv_container_create();
    string = "makes Jack a dull boy.";
    tlv_package(&tlv, tlv_tag_get_low(TEST_TAGcb1), strlen(string), string);
    tlv_container_push(container, tlv);
    tlv_destroy(&tlv);

    if (tlv_container_serialize(container) != 0) {
        LOGD("tlv container serialize fail");
    }
    tlv_nested_package(&tlv, tlv_tag_get_high(TEST_TAGb0), container);
    tlv_container_push(container1, tlv);
    tlv_destroy(&tlv);
    tlv_container_destroy(&container);

    value = 0xB581E7;
    tlv_package(&tlv, tlv_tag_get_low(TEST_TAGc1), sizeof(value), &value);
    tlv_container_push(container1, tlv);
    tlv_destroy(&tlv);

    if (tlv_container_serialize(container1) != 0) {
        LOGD("tlv container serialize fail");
    }
    tlv_nested_package(&tlv, tlv_tag_get_high(TEST_TAGc00), container1);
    tlv_container_push(container_root, tlv);
    tlv_destroy(&tlv);
    tlv_container_destroy(&container1);

    // -*- multi: TL TLV -*-
    container = tlv_container_create();
    value = 0x969BE7;
    tlv_package(&tlv, tlv_tag_get_low(TEST_TAGd1), sizeof(value), &value);
    tlv_container_push(container, tlv);
    tlv_destroy(&tlv);

    if (tlv_container_serialize(container) != 0) {
        LOGD("tlv container serialize fail");
    }
    tlv_nested_package(&tlv, tlv_tag_get_high(TEST_TAGd0), container);
    tlv_container_push(container_root, tlv);
    tlv_destroy(&tlv);
    tlv_container_destroy(&container);

    // -*- serialize -*-
    if (tlv_container_serialize(container_root) != 0) {
        LOGD("tlv container_root serialize fail");
    }
```

#### 智能（序列化结果序取决于Tag定义，与执行顺序无关）：
```c
// test.c
int8_t tlv_producer_smart(uint8_t **data)
{
    MAIN_ASSERT(*data == NULL);

    tlv_container_t *container_root = NULL;

    char *string = "TLV";
    tlv_container_handle(&container_root, TEST_TAG01, strlen(string), string);

    string = "@";
    tlv_container_handle(&container_root, TEST_TAG02, strlen(string), string);

    string = "skull";
    tlv_container_handle(&container_root, TEST_TAG03, strlen(string), string);

    string = "Half a loaf is better than no bread.";
    tlv_container_handle(&container_root, TEST_TAGa1, strlen(string), string);

    string = "All work and";
    tlv_container_handle(&container_root, TEST_TAGca1, strlen(string), string);

    string = "no play";
    tlv_container_handle(&container_root, TEST_TAGca2, strlen(string), string);

    uint32_t value = 0xA9A4E5;
    tlv_container_handle(&container_root, TEST_TAGa2, sizeof(value), &value);

    string = "makes Jack a dull boy.";
    tlv_container_handle(&container_root, TEST_TAGcb1, strlen(string), string);

    value = 0xB581E7;
    tlv_container_handle(&container_root, TEST_TAGd1, sizeof(value), &value);

    value = 0x969BE7;
    tlv_container_handle(&container_root, TEST_TAGc1, sizeof(value), &value);

    // -*- serialize -*-
    if (tlv_container_serialize(container_root) != 0) {
        LOGD("tlv container_root serialize fail");
    }
```

设计
---
新增tlv节点在tlv-container中存在相同tag的处理逻辑
```
             root                                             root
            .----------------------.--------.                .-----------------------.--------.
            | T1| T2| ... | Tn| Tm|         |      =>        | T1| T2| ... | Tn'| Tm|         |
            | L | L | ... | L | L |->       |                | L | L | ... | L  | L |->       |
            | V | V | ... | V | V |         |                | V | V | ... | V  | V |         |
            '----------------------'--------'                '-----------------------'--------'
                            |                                                ^
          .---->-----------' `--.                                            | remount
         /     check             \ pick off                                  |
        ^                         v                                          |
      .---.                     .---.                                      .----.
      | T |                     | Tn|                                      | Tn'|
      | L |                     | L |                                      | L  |
      | V |                     | V |                                      | V  |
      '---'                     '---'                                      '----'
       new                        |                                          ^
                               .-' disassembly                               | assembly
                              v                                              |
                         .--------.----.     new   .---.  group   .------------.----.
                         | T1| T2| ... |      +    | T |   =>     | T1| T2| Tx| ... |
                         | L | L | ... |           | L |          | L | L | L | ... |
                         | V | V | ... |           | V |          | V | V | V | ... |
                         '--------'----'           '---'          '------------'----'
```

示例
---
示例`test.c`中演示了一种典型使用场景
```
  +------------+       +-----------------------+   transfer   +-------------------------+     +------------+
  | TLV encode | ==>   | data package and send | ===========> | data receive and verify | ==> | TLV decode |
  +------------+       +-----------------------+              +-------------------------+     +------------+
```

共同进步
---
欢迎大家使用并`issue`反馈
