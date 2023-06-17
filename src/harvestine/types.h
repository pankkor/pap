#pragma once

typedef signed char     i8;
typedef unsigned char   u8;
typedef short           i16;
typedef unsigned short  u16;
typedef int             i32;
typedef unsigned int    u32;
typedef long            i64;
typedef unsigned long   u64;
typedef float           f32;
typedef double          f64;

typedef i32             b32;
#define true            1
#define false           0

#define LIKELY(x)       __builtin_expect((x), 1)
#define UNLIKELY(x)     __builtin_expect((x), 0)
#define FORCE_INLINE    inline __attribute__((always_inline))

#define ARRAY_LEN(x)    (u64)(sizeof(x) / sizeof(x[0]))

#define SWAP(a, b)            \
  do {                        \
    __typeof__(a) tmp = (a);  \
    (a) = (b);                \
    (b) = tmp;                \
  } while(0)

#define DSB(option)     __asm__ volatile ("dsb " #option : : : "memory")

#define NO_OPT          __attribute__((optnone))
#define LOOP_NO_OPT    _Pragma(\
    "clang loop unroll(disable) vectorize(disable) interleave(disable)")
