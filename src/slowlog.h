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
// slowlog 最大参数个数, 超过也只会记录 最大值个数
#define SLOWLOG_ENTRY_MAX_ARGC 32
// slowlog 记录最大的字符串长度, 超过也只会记录 最大值个数
#define SLOWLOG_ENTRY_MAX_STRING 128

/**
 * todo: 慢查询日志就是系统在命令执行前后计算每条命令的执行时间, 先进先出的队列
 * 当超过预设阀值，就将这条命令的相关信息（慢查询ID，发生时间戳，耗时，命令的详细信息）
 * 记录下来。
 *
 * 通过 slowlog get 命令来获取慢查询日志
 */
typedef struct slowlogEntry {
    /*
     * 存储命令以及命令参数，例如 config set slowlog-max-len 10
     * 那么 argv 指向的就是 "config" "set" "slowlog-max-len" "10"
     *
     * 参数的长度受 SLOWLOG_ENTRY_MAX_STRING 限制
     */
    robj **argv;
    /*
     * 命令与命令参数的数量
     * 参数个数受 SLOWLOG_ENTRY_MAX_ARGC 限制
     */
    int argc;
    // 唯一 id
    long long id;       /* Unique entry identifier. */
    // 命令执行时间的时长，以微秒计算
    long long duration; /* Time spent by the query, in microseconds. */
    // 命令执行时的 Unix 时间戳
    time_t time;        /* Unix time at which the query was executed. */
    // client 的名字
    sds cname;          /* Client name. */
    // client 的地址
    sds peerid;         /* Client network address. */
} slowlogEntry;

/* Exported API */
void slowlogInit(void);
void slowlogPushEntryIfNeeded(client *c, robj **argv, int argc, long long duration);

/* Exported commands */
void slowlogCommand(client *c);
