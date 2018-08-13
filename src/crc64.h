#ifndef CRC64_H
#define CRC64_H

#include <stdint.h>

/**
 * 冗余校验 crc64 方法
 *
 * @param crc
 * @param s
 * @param l
 * @return
 */
uint64_t crc64(uint64_t crc, const unsigned char *s, uint64_t l);

#ifdef REDIS_TEST
int crc64Test(int argc, char *argv[]);
#endif

#endif
