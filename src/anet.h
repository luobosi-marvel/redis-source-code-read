/* anet.c -- Basic TCP socket stuff made a bit less boring
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

#ifndef ANET_H
#define ANET_H

#include <sys/types.h>

#define ANET_OK 0
#define ANET_ERR -1
#define ANET_ERR_LEN 256

/* Flags used with certain functions. */
#define ANET_NONE 0
#define ANET_IP_ONLY (1<<0)

#if defined(__sun) || defined(_AIX)
#define AF_LOCAL AF_UNIX
#endif

#ifdef _AIX
#undef ip_len
#endif
/* TCP的默认连接 */
int anetTcpConnect(char *err, char *addr, int port);
/* TCP的非阻塞连接 */
int anetTcpNonBlockConnect(char *err, char *addr, int port);

int anetTcpNonBlockBindConnect(char *err, char *addr, int port, char *source_addr);

int anetTcpNonBlockBestEffortBindConnect(char *err, char *addr, int port, char *source_addr);
/* anet的Unix方式的默认连接方式 */
int anetUnixConnect(char *err, char *path);
/* anet的Unix方式的非阻塞连接方式 */
int anetUnixNonBlockConnect(char *err, char *path);
/* anet网络读取文件到buffer中操作 */
int anetRead(int fd, char *buf, int count);
/* 解析所有的东西 */
int anetResolve(char *err, char *host, char *ipbuf, size_t ipbuf_len);
/* 单单解析IP的地址 */
int anetResolveIP(char *err, char *host, char *ipbuf, size_t ipbuf_len);

int anetTcpServer(char *err, int port, char *bindaddr, int backlog);

int anetTcp6Server(char *err, int port, char *bindaddr, int backlog);

int anetUnixServer(char *err, char *path, mode_t perm, int backlog);

int anetTcpAccept(char *err, int serversock, char *ip, size_t ip_len, int *port);

int anetUnixAccept(char *err, int serversock);
/* anet通过网络从buffer中写入文件操作 */
int anetWrite(int fd, char *buf, int count);
/* anet设置非阻塞的方法 */
int anetNonBlock(char *err, int fd);

int anetBlock(char *err, int fd);
/* 启用TCP没有延迟 */
int anetEnableTcpNoDelay(char *err, int fd);
/* 禁用TCP连接没有延迟 */
int anetDisableTcpNoDelay(char *err, int fd);
/* 设置TCP保持活跃连接状态。适用于所有系统 */
int anetTcpKeepAlive(char *err, int fd);

int anetSendTimeout(char *err, int fd, long long ms);

int anetPeerToString(int fd, char *ip, size_t ip_len, int *port);
/* 设置TCP连接一直存活，用来检测已经死去的结点，interval选项只适用于Linux下的系统 */
int anetKeepAlive(char *err, int fd, int interval);

int anetSockName(int fd, char *ip, size_t ip_len, int *port);

int anetFormatAddr(char *fmt, size_t fmt_len, char *ip, int port);

int anetFormatPeer(int fd, char *fmt, size_t fmt_len);

int anetFormatSock(int fd, char *fmt, size_t fmt_len);

#endif
