#!/usr/bin/bash
# Copyright © 2023 <copyright skull.gu@gmail.com>
#
# Permission is hereby granted, free of charge, to any person obtaining a copy of this software
# and associated documentation files (the “Software”), to deal in the Software without
# restriction, including without limitation the rights to use, copy, modify, merge, publish,
# distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the
# Software is furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all copies or
# substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
# BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
# NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
# DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

function help {
cat <<EOF
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
EOF
}

function clean {
    ./do.sh mla OFF
    [ -f mla.c ] && rm mla.c
    [ -f mla.h ] && rm mla.h
    [ -f slist.c ] && rm slist.c
    [ -f slist.h ] && rm slist.h
    [ -f a.out ] && rm a.out
    [ -f build.log ] && rm build.log
    [ -f out.log ] && rm out.log
}

function mla {
    [ ! $1 ] && echo -e "\e[47m\e[31m!!Please enter option parameters\e[0m" && exit -1
    [ $1 = 'ON' ] && {
        [ ! -f mla.c ] && echo "Download the file from github: https://github.com/skullboyer/MLA" && exit -1
        if ! grep -q "#undef MLA_ENABLE" adapter.h; then
            sed -i '/#define DEBUG/a\#undef MLA_ENABLE\
#define MLA_ENABLE    1\n\
#if MLA_ENABLE\
#include "slist.h"\
#define mla_list_init                slist_init\
#define mla_slist_foreach            slist_foreach\
#define mla_list_add                 slist_add\
#define mla_list_add_tail            slist_add_tail\
#define mla_list_next_get            slist_next_get\
#define mla_list_prev_get            slist_prev_get\
#define mla_list_del                 slist_del\
#define mla_list_node_count_get      slist_node_count_get\
typedef slist_node_t    mla_list_node_t;\
#endif' adapter.h
        fi
        if ! grep -q "#include \"mla.h\"" adapter.h; then
            sed -i '/#define TLV_FREE(addr)/a\#if MLA_ENABLE\n#include "mla.h"' adapter.h
            sed -i '/#define TLV_MALLOC(size)/d' adapter.h
            sed -i '/free(addr)/d' adapter.h
        fi
        if ! grep -q "#define TLV_MALLOC(size)" adapter.h; then
            sed -i '/#include "mla.h"/a\#define TLV_MALLOC(size)    PORT_MALLOC(size)\
#define TLV_FREE(addr)      PORT_FREE(addr)\
#else\
#define TLV_MALLOC(size)    malloc(size)\
#define TLV_FREE(addr)      free(addr)\
static int MlaInit(void) {}\
static int MlaOutput(void) {}\
#endif' adapter.h
        fi
        if ! grep -q "MlaInit();" test.c; then
            sed -i '/uint8_t \*tlv_packet/a\    MlaInit();' test.c
            sed -i '/tlv_consumer(&tlv_packet)/a\    MlaOutput();'  test.c
        fi
    } || {
        [ $1 = 'OFF' ] && {
            [ ! -f mla.c ] && exit -1
            sed -i  '/#undef MLA_ENABLE/,/#endif/d' adapter.h
            sed -i  '/#if MLA_ENABLE/,/#endif/d' adapter.h
            if ! grep -q "#define TLV_MALLOC(size)" adapter.h; then
                sed -i '/#endif/a\#define TLV_MALLOC(size)    malloc(size)\n#define TLV_FREE(addr)      free(addr)' adapter.h
            fi
            sed -i '/MlaInit();/d' test.c
            sed -i '/MlaOutput();/d' test.c
            [ -f mla.c ] && rm mla.c
            [ -f mla.h ] && rm mla.h
            [ -f slist.c ] && rm slist.c
            [ -f slist.h ] && rm slist.h
        }
    }
    sed -i 's/\r$//' adapter.h
    sed -i '/^$/N;/\n$/D' adapter.h
}

function process {
    case ${user_arg[0]} in
        mla)
            mla ${user_arg[1]}
            ;;
        make)
            gcc *.c
            gcc *.c 2>&1 |grep -e error: -e warning >build.log
            grep -q error: build.log && echo -e "\033[1;31m> Build Error!\033[0m" && grep -e error: build.log
            ;;
        exec)
            [ ! -f a.out ] && echo "!!Run the command './do.sh make'" && exit -1
            ./a.out ${user_arg[1]} |tee out.log
            ;;
        clean)
            clean
            ;;
        *)
            help
            ;;
    esac
}

user_arg=(${@:1})
process
