/* A simple event-driven programming library. Originally I wrote this code
 * for the Jim's event-loop (Jim is a Tcl interpreter) but later translated
 * it in form of a library for easy reuse.
 *
 * Copyright (c) 2006-2012, Salvatore Sanfilippo <antirez at gmail dot com>
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

#ifndef __AE_H__
#define __AE_H__

#include <time.h>

#define AE_OK 0
#define AE_ERR -1

#define AE_NONE 0       /* No events registered. */
#define AE_READABLE 1   /* Fire when descriptor is readable. */
#define AE_WRITABLE 2   /* Fire when descriptor is writable. */
#define AE_BARRIER 4    /* With WRITABLE, never fire the event if the
                           READABLE event already fired in the same event
                           loop iteration. Useful when you want to persist
                           things to disk before sending replies, and want
                           to do that in a group fashion. */

#define AE_FILE_EVENTS 1
#define AE_TIME_EVENTS 2
#define AE_ALL_EVENTS (AE_FILE_EVENTS|AE_TIME_EVENTS)
#define AE_DONT_WAIT 4
#define AE_CALL_AFTER_SLEEP 8

#define AE_NOMORE -1
#define AE_DELETED_EVENT_ID -1

/* Macros */
#define AE_NOTUSED(V) ((void) V)

struct aeEventLoop;

/* Types and data structures */
typedef void aeFileProc(struct aeEventLoop *eventLoop, int fd, void *clientData, int mask);
typedef int aeTimeProc(struct aeEventLoop *eventLoop, long long id, void *clientData);
typedef void aeEventFinalizerProc(struct aeEventLoop *eventLoop, void *clientData);
typedef void aeBeforeSleepProc(struct aeEventLoop *eventLoop);

/* 文件事件结构体 */
typedef struct aeFileEvent {
    /* 文件事件的类型：READABLE|WRITABLE */
    int mask; /* one of AE_(READABLE|WRITABLE|BARRIER) */
    /*
     * 函数指针(回调函数)
     * 文件事件读处理器的函数指针，当前事件是读事件的时候就会调用该方法
     */
    aeFileProc *rfileProc;
    /*
     * 函数指针(回调函数)
     * 文件事件写处理器，当前事件是写事件的时候就会调用该方法
     */
    aeFileProc *wfileProc;
    // 客户端数据
    void *clientData;
} aeFileEvent;

/* 时间事件结构体 */
typedef struct aeTimeEvent {
    long long id; /* 时间事件id */
    long when_sec; //时间事件到达的时间，精度为秒
    long when_ms; //时间事件到达的时间，精度为毫秒
    /*
     * 函数指针(回调函数)
     * 时间处理器
     */
    aeTimeProc *timeProc;
    //处理完时间事件的收尾方法
    aeEventFinalizerProc *finalizerProc;
    // client 数据
    void *clientData;
    // 前置指针，指向前面的时间事件
    struct aeTimeEvent *prev;
    // 后置指针，指向后一个时间事件
    struct aeTimeEvent *next;
} aeTimeEvent;

/* A fired event */
typedef struct aeFiredEvent {
    int fd;
    int mask;
} aeFiredEvent;

/* State of an event based program */

/*
 * 事件循环，这不仅是AE，也是Redis最重要的一个部分。它是一个结构体，里面保存着事件循环总体的信息，
 * 比如：时间事件的链表头，最大的文件描述符，sleep前要执行的任务函数等。
 */
typedef struct aeEventLoop {
    /* 当前注册的文件事件的最大文件描述符 */
    int maxfd;   /* highest file descriptor currently registered */
    /* 注册文件事件的文件描述符的最大限制 */
    int setsize; /* max number of file descriptors tracked */
    // 下一个时间事件id
    long long timeEventNextId;
    time_t lastTime;     /* Used to detect system clock skew */
    // 注册的文件事件
    aeFileEvent *events; /* Registered events */
    // 注册的 fired 事件
    aeFiredEvent *fired; /* Fired events */
    // 注册的时间事件，这里是一个 链表
    aeTimeEvent *timeEventHead;
    int stop;
    void *apidata; /* This is used for polling API specific data */
    aeBeforeSleepProc *beforesleep;
    aeBeforeSleepProc *aftersleep;
} aeEventLoop;

/* Prototypes */
/**
 * 创建一个事件
 * @param setsize
 * @return
 */
aeEventLoop *aeCreateEventLoop(int setsize);
/**
 * 删除一个事件
 * @param eventLoop
 */
void aeDeleteEventLoop(aeEventLoop *eventLoop);
/**
 * 停止一个事件
 * @param eventLoop
 */
void aeStop(aeEventLoop *eventLoop);
/**
 * 创建一个文件事件
 */
int aeCreateFileEvent(aeEventLoop *eventLoop, int fd, int mask,
        aeFileProc *proc, void *clientData);
/**
 * 删除一个文件事件
 *
 * @param eventLoop
 * @param fd
 * @param mask
 */
void aeDeleteFileEvent(aeEventLoop *eventLoop, int fd, int mask);
/**
 * 获取一个文件事件
 * @param eventLoop
 * @param fd
 * @return
 */
int aeGetFileEvents(aeEventLoop *eventLoop, int fd);
/**
 * 创建一个时间事件
 */
long long aeCreateTimeEvent(aeEventLoop *eventLoop, long long milliseconds,
        aeTimeProc *proc, void *clientData,
        aeEventFinalizerProc *finalizerProc);
int aeDeleteTimeEvent(aeEventLoop *eventLoop, long long id);
/**
 * 执行一个事件
 */
int aeProcessEvents(aeEventLoop *eventLoop, int flags);
int aeWait(int fd, int mask, long long milliseconds);
/** aeMain是AE的启动函数，也是AE的事件循环,主程序的循环也起始于这个函数：*/
void aeMain(aeEventLoop *eventLoop);
char *aeGetApiName(void);
void aeSetBeforeSleepProc(aeEventLoop *eventLoop, aeBeforeSleepProc *beforesleep);
void aeSetAfterSleepProc(aeEventLoop *eventLoop, aeBeforeSleepProc *aftersleep);
int aeGetSetSize(aeEventLoop *eventLoop);
int aeResizeSetSize(aeEventLoop *eventLoop, int setsize);

#endif
