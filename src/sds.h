#ifndef __SDS_H
#define __SDS_H

#define SDS_MAX_PREALLOC (1024*1024)
const char *SDS_NOINIT;

#include <sys/types.h>
#include <stdarg.h>
#include <stdint.h>

typedef char *sds;

// todo: sds.h 里面没有 sdshdr 结构了，已经被 下面五中类型替换了
/*
 * Note: sdshdr5 is never used, we just access the flags byte directly.
 * However is here to document the layout of type 5 SDS strings.
 *
 * sds一共有 5 中类型的 header。之所以有 5 种，是为了能让不同长度的字符串使用不同大小的 header。
 * 这样短字符串就能使用较小的 header，从而节省内存
 */

/**
 * 注意 sdshdr5 与其它几个 header 结构不同，它不包含 alloc 字段，而长度
 * 使用 flags 的高 5 位来存储。因此，它不能为字符串分配空余空间。
 * 如果字符串需要动态增长，那么它必然要重新分配内存才行。
 * 因此这种类型的 sds 字符串更适用于存储静态的短字符串(长度小于 32)
 */
struct __attribute__ ((__packed__)) sdshdr5 {
    // flags既是标记了头部类型，同时也记录了字符串的长度
    // 共8位，flags用前5位记录字符串长度（小于32=1<<5），后3位作为标志
    unsigned char flags; /* 3 lsb of type, and 5 msb of string length */
    // todo: C 语言数组必须指定长度，为甚这里不用？
    char buf[];
};
struct __attribute__ ((__packed__)) sdshdr8 {
    // 表示字符串的真正长度（不包含NULL结束符在内
    uint8_t len; /* used */
    // 表示字符串的最大容量（不包含最后多余的那个字节）。
    uint8_t alloc; /* excluding the header and null terminator */
    // C 中 char 类型占用一个字节。其中的最低3个bit用来表示header的类型。
    unsigned char flags; /* 3 lsb of type, 5 unused bits */
    char buf[];
};
struct __attribute__ ((__packed__)) sdshdr16 {
    uint16_t len; /* used */
    uint16_t alloc; /* excluding the header and null terminator */
    unsigned char flags; /* 3 lsb of type, 5 unused bits */
    char buf[];
};
struct __attribute__ ((__packed__)) sdshdr32 {
    uint32_t len; /* used */
    uint32_t alloc; /* excluding the header and null terminator */
    unsigned char flags; /* 3 lsb of type, 5 unused bits */
    char buf[];
};
struct __attribute__ ((__packed__)) sdshdr64 {
    uint64_t len; /* used */
    uint64_t alloc; /* excluding the header and null terminator */
    unsigned char flags; /* 3 lsb of type, 5 unused bits */
    char buf[];
};

#define SDS_TYPE_5  0
#define SDS_TYPE_8  1
#define SDS_TYPE_16 2
#define SDS_TYPE_32 3
#define SDS_TYPE_64 4
#define SDS_TYPE_MASK 7
#define SDS_TYPE_BITS 3
#define SDS_HDR_VAR(T, s) struct sdshdr##T *sh = (void*)((s)-(sizeof(struct sdshdr##T)));
#define SDS_HDR(T, s) ((struct sdshdr##T *)((s)-(sizeof(struct sdshdr##T))))
#define SDS_TYPE_5_LEN(f) ((f)>>SDS_TYPE_BITS)

/* 内联函数，计算 SDS 的长度 */
static inline size_t sdslen(const sds s) {
    unsigned char flags = s[-1];
    switch (flags & SDS_TYPE_MASK) {
        case SDS_TYPE_5:
            return SDS_TYPE_5_LEN(flags);
        case SDS_TYPE_8:
            return SDS_HDR(8, s)->len;
        case SDS_TYPE_16:
            return SDS_HDR(16, s)->len;
        case SDS_TYPE_32:
            return SDS_HDR(32, s)->len;
        case SDS_TYPE_64:
            return SDS_HDR(64, s)->len;
    }
    return 0;
}

/**
 * 获取 sds 中的可用空间
 *
 * @param s sds
 * @return  返回可用空间
 */
static inline size_t sdsavail(const sds s) {
    unsigned char flags = s[-1];
    switch (flags & SDS_TYPE_MASK) {
        case SDS_TYPE_5: {
            return 0;
        }
        case SDS_TYPE_8: {
            SDS_HDR_VAR(8, s);
            return sh->alloc - sh->len;
        }
        case SDS_TYPE_16: {
            SDS_HDR_VAR(16, s);
            return sh->alloc - sh->len;
        }
        case SDS_TYPE_32: {
            SDS_HDR_VAR(32, s);
            return sh->alloc - sh->len;
        }
        case SDS_TYPE_64: {
            SDS_HDR_VAR(64, s);
            return sh->alloc - sh->len;
        }
    }
    return 0;
}

/* 重新设置 SDS 的长度大小 */
static inline void sdssetlen(sds s, size_t newlen) {
    unsigned char flags = s[-1];
    switch (flags & SDS_TYPE_MASK) {
        case SDS_TYPE_5: {
            unsigned char *fp = ((unsigned char *) s) - 1;
            *fp = SDS_TYPE_5 | (newlen << SDS_TYPE_BITS);
        }
            break;
        case SDS_TYPE_8:
            SDS_HDR(8, s)->len = newlen;
            break;
        case SDS_TYPE_16:
            SDS_HDR(16, s)->len = newlen;
            break;
        case SDS_TYPE_32:
            SDS_HDR(32, s)->len = newlen;
            break;
        case SDS_TYPE_64:
            SDS_HDR(64, s)->len = newlen;
            break;
    }
}

/* 给 SDS 字符串 len = len + inc */
static inline void sdsinclen(sds s, size_t inc) {
    unsigned char flags = s[-1];
    switch (flags & SDS_TYPE_MASK) {
        case SDS_TYPE_5: {
            unsigned char *fp = ((unsigned char *) s) - 1;
            unsigned char newlen = SDS_TYPE_5_LEN(flags) + inc;
            *fp = SDS_TYPE_5 | (newlen << SDS_TYPE_BITS);
        }
            break;
        case SDS_TYPE_8:
            SDS_HDR(8, s)->len += inc;
            break;
        case SDS_TYPE_16:
            SDS_HDR(16, s)->len += inc;
            break;
        case SDS_TYPE_32:
            SDS_HDR(32, s)->len += inc;
            break;
        case SDS_TYPE_64:
            SDS_HDR(64, s)->len += inc;
            break;
    }
}

/* sdsalloc() = sdsavail() + sdslen() */
static inline size_t sdsalloc(const sds s) {
    unsigned char flags = s[-1];
    switch (flags & SDS_TYPE_MASK) {
        case SDS_TYPE_5:
            return SDS_TYPE_5_LEN(flags);
        case SDS_TYPE_8:
            return SDS_HDR(8, s)->alloc;
        case SDS_TYPE_16:
            return SDS_HDR(16, s)->alloc;
        case SDS_TYPE_32:
            return SDS_HDR(32, s)->alloc;
        case SDS_TYPE_64:
            return SDS_HDR(64, s)->alloc;
    }
    return 0;
}

static inline void sdssetalloc(sds s, size_t newlen) {
    unsigned char flags = s[-1];
    switch (flags & SDS_TYPE_MASK) {
        case SDS_TYPE_5:
            /* Nothing to do, this type has no total allocation info. */
            break;
        case SDS_TYPE_8:
            SDS_HDR(8, s)->alloc = newlen;
            break;
        case SDS_TYPE_16:
            SDS_HDR(16, s)->alloc = newlen;
            break;
        case SDS_TYPE_32:
            SDS_HDR(32, s)->alloc = newlen;
            break;
        case SDS_TYPE_64:
            SDS_HDR(64, s)->alloc = newlen;
            break;
    }
}


sds sdsnewlen(const void *init, size_t initlen);

sds sdsnew(const char *init);

sds sdsempty(void);

sds sdsdup(const sds s);

void sdsfree(sds s);

// 用空字符将 SDS 扩展到指定长度
sds sdsgrowzero(sds s, size_t len);
/*
 * todo: 字符串连接操作
 * 这里涉及拷贝内存操作
 * s: 原来字符串
 * t: 追加的字符串
 * len: 追加字符串的长度
 */
sds sdscatlen(sds s, const void *t, size_t len);
/* 字符串连接操作 */
sds sdscat(sds s, const char *t);
/* 字符串连接操作 */
sds sdscatsds(sds s, const sds t);
/* 字符串复制操作 */
sds sdscpylen(sds s, const char *t, size_t len);

// 将给定的 c 字符串复制到 SDS 里面，覆盖 SDS原有的字符串
sds sdscpy(sds s, const char *t);

sds sdscatvprintf(sds s, const char *fmt, va_list ap);

#ifdef __GNUC__
sds sdscatprintf(sds s, const char *fmt, ...)
    __attribute__((format(printf, 2, 3)));
#else

sds sdscatprintf(sds s, const char *fmt, ...);

#endif

sds sdscatfmt(sds s, char const *fmt, ...);

// 接收一个 SDS 和一个 c 字符串作为参数，从 SDS 中移除所有在 C 字符串中出现过的字符
sds sdstrim(sds s, const char *cset);

// 保留 SDS 给定区间内的数据，不在区间内的数据将会被覆盖或清除
void sdsrange(sds s, ssize_t start, ssize_t end);

void sdsupdatelen(sds s);

void sdsclear(sds s);

// 比较两个字符串是否相等
int sdscmp(const sds s1, const sds s2);

sds *sdssplitlen(const char *s, ssize_t len, const char *sep, int seplen, int *count);

void sdsfreesplitres(sds *tokens, int count);
/* 统一转换为小写字符 */
void sdstolower(sds s);
/* 统一转换为大写字符 */
void sdstoupper(sds s);
/* 将一个long long类型的数字转换为字符串 */
sds sdsfromlonglong(long long value);
/* 添加引用字符串 */
sds sdscatrepr(sds s, const char *p, size_t len);
/* 参数解析 */
sds *sdssplitargs(const char *line, int *argc);

/**
 * sdsmapchars执行的字符替换操作与我们常规理解的字符替换有些不同，
 * 它将sds中出现在from中的字符替换为to对应的字符。
 * 比如：
 * from = “ho”
 * to = “01”
 * s = “hello”
 * 经过sdsmapchars处理后s = “0ell1”
 *
 * @param s         源字符串
 * @param from      sds 中出现的 from 中的字符
 * @param to        使用 to 里面的字符替换
 * @param setlen    设置的长度
 * @return  返回一个新的 sds
 */
sds sdsmapchars(sds s, const char *from, const char *to, size_t setlen);
/* 将一个C风格的字符串数组用指定分隔符连接成一个字符串*/
sds sdsjoin(char **argv, int argc, char *sep);

sds sdsjoinsds(sds *argv, int argc, const char *sep, size_t seplen);

/* Low level functions exposed to the user API */

/* 确保sds中的可用空间大于或等于addlen，如果当前字符串可用空间不满足则重新配置空间 */
sds sdsMakeRoomFor(sds s, size_t addlen);
/* 根据给定参数incr调整当前长度和可用空间大小 */
void sdsIncrLen(sds s, ssize_t incr);
/* 释放字符数组buf中的多余空间，使其刚好能存放当前字符数 */
sds sdsRemoveFreeSpace(sds s);
/* 获取sds实际分配的空间大小（包括最后的'\0'结束符） */
size_t sdsAllocSize(sds s);

void *sdsAllocPtr(sds s);

/* Export the allocator used by SDS to the program using SDS.
 * Sometimes the program SDS is linked to, may use a different set of
 * allocators, but may want to allocate or free things that SDS will
 * respectively free or allocate. */
void *sds_malloc(size_t size);

void *sds_realloc(void *ptr, size_t size);

void sds_free(void *ptr);

#ifdef REDIS_TEST
int sdsTest(int argc, char *argv[]);
#endif

#endif
