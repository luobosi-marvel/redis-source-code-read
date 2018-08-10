/*
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

#ifndef __REDIS_UTIL_H
#define __REDIS_UTIL_H

#include <stdint.h>
#include "sds.h"

/*
 * The maximum number of characters needed to represent a long double
 * as a string (long double has a huge range).
 * This should be the size of the buffer given to ld2string
 */
#define MAX_LONG_DOUBLE_CHARS 5*1024

int stringmatchlen(const char *p, int plen, const char *s, int slen, int nocase);
/**
 * 字符串匹配
 *
 * @param p
 * @param s
 * @param nocase
 * @return
 */
int stringmatch(const char *p, const char *s, int nocase);
long long memtoll(const char *p, int *err);
/**
 * unit64_t = 2^64 - 1
 * 求一个数字的长度
 *
 * @param v
 * @return
 */
uint32_t digits10(uint64_t v);
uint32_t sdigits10(int64_t v);
/**
 * 将 long long 类型转换成字符串
 *
 * @param s
 * @param len
 * @param value
 * @return
 */
int ll2string(char *s, size_t len, long long value);
/**
 * 将字符串装换成 long long 类型
 *
 * @param s         字符串
 * @param slen      字符串长度
 * @param value     long long value 指针
 * @return          length
 */
int string2ll(const char *s, size_t slen, long long *value);
/**
 * 把字符串转换成 long 类型的整数
 *
 * @param s         字符串
 * @param slen      字符串长度
 * @param value     long value 指针，转换之后的值要存里面
 * @return          length
 */
int string2l(const char *s, size_t slen, long *value);
int string2ld(const char *s, size_t slen, long double *dp);
int d2string(char *buf, size_t len, double value);
int ld2string(char *buf, size_t len, long double value, int humanfriendly);
/**
 * 获取文件的绝对路径
 *
 * @param filename 文件名称
 * @return 返回路径
 */
sds getAbsolutePath(char *filename);
int pathIsBaseName(char *path);

#ifdef REDIS_TEST
int utilTest(int argc, char **argv);
#endif

#endif
