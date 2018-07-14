/*
 * Copyright (c) 2009-2012, Pieter Noordhuis <pcnoordhuis at gmail dot com>
 * Copyright (c) 2009-2012, Salvatore Sanfilippo <antirez at gmail dot com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   * Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *   * Neither the name of Redis nor the names of its contributors may be used
 *     to endorse or promote products derived from this software without
 *     specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __INTSET_H
#define __INTSET_H
#include <stdint.h>
/**
 * 整型数据结构体
 */
typedef struct intset {
    /*
     * 数据编码，表示 intset 中的每个元素用几个字节来存储
     * INTSET_ENC_INT16 表示每个元素用2个字节存储
     * INTSET_ENC_INT32 表示每个元素用4个字节存储
     * INTSET_ENC_INT64 表示每个元素用8个字节存储
     * 因此，intset中存储的整数最多只能占用64bit。
     */
    uint32_t encoding;
    // 集合长度
    uint32_t length;
    /*
     * 用来存储集合的容器，是一个柔性数组（flexible array member）
     *
     * 比起指针用空数组有这样的优势：
     * 1. 不需要初始化，数组名直接就是缓冲区数据的起始地址(如果存在数据)
     * 2. 不占任何空间，指针需要占用4 byte长度空间，空数组不占任何空间，节约了空间
     */
    int8_t contents[];
} intset;

// 创建一个 intset 集合
intset *intsetNew(void);
// 往集合里面添加元素
intset *intsetAdd(intset *is, int64_t value, uint8_t *success);
// 删除一个元素
intset *intsetRemove(intset *is, int64_t value, int *success);
// 查找元素
uint8_t intsetFind(intset *is, int64_t value);
int64_t intsetRandom(intset *is);
uint8_t intsetGet(intset *is, uint32_t pos, int64_t *value);
// 获取集合长度
uint32_t intsetLen(const intset *is);
size_t intsetBlobLen(intset *is);

#ifdef REDIS_TEST
int intsetTest(int argc, char *argv[]);
#endif

#endif // __INTSET_H
