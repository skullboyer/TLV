
Introduction of TLV
---
Description of the TLV node. The high value of the tag field indicates whether there is nesting.
```
+-------------------------------------------------------------------------------------------+
|  Single     |  Multi                                                                      |
+-------------------------------------------------------------------------------------------+
|  TLV        |  TL TLV TLV                     |  TL TL TLV TLV                            |
|-----------------------------------------------|-------------------------------------------|
|  0xxx_xxxx  |  1xxx_xxxx 0xxx_xxxx 0xxx_xxxx  |  1xxx_xxxx 1xxx_xxxx 0xxx_xxxx 0xxx_xxxx  |
+-------------------------------------------------------------------------------------------+
```
Use linked lists to manage tlv nodes
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
Serialization, that is, binary byte stream
```
+-------------------------------------------------+
|   TLVTLVTLVTLTLVTLVTLTLTLVTLVTLTLVTLVTLTLV      |
+-------------------------------------------------+
|   1010101010000101111010010100111000100001'b    |
+-------------------------------------------------+
```

Quick Start
---
You can use the script `do.sh` provided to quickly use this code base<br />
>You can use the `./do.sh help` command<br />
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

>You can use compile, execute commands to quickly familiarize yourself with the usage of this project<br />
```bash
$ ./do.sh make
$ ./do.sh exec manual/smart
```

>You can use the MLA command to check for memory leaks in this project (details: https://github.com/skullboyer/MLA)<br />
```bash
$ ./do.sh mla ON/OFF
$ ./do.sh make
$ ./do.sh exec manual/smart
```

How To Use
---
In the use of manual and smart, the difference in the balance of time and space, as well as the user's convenience, by processing the following tlv nodes to demonstrate the use of the two ways of rules
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

#### Manual (sequential):
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

#### Smart (random) :
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

Design
---
New tlv nodes The same tag processing logic exists in TLV-Container
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
Example
---
A typical usage scenario is demonstrated in `test.c`
```
  +------------+       +-----------------------+   transfer   +-------------------------+     +------------+
  | TLV encode | ==>   | data package and send | ===========> | data receive and verify | ==> | TLV decode |
  +------------+       +-----------------------+              +-------------------------+     +------------+
```

Work Together
---
`issue` Welcome to use and feedback