/*
 * Copyright (c) 2015, Real-Time Innovations, Inc. All rights reserved.
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 */
#include <stdint.h>

typedef  uint8_t  u8_t;
typedef uint16_t u16_t;
typedef uint32_t u32_t;
typedef uint64_t u64_t;

typedef  int8_t   s8_t;
typedef int16_t  s16_t;
typedef int32_t  s32_t;
typedef int64_t  s64_t;

typedef uintptr_t mem_ptr_t;


#define U16_F "hu"
#define S16_F "d"
#define X16_F "hx"
#define U32_F "u"
#define S32_F "d"
#define X32_F "x"
#define SZT_F "uz"


#if defined(__BYTE_ORDER__)
#ifndef BYTE_ORDER
#  define BYTE_ORDER __BYTE_ORDER__
#endif
#elif defined(__BIG_ENDIAN)
#ifndef BYTE_ORDER
#  define BYTE_ORDER BIG_ENDIAN
#endif
#elif defined(__LITTLE_ENDIAN)
#ifndef BYTE_ORDER
#  define BYTE_ORDER LITTLE_ENDIAN
#endif
#else
#  error Unable to detemine system endianess
#endif


#define LWIP_CHKSUM_ALGORITHM 2


//#define PACK_STRUCT_FIELD(x) x __attribute__((packed))
#define PACK_STRUCT_STRUCT __attribute__((packed))
#define PACK_STRUCT_BEGIN
#define PACK_STRUCT_END


#if 0
#define LWIP_PLATFORM_BYTESWAP 1
#define LWIP_PLATFORM_HTONS(x) ( (((u16_t)(x))>>8) | (((x)&0xFF)<<8) )
#define LWIP_PLATFORM_HTONL(x) ( (((u32_t)(x))>>24) | (((x)&0xFF0000)>>8) \
                               | (((x)&0xFF00)<<8) | (((x)&0xFF)<<24) )
#endif


#include <stdio.h>
#include <stdlib.h>
/* Plaform specific diagnostic output */
#define LWIP_PLATFORM_DIAG(x) \
        do {                  \
            printf x;         \
            fflush(stdout);   \
        } while(0)

#define LWIP_PLATFORM_ASSERT(x)                                  \
        do {                                                     \
            printf("Assertion \"%s\" failed at line %d in %s\n", \
                   x, __LINE__, __FILE__);                       \
            fflush(stdout);                                      \
            abort();                                             \
        } while(0)


