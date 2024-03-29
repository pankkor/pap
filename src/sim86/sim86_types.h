#pragma once

typedef signed char         i8;
typedef unsigned char       u8;
typedef short               i16;
typedef unsigned short      u16;
typedef int                 i32;
typedef unsigned int        u32;
typedef long long           i64;
typedef unsigned long long  u64;
typedef _Bool               bool;
#define true                1
#define false               0

#define FORCE_INLINE        inline __attribute__((always_inline))
#define ARRAY_COUNT(x)      (u64)(sizeof(x) / sizeof(x[0]))
#define SWAP(a, b)            \
  do {                        \
    __typeof__(a) tmp = (a);  \
    (a) = (b);                \
    (b) = tmp;                \
  } while(0)
