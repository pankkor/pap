#pragma once

// --------------------------------------
// Types
// --------------------------------------

typedef signed char         i8;
typedef unsigned char       u8;
typedef short               i16;
typedef unsigned short      u16;
typedef int                 i32;
typedef unsigned int        u32;
typedef long long           i64;
typedef unsigned long long  u64;
typedef float               f32;
typedef double              f64;

typedef i32                 b32;
#define true                1
#define false               0

#define U64_MAX             -1UL

// --------------------------------------
// Compiler
// --------------------------------------

#define LIKELY(x)       __builtin_expect((x), 1)
#define UNLIKELY(x)     __builtin_expect((x), 0)
#define FORCE_INLINE    inline __attribute__((always_inline))
#define FUNC_NAME       __func__

#define CLEANUP(x)      __attribute__((cleanup(x)))

#define XSTR(x)         STR(x)
#define STR(x)          #x
#define CONCAT(x, y)    x ## y
#define XCONCAT(x, y)   CONCAT(x, y)

#ifdef __clang__
#define NO_OPT          __attribute__((optnone))
#define LOOP_NO_OPT    _Pragma(\
    "clang loop unroll(disable) vectorize(disable) interleave(disable)")
#else
#define NO_OPT          __attribute__((optimize(0)))
#define LOOP_NO_OPT    _Pragma("GCC unroll 1")
#endif // #ifdef __clang__

// --------------------------------------
// Utils
// --------------------------------------

#define ARRAY_COUNT(x)  (u64)(sizeof(x) / sizeof(x[0]))

#define SWAP(a, b)            \
  do {                        \
    __typeof__(a) tmp = (a);  \
    (a) = (b);                \
    (b) = tmp;                \
  } while(0)

#define CLAMP(k, l, r) __extension__ ({ \
    __typeof__(k) k_ = (k);             \
    __typeof__(l) l_ = (l);             \
    __typeof__(r) r_ = (r);             \
    k_ <= l_ ? l_ : k_ >= r_ ? r_ : k_; \
})

#define DSB(option)     __asm__ volatile ("dsb " #option : : : "memory")
