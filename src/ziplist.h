#ifndef _ZIPLIST_H
#define _ZIPLIST_H

#define ZIPLIST_HEAD 0
#define ZIPLIST_TAIL 1

/*
 * todo：ziplist 采用小端模式来存储
 *
 * ziplist的数据类型，没有用自定义的struct之类的来表达，而就是简单的unsigned char *。
 * 这是因为ziplist本质上就是一块连续内存，内部组成结构又是一个高度动态的设计（变长编码），
 * 也没法用一个固定的数据结构来表达。
 */

// ziplistNew: 创建一个空的ziplist（只包含<zlbytes><zltail><zllen><zlend>）
unsigned char *ziplistNew(void);
// ziplistMerge: 将两个ziplist合并成一个新的ziplist
unsigned char *ziplistMerge(unsigned char **first, unsigned char **second);
/**
 * 在ziplist的头部或尾端插入一段数据（产生一个新的数据项）。注意一下这个接口的返回值，是一个新的ziplist。
 * 调用方必须用这里返回的新的ziplist，替换之前传进来的旧的ziplist变量，而经过这个函数处理之后，原来旧的ziplist变量就失效了。
 * 为什么一个简单的插入操作会导致产生一个新的ziplist呢？
 * 这是因为ziplist是一块连续空间，对它的追加操作，会引发内存的realloc，因此ziplist的内存位置可能会发生变化。
 *
 * @param zl
 * @param s
 * @param slen
 * @param where
 * @return
 */
unsigned char *ziplistPush(unsigned char *zl, unsigned char *s, unsigned int slen, int where);
//  返回index参数指定的数据项的内存位置。index可以是负数，表示从尾端向前进行索引。
unsigned char *ziplistIndex(unsigned char *zl, int index);
// ziplistNext和ziplistPrev分别返回一个ziplist中指定数据项p的后一项和前一项。
unsigned char *ziplistNext(unsigned char *zl, unsigned char *p);
unsigned char *ziplistPrev(unsigned char *zl, unsigned char *p);
//
unsigned int ziplistGet(unsigned char *p, unsigned char **sval, unsigned int *slen, long long *lval);
// 在ziplist的任意数据项前面插入一个新的数据项。
unsigned char *ziplistInsert(unsigned char *zl, unsigned char *p, unsigned char *s, unsigned int slen);
// 删除指定的数据项。
unsigned char *ziplistDelete(unsigned char *zl, unsigned char **p);

unsigned char *ziplistDeleteRange(unsigned char *zl, int index, unsigned int num);
unsigned int ziplistCompare(unsigned char *p, unsigned char *s, unsigned int slen);
/**
 * 查找给定的数据（由vstr和vlen指定）。注意它有一个skip参数，表示查找的时候每次比较之间要跳过几个数据项。
 * 为什么会有这么一个参数呢？
 * 其实这个参数的主要用途是当用ziplist表示hash结构的时候，是按照一个field，一个value来依次存入ziplist的。
 * 也就是说，偶数索引的数据项存field，奇数索引的数据项存value。当按照field的值进行查找的时候，就需要把奇数项跳过去。
 *
 * @param p
 * @param vstr
 * @param vlen
 * @param skip
 * @return
 */
unsigned char *ziplistFind(unsigned char *p, unsigned char *vstr, unsigned int vlen, unsigned int skip);
/**
 * 计算ziplist的长度（即包含数据项的个数）。
 *
 * @param zl
 * @return
 */
unsigned int ziplistLen(unsigned char *zl);
size_t ziplistBlobLen(unsigned char *zl);
void ziplistRepr(unsigned char *zl);

#ifdef REDIS_TEST
int ziplistTest(int argc, char *argv[]);
#endif

#endif /* _ZIPLIST_H */
