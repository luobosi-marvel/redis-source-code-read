#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "intset.h"
#include "zmalloc.h"
#include "endianconv.h"

/*
 * Note that these encodings are ordered, so:
 * INTSET_ENC_INT16 < INTSET_ENC_INT32 < INTSET_ENC_INT64.
 * todo: 整数集合intset中是以递增的形式存放整数的，可以加快查找速度
 * 这里表示 intset 的三种编码类型
 */
#define INTSET_ENC_INT16 (sizeof(int16_t))
#define INTSET_ENC_INT32 (sizeof(int32_t))
#define INTSET_ENC_INT64 (sizeof(int64_t))

/* 返回该值实际的编码 */
static uint8_t _intsetValueEncoding(int64_t v) {
    if (v < INT32_MIN || v > INT32_MAX)
        return INTSET_ENC_INT64;
    else if (v < INT16_MIN || v > INT16_MAX)
        return INTSET_ENC_INT32;
    else
        return INTSET_ENC_INT16;
}

/* 根据指定位置和编码格式获取值(涉及内存地址操作) */
static int64_t _intsetGetEncoded(intset *is, int pos, uint8_t enc) {
    int64_t v64;
    int32_t v32;
    int16_t v16;

    if (enc == INTSET_ENC_INT64) {
        // memcpy 函数是内存拷贝函数
        // 这里不是访问下标，而是直接操作内存地址，效率更快，这也是为什么需要传入编码格式的原因
        memcpy(&v64, ((int64_t *) is->contents) + pos, sizeof(v64));
        // 将 64位无符号整型由小端转成大端
        memrev64ifbe(&v64);
        return v64;
    } else if (enc == INTSET_ENC_INT32) {
        memcpy(&v32, ((int32_t *) is->contents) + pos, sizeof(v32));
        memrev32ifbe(&v32);
        return v32;
    } else {
        memcpy(&v16, ((int16_t *) is->contents) + pos, sizeof(v16));
        memrev16ifbe(&v16);
        return v16;
    }
}

/* Return the value at pos, using the configured encoding. */
static int64_t _intsetGet(intset *is, int pos) {
    return _intsetGetEncoded(is, pos, intrev32ifbe(is->encoding));
}

/*
 * 私有方法
 * 使用该值的编码格式，在指定位置插入改值
 */
static void _intsetSet(intset *is, int pos, int64_t value) {
    uint32_t encoding = intrev32ifbe(is->encoding);

    if (encoding == INTSET_ENC_INT64) {
        // 在指定位置存放元素
        ((int64_t *) is->contents)[pos] = value;
        // todo
        memrev64ifbe(((int64_t *) is->contents) + pos);
    } else if (encoding == INTSET_ENC_INT32) {
        ((int32_t *) is->contents)[pos] = value;
        memrev32ifbe(((int32_t *) is->contents) + pos);
    } else {
        ((int16_t *) is->contents)[pos] = value;
        memrev16ifbe(((int16_t *) is->contents) + pos);
    }
}

/* Create an empty intset. */
intset *intsetNew(void) {
    intset *is = zmalloc(sizeof(intset));
    is->encoding = intrev32ifbe(INTSET_ENC_INT16);
    is->length = 0;
    return is;
}

/*
 * Resize the intset  扩容，重新分配数组的大小, 每次扩容一个元素
 * todo: 这里为什么不用判断就直接把小端转成大端？
 * 因为这里存储就是按照小端存储的
 */
static intset *intsetResize(intset *is, uint32_t len) {
    // 计算新增的字节数
    // intrev32ifbe 用来把 32 位整型从小端转成大端
    uint32_t size = len * intrev32ifbe(is->encoding);
    // 重新分配内存
    is = zrealloc(is, sizeof(intset) + size);
    return is;
}

/* 
 * Search for the position of "value". Return 1 when the value was found and
 * sets "pos" to the position of the value within the intset. Return 0 when
 * the value is not present in the intset and sets "pos" to the position
 * where "value" can be inserted. 
 *
 * 二分法查找该元素是否已经存在该集合之中
 */
static uint8_t intsetSearch(intset *is, int64_t value, uint32_t *pos) {
    int min = 0, max = intrev32ifbe(is->length) - 1, mid = -1;
    int64_t cur = -1;

    /* The value can never be found when the set is empty */
    if (intrev32ifbe(is->length) == 0) {
        if (pos) *pos = 0;
        return 0;
    } else {
        /* Check for the case where we know we cannot find the value,
         * but do know the insert position. */
        if (value > _intsetGet(is, intrev32ifbe(is->length) - 1)) {
            if (pos) *pos = intrev32ifbe(is->length);
            return 0;
        } else if (value < _intsetGet(is, 0)) {
            if (pos) *pos = 0;
            return 0;
        }
    }

    while (max >= min) {
        mid = ((unsigned int) min + (unsigned int) max) >> 1;
        cur = _intsetGet(is, mid);
        if (value > cur) {
            min = mid + 1;
        } else if (value < cur) {
            max = mid - 1;
        } else {
            break;
        }
    }

    if (value == cur) {
        if (pos) *pos = mid;
        return 1;
    } else {
        if (pos) *pos = min;
        return 0;
    }
}

/*
 * todo: 如果给定的值超过了当前数组的编码，那么该方法会将 intset 升级为更大的编码然后插入给定整数
 *
 */
static intset *intsetUpgradeAndAdd(intset *is, int64_t value) {
    // 旧的编码格式
    uint8_t curenc = intrev32ifbe(is->encoding);
    // 插入元素的编码格式
    uint8_t newenc = _intsetValueEncoding(value);
    int length = intrev32ifbe(is->length);
    // 该值是用来判断 value 在 beginning 还是 end 位置插入的标志
    int prepend = value < 0 ? 1 : 0;

    /* First set new encoding and resize */
    is->encoding = intrev32ifbe(newenc);
    // 扩容一个元素
    is = intsetResize(is, intrev32ifbe(is->length) + 1);

    /*
     * Upgrade back-to-front so we don't overwrite values.
     * Note that the "prepend" variable is used to make sure we have an empty
     * space at either the beginning or the end of the intset.
     */
    while (length--)
        _intsetSet(is, length + prepend, _intsetGetEncoded(is, length, curenc));

    /* Set the value at the beginning or the end. */
    if (prepend)
        _intsetSet(is, 0, value);
    else
        _intsetSet(is, intrev32ifbe(is->length), value);
    // todo: 搞不懂，为什么要转换两次？
    is->length = intrev32ifbe(intrev32ifbe(is->length) + 1);
    return is;
}

/**
 * [intsetMoveTail description]: 将 from 位置开始的数据移动到 to 位置开始后的空间，用户删除操作。
 * 改操作结束后，contents 数组尾端可能存在无效数据
 * 
 * @param is   [description]
 * @param from [description]
 * @param to   [description]
 */
static void intsetMoveTail(intset *is, uint32_t from, uint32_t to) {
    void *src, *dst;
    // 计算 from 位置开始共有多少个元素
    uint32_t bytes = intrev32ifbe(is->length) - from;
    uint32_t encoding = intrev32ifbe(is->encoding);
    // 根据编码方式计算需要移动的字节数、目标位置移动开始位置
    if (encoding == INTSET_ENC_INT64) {
        src = (int64_t *) is->contents + from;
        dst = (int64_t *) is->contents + to;
        bytes *= sizeof(int64_t);
    } else if (encoding == INTSET_ENC_INT32) {
        src = (int32_t *) is->contents + from;
        dst = (int32_t *) is->contents + to;
        bytes *= sizeof(int32_t);
    } else {
        src = (int16_t *) is->contents + from;
        dst = (int16_t *) is->contents + to;
        bytes *= sizeof(int16_t);
    }
    // 内存移动
    memmove(dst, src, bytes);
}

/*
 * Insert an integer in the intset
 * 添加一个整数节点
 */
intset *intsetAdd(intset *is, int64_t value, uint8_t *success) {
    // 返回该值实际的编码
    uint8_t valenc = _intsetValueEncoding(value);
    uint32_t pos;
    if (success) *success = 1;

    /* 
     * Upgrade encoding if necessary. If we need to upgrade, we know that
     * this value should be either appended (if > 0) or prepended (if < 0),
     * because it lies outside the range of existing values. 
     *
     * 将intset升级为更大的编码并插入给定的整数。
     */
    if (valenc > intrev32ifbe(is->encoding)) {
        /* This always succeeds, so we don't need to curry *success. */
        return intsetUpgradeAndAdd(is, value);
    } else {
        /* 
         * Abort if the value is already present in the set.
         * This call will populate "pos" with the right position to insert
         * the value when it cannot be found. 
         *
         * 判断该值是否已经存在该集合之中
         */
        if (intsetSearch(is, value, &pos)) {
            if (success) *success = 0;
            return is;
        }
        // todo: 给数组扩容，每次扩容 1 个单位
        is = intsetResize(is, intrev32ifbe(is->length) + 1);
        // 如果 pos < length 则将 pos 后面的元素后移一位，想想数组实现的线下表就能明白了
        if (pos < intrev32ifbe(is->length)) intsetMoveTail(is, pos, pos + 1);
    }
    // 在指定位置插入元素
    _intsetSet(is, pos, value);
    is->length = intrev32ifbe(intrev32ifbe(is->length) + 1);
    return is;
}

/* Delete integer from intset */
intset *intsetRemove(intset *is, int64_t value, int *success) {
    uint8_t valenc = _intsetValueEncoding(value);
    uint32_t pos;
    if (success) *success = 0;

    if (valenc <= intrev32ifbe(is->encoding) && intsetSearch(is, value, &pos)) {
        uint32_t len = intrev32ifbe(is->length);

        /* We know we can delete */
        if (success) *success = 1;

        /* Overwrite value with tail and update length */
        if (pos < (len - 1)) intsetMoveTail(is, pos + 1, pos);
        is = intsetResize(is, len - 1);
        is->length = intrev32ifbe(len - 1);
    }
    return is;
}

/* Determine whether a value belongs to this set */
uint8_t intsetFind(intset *is, int64_t value) {
    uint8_t valenc = _intsetValueEncoding(value);
    return valenc <= intrev32ifbe(is->encoding) && intsetSearch(is, value, NULL);
}

/* Return random member */
int64_t intsetRandom(intset *is) {
    return _intsetGet(is, rand() % intrev32ifbe(is->length));
}

/*
 * Get the value at the given position. When this position is
 * out of range the function returns 0, when in range it returns 1.
 */
uint8_t intsetGet(intset *is, uint32_t pos, int64_t *value) {
    if (pos < intrev32ifbe(is->length)) {
        *value = _intsetGet(is, pos);
        return 1;
    }
    return 0;
}

/* Return intset length */
uint32_t intsetLen(const intset *is) {
    return intrev32ifbe(is->length);
}

/* Return intset blob size in bytes. */
size_t intsetBlobLen(intset *is) {
    return sizeof(intset) + intrev32ifbe(is->length) * intrev32ifbe(is->encoding);
}

#ifdef REDIS_TEST
#include <sys/time.h>
#include <time.h>

#if 0
static void intsetRepr(intset *is) {
    for (uint32_t i = 0; i < intrev32ifbe(is->length); i++) {
        printf("%lld\n", (uint64_t)_intsetGet(is,i));
    }
    printf("\n");
}

static void error(char *err) {
    printf("%s\n", err);
    exit(1);
}
#endif

static void ok(void) {
    printf("OK\n");
}

static long long usec(void) {
    struct timeval tv;
    gettimeofday(&tv,NULL);
    return (((long long)tv.tv_sec)*1000000)+tv.tv_usec;
}

#define assert(_e) ((_e)?(void)0:(_assert(#_e,__FILE__,__LINE__),exit(1)))
static void _assert(char *estr, char *file, int line) {
    printf("\n\n=== ASSERTION FAILED ===\n");
    printf("==> %s:%d '%s' is not true\n",file,line,estr);
}

static intset *createSet(int bits, int size) {
    uint64_t mask = (1<<bits)-1;
    uint64_t value;
    intset *is = intsetNew();

    for (int i = 0; i < size; i++) {
        if (bits > 32) {
            value = (rand()*rand()) & mask;
        } else {
            value = rand() & mask;
        }
        is = intsetAdd(is,value,NULL);
    }
    return is;
}

static void checkConsistency(intset *is) {
    for (uint32_t i = 0; i < (intrev32ifbe(is->length)-1); i++) {
        uint32_t encoding = intrev32ifbe(is->encoding);

        if (encoding == INTSET_ENC_INT16) {
            int16_t *i16 = (int16_t*)is->contents;
            assert(i16[i] < i16[i+1]);
        } else if (encoding == INTSET_ENC_INT32) {
            int32_t *i32 = (int32_t*)is->contents;
            assert(i32[i] < i32[i+1]);
        } else {
            int64_t *i64 = (int64_t*)is->contents;
            assert(i64[i] < i64[i+1]);
        }
    }
}

#define UNUSED(x) (void)(x)
int intsetTest(int argc, char **argv) {
    uint8_t success;
    int i;
    intset *is;
    srand(time(NULL));

    UNUSED(argc);
    UNUSED(argv);

    printf("Value encodings: "); {
        assert(_intsetValueEncoding(-32768) == INTSET_ENC_INT16);
        assert(_intsetValueEncoding(+32767) == INTSET_ENC_INT16);
        assert(_intsetValueEncoding(-32769) == INTSET_ENC_INT32);
        assert(_intsetValueEncoding(+32768) == INTSET_ENC_INT32);
        assert(_intsetValueEncoding(-2147483648) == INTSET_ENC_INT32);
        assert(_intsetValueEncoding(+2147483647) == INTSET_ENC_INT32);
        assert(_intsetValueEncoding(-2147483649) == INTSET_ENC_INT64);
        assert(_intsetValueEncoding(+2147483648) == INTSET_ENC_INT64);
        assert(_intsetValueEncoding(-9223372036854775808ull) ==
                    INTSET_ENC_INT64);
        assert(_intsetValueEncoding(+9223372036854775807ull) ==
                    INTSET_ENC_INT64);
        ok();
    }

    printf("Basic adding: "); {
        is = intsetNew();
        is = intsetAdd(is,5,&success); assert(success);
        is = intsetAdd(is,6,&success); assert(success);
        is = intsetAdd(is,4,&success); assert(success);
        is = intsetAdd(is,4,&success); assert(!success);
        ok();
    }

    printf("Large number of random adds: "); {
        uint32_t inserts = 0;
        is = intsetNew();
        for (i = 0; i < 1024; i++) {
            is = intsetAdd(is,rand()%0x800,&success);
            if (success) inserts++;
        }
        assert(intrev32ifbe(is->length) == inserts);
        checkConsistency(is);
        ok();
    }

    printf("Upgrade from int16 to int32: "); {
        is = intsetNew();
        is = intsetAdd(is,32,NULL);
        assert(intrev32ifbe(is->encoding) == INTSET_ENC_INT16);
        is = intsetAdd(is,65535,NULL);
        assert(intrev32ifbe(is->encoding) == INTSET_ENC_INT32);
        assert(intsetFind(is,32));
        assert(intsetFind(is,65535));
        checkConsistency(is);

        is = intsetNew();
        is = intsetAdd(is,32,NULL);
        assert(intrev32ifbe(is->encoding) == INTSET_ENC_INT16);
        is = intsetAdd(is,-65535,NULL);
        assert(intrev32ifbe(is->encoding) == INTSET_ENC_INT32);
        assert(intsetFind(is,32));
        assert(intsetFind(is,-65535));
        checkConsistency(is);
        ok();
    }

    printf("Upgrade from int16 to int64: "); {
        is = intsetNew();
        is = intsetAdd(is,32,NULL);
        assert(intrev32ifbe(is->encoding) == INTSET_ENC_INT16);
        is = intsetAdd(is,4294967295,NULL);
        assert(intrev32ifbe(is->encoding) == INTSET_ENC_INT64);
        assert(intsetFind(is,32));
        assert(intsetFind(is,4294967295));
        checkConsistency(is);

        is = intsetNew();
        is = intsetAdd(is,32,NULL);
        assert(intrev32ifbe(is->encoding) == INTSET_ENC_INT16);
        is = intsetAdd(is,-4294967295,NULL);
        assert(intrev32ifbe(is->encoding) == INTSET_ENC_INT64);
        assert(intsetFind(is,32));
        assert(intsetFind(is,-4294967295));
        checkConsistency(is);
        ok();
    }

    printf("Upgrade from int32 to int64: "); {
        is = intsetNew();
        is = intsetAdd(is,65535,NULL);
        assert(intrev32ifbe(is->encoding) == INTSET_ENC_INT32);
        is = intsetAdd(is,4294967295,NULL);
        assert(intrev32ifbe(is->encoding) == INTSET_ENC_INT64);
        assert(intsetFind(is,65535));
        assert(intsetFind(is,4294967295));
        checkConsistency(is);

        is = intsetNew();
        is = intsetAdd(is,65535,NULL);
        assert(intrev32ifbe(is->encoding) == INTSET_ENC_INT32);
        is = intsetAdd(is,-4294967295,NULL);
        assert(intrev32ifbe(is->encoding) == INTSET_ENC_INT64);
        assert(intsetFind(is,65535));
        assert(intsetFind(is,-4294967295));
        checkConsistency(is);
        ok();
    }

    printf("Stress lookups: "); {
        long num = 100000, size = 10000;
        int i, bits = 20;
        long long start;
        is = createSet(bits,size);
        checkConsistency(is);

        start = usec();
        for (i = 0; i < num; i++) intsetSearch(is,rand() % ((1<<bits)-1),NULL);
        printf("%ld lookups, %ld element set, %lldusec\n",
               num,size,usec()-start);
    }

    printf("Stress add+delete: "); {
        int i, v1, v2;
        is = intsetNew();
        for (i = 0; i < 0xffff; i++) {
            v1 = rand() % 0xfff;
            is = intsetAdd(is,v1,NULL);
            assert(intsetFind(is,v1));

            v2 = rand() % 0xfff;
            is = intsetRemove(is,v2,NULL);
            assert(!intsetFind(is,v2));
        }
        checkConsistency(is);
        ok();
    }

    return 0;
}
#endif
