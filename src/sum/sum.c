// Performance-Aware-Programming Course
// https://www.computerenhance.com/p/table-of-contents
//
// Prologue
// sum of array of u32 numbers

#include <stdio.h>    // printf
#include <stdlib.h>   // atoi

#if defined(__i386__) || defined(__x86_64__)
#include <immintrin.h>
#elif defined(__ARM_NEON)
#include <arm_neon.h>
#else
#error Unsupported architecture
#endif

typedef int           i32;
typedef unsigned int  u32;
typedef long          i64;
typedef unsigned long u64;
typedef float         f32;
typedef double        f64;

#define BENCH_ESCAPE(p) __asm__ volatile("" : : "m"(p) : "memory")
#define BENCH_CLOBBER() __asm__ volatile("" : : : "memory")

#define ALIGNED(x)      __attribute__((aligned(x)))
#define FORCE_INLINE    inline __attribute__((always_inline))
#define ARRAY_SIZE(x)  (u64)(sizeof(x) / sizeof(x[0]))

// Disable optimizations
#define NO_OPT          __attribute__((optnone))

#ifdef __clang__
#define LOOP_NO_OPT    _Pragma(\
    "clang loop unroll(disable) vectorize(disable) interleave(disable)")
#else
// TODO
#define LOOP_NO_OPT
#endif // #ifdef __clang__

#ifdef __aarch64__
static FORCE_INLINE u64 rdtsc(void) {
  u64 val;
  // use isb to avoid speculative read of cntvct_el0
  __asm__ volatile("isb;\n\tmrs %0, cntvct_el0" : "=r" (val) :: "memory");
  return val;
}

static FORCE_INLINE u64 get_tsc_freq(void) {
  u64 val;
  __asm__ volatile("mrs %0, cntfrq_el0" : "=r" (val));
  return val;
}
#else
static FORCE_INLINE u64 rdtsc(void) {
  return __rdtsc();
}

static FORCE_INLINE u64 get_tsc_freq(void) {
  return 0;
}
#endif // #ifdef __aarch64__

static u64 s_tsc_freq;

static FORCE_INLINE f64 tsc_to_s(f64 tsc) {
  return tsc / s_tsc_freq;
}

static u64 benchmark(void (func)(void), u64 count) {
  u64 min_cycles = -1;
  for (u64 i = 0; i < count; ++i) {
    u64 beg = rdtsc();

    func();

    u64 end = rdtsc();

    u64 cycles = end - beg;
    if (cycles < min_cycles) {
      min_cycles = cycles;
    }
  }
  return min_cycles;
}

static void benchmark_print_result(
    const char *benchmark_name,
    f64 tsc,
    u64 ops) {
  printf("--- %s ---\n", benchmark_name);
  printf("TSC total                   %.8ftsc\n", tsc);
  printf("TSC per operation           %.8ftsc\n", tsc / ops);
  printf("Operations per TSC          %.8fops p/tsc\n", ops / tsc);

  if (s_tsc_freq) {
    f64 ns = tsc_to_s(tsc * 1e9);
    printf("Time total                  %.8fns\n", ns);
    printf("Time per operation          %.8fns\n", ns / ops);
    printf("Operation per ns            %.8fops p/ns\n", ops / ns);
  }
  printf("\n");
  fflush(stdout);
}

// Benchmnarks
static FORCE_INLINE u32 sum_naive(u64 count, u32 * restrict in) {
  u32 sum = 0;
  LOOP_NO_OPT
  for (u64 i = 0; i < count; ++i) {
    sum += in[i];
  }
  return sum;
}

static FORCE_INLINE u32 sum_unroll2(u64 count, u32 * restrict in) {
  u32 sum0 = 0;
  u32 sum1 = 0;

  u32 *it = in;
  u32 *end = in + count - 1;
  LOOP_NO_OPT
  while (it < end) {
    sum0 += it[0];
    sum1 += it[1];
    it += 2;
  }
  return sum0 + sum1;
}

static FORCE_INLINE u32 sum_unroll4(u64 count, u32 * restrict in) {
  u32 sum0 = 0;
  u32 sum1 = 0;
  u32 sum2 = 0;
  u32 sum3 = 0;

  u32 *it = in;
  u32 *end = in + count - 3;
  LOOP_NO_OPT
  while (it < end) {
    sum0 += it[0];
    sum1 += it[1];
    sum2 += it[2];
    sum3 += it[3];
    it += 4;
  }
  return sum0 + sum1 + sum2 + sum3;
}

static FORCE_INLINE u32 sum_unroll8(u64 count, u32 * restrict in) {
  u32 sum0 = 0;
  u32 sum1 = 0;
  u32 sum2 = 0;
  u32 sum3 = 0;
  u32 sum4 = 0;
  u32 sum5 = 0;
  u32 sum6 = 0;
  u32 sum7 = 0;

  u32 *it = in;
  u32 *end = in + count - 7;
  LOOP_NO_OPT
  while (it < end) {
    sum0 += it[0];
    sum1 += it[1];
    sum2 += it[2];
    sum3 += it[3];
    sum4 += it[4];
    sum5 += it[5];
    sum6 += it[6];
    sum7 += it[7];
    it += 8;
  }
  return sum0 + sum1 + sum2 + sum3 + sum4 + sum5 + sum6 + sum7;
}

#ifdef __ARM_NEON
static FORCE_INLINE u32 sum_simd4(u64 count, u32 * restrict in) {
  uint32x4_t sum0 = vdupq_n_u32(0);

  u32 *it = in;
  u32 *end = in + count - 3;
  while (it < end) {
    uint32x4_t v0 = vld1q_u32(it);
    sum0 = vaddq_u32(sum0, v0);
    it += 4;
  }
  return vaddvq_u32(sum0);
}

static FORCE_INLINE u32 sum_simd8(u64 count, u32 * restrict in) {
  uint32x4_t sum0 = vdupq_n_u32(0);
  uint32x4_t sum1 = vdupq_n_u32(0);

  u32 *it = in;
  u32 *end = in + count - 7;
  while (it < end) {
    uint32x4_t v0 = vld1q_u32(it + 0);
    uint32x4_t v1 = vld1q_u32(it + 4);
    sum0 = vaddq_u32(sum0, v0);
    sum1 = vaddq_u32(sum1, v1);
    it += 8;
  }

  uint32x4_t t = vaddq_u32(sum0, sum1);
  return vaddvq_u32(t);
}

static FORCE_INLINE u32 sum_simd16(u64 count, u32 * restrict in) {
  uint32x4_t sum0 = vdupq_n_u32(0);
  uint32x4_t sum1 = vdupq_n_u32(0);
  uint32x4_t sum2 = vdupq_n_u32(0);
  uint32x4_t sum3 = vdupq_n_u32(0);

  u32 *it = in;
  u32 *end = in + count - 15;
  while (it < end) {
    // Note: using 4 vld1q_u32 intrinsics results in two ldp asm instructions
    // using vld2q or vld4q is 2 times slower than ldp
    uint32x4_t v0 = vld1q_u32(it + 0);
    uint32x4_t v1 = vld1q_u32(it + 4);
    uint32x4_t v2 = vld1q_u32(it + 8);
    uint32x4_t v3 = vld1q_u32(it + 12);
    sum0 = vaddq_u32(sum0, v0);
    sum1 = vaddq_u32(sum1, v1);
    sum2 = vaddq_u32(sum2, v2);
    sum3 = vaddq_u32(sum3, v3);
    it += 16;
  }

  uint32x4_t t0 = vaddq_u32(sum0, sum1);
  uint32x4_t t1 = vaddq_u32(sum2, sum3);
  uint32x4_t t = vaddq_u32(t0, t1);
  return vaddvq_u32(t);
}

static FORCE_INLINE u32 sum_simd32(u64 count, u32 * restrict in) {
  uint32x4_t sum0 = vdupq_n_u32(0);
  uint32x4_t sum1 = vdupq_n_u32(0);
  uint32x4_t sum2 = vdupq_n_u32(0);
  uint32x4_t sum3 = vdupq_n_u32(0);
  uint32x4_t sum4 = vdupq_n_u32(0);
  uint32x4_t sum5 = vdupq_n_u32(0);
  uint32x4_t sum6 = vdupq_n_u32(0);
  uint32x4_t sum7 = vdupq_n_u32(0);

  u32 *it = in;
  u32 *end = in + count - 31;
  while (it < end) {
    // Note: using 4 vld1q_u32 intrinsics results in two ldp asm instructions
    // using vld2q or vld4q is 2 times slower than ldp
    uint32x4_t v0 = vld1q_u32(it + 0);
    uint32x4_t v1 = vld1q_u32(it + 4);
    uint32x4_t v2 = vld1q_u32(it + 8);
    uint32x4_t v3 = vld1q_u32(it + 12);
    uint32x4_t v4 = vld1q_u32(it + 16);
    uint32x4_t v5 = vld1q_u32(it + 20);
    uint32x4_t v6 = vld1q_u32(it + 24);
    uint32x4_t v7 = vld1q_u32(it + 28);
    sum0 = vaddq_u32(sum0, v0);
    sum1 = vaddq_u32(sum1, v1);
    sum2 = vaddq_u32(sum2, v2);
    sum3 = vaddq_u32(sum3, v3);
    sum4 = vaddq_u32(sum4, v4);
    sum5 = vaddq_u32(sum5, v5);
    sum6 = vaddq_u32(sum6, v6);
    sum7 = vaddq_u32(sum7, v7);
    it += 32;
  }

  uint32x4_t t0 = vaddq_u32(sum0, sum1);
  uint32x4_t t1 = vaddq_u32(sum2, sum3);
  uint32x4_t t2 = vaddq_u32(sum4, sum5);
  uint32x4_t t3 = vaddq_u32(sum6, sum7);
  uint32x4_t t01 = vaddq_u32(t0, t1);
  uint32x4_t t23 = vaddq_u32(t2, t3);
  uint32x4_t t = vaddq_u32(t01, t23);
  return vaddvq_u32(t);
}

#elif __SSSE3__

static FORCE_INLINE u32 sum_simd4(u64 count, u32 * restrict in) {
  __m128i sum0 = _mm_setzero_si128();

  u32 *it = in;
  u32 *end = in + count - 3;

  while (it < end) {
    __m128i v0 = _mm_load_si128((__m128i *)in);
    sum0 = _mm_add_epi32(sum0, v0);
    it += 4;
  }

  sum0 = _mm_hadd_epi32(sum0, sum0);
  sum0 = _mm_hadd_epi32(sum0, sum0);
  return _mm_cvtsi128_si32(sum0);
}

static FORCE_INLINE u32 sum_simd8(u64 count, u32 * restrict in) {
  __m128i sum0 = _mm_setzero_si128();
  __m128i sum1 = _mm_setzero_si128();

  u32 *it = in;
  u32 *end = in + count - 7;

  while (it < end) {
    __m128i v0 = _mm_load_si128((__m128i *)in + 0);
    __m128i v1 = _mm_load_si128((__m128i *)in + 4);
    sum0 = _mm_add_epi32(sum0, v0);
    sum1 = _mm_add_epi32(sum1, v1);
    it += 8;
  }

  __m128i t = _mm_hadd_epi32(sum0, sum1);
  t = _mm_hadd_epi32(t, t);
  t = _mm_hadd_epi32(t, t);

  return _mm_cvtsi128_si32(t);
}

static FORCE_INLINE u32 sum_simd16(u64 count, u32 * restrict in) {
  __m128i sum0 = _mm_setzero_si128();
  __m128i sum1 = _mm_setzero_si128();
  __m128i sum2 = _mm_setzero_si128();
  __m128i sum3 = _mm_setzero_si128();

  u32 *it = in;
  u32 *end = in + count - 15;
  while (it < end) {
    __m128i v0 = _mm_load_si128((__m128i *)in + 0);
    __m128i v1 = _mm_load_si128((__m128i *)in + 4);
    __m128i v2 = _mm_load_si128((__m128i *)in + 8);
    __m128i v3 = _mm_load_si128((__m128i *)in + 12);
    sum0 = _mm_add_epi32(sum0, v0);
    sum1 = _mm_add_epi32(sum1, v1);
    sum2 = _mm_add_epi32(sum2, v2);
    sum3 = _mm_add_epi32(sum3, v3);
    it += 16;
  }

  __m128i t0 = _mm_hadd_epi32(sum0, sum1);
  __m128i t1 = _mm_hadd_epi32(sum2, sum3);
  __m128i t = _mm_hadd_epi32(t0, t1);

  t = _mm_hadd_epi32(t, t);
  t = _mm_hadd_epi32(t, t);
  return _mm_cvtsi128_si32(t);
}

static FORCE_INLINE u32 sum_simd32(u64 count, u32 * restrict in) {
  __m128i sum0 = _mm_setzero_si128();
  __m128i sum1 = _mm_setzero_si128();
  __m128i sum2 = _mm_setzero_si128();
  __m128i sum3 = _mm_setzero_si128();
  __m128i sum4 = _mm_setzero_si128();
  __m128i sum5 = _mm_setzero_si128();
  __m128i sum6 = _mm_setzero_si128();
  __m128i sum7 = _mm_setzero_si128();

  u32 *it = in;
  u32 *end = in + count - 31;
  while (it < end) {
    __m128i v0 = _mm_load_si128((__m128i *)in + 0);
    __m128i v1 = _mm_load_si128((__m128i *)in + 4);
    __m128i v2 = _mm_load_si128((__m128i *)in + 8);
    __m128i v3 = _mm_load_si128((__m128i *)in + 12);
    __m128i v4 = _mm_load_si128((__m128i *)in + 16);
    __m128i v5 = _mm_load_si128((__m128i *)in + 20);
    __m128i v6 = _mm_load_si128((__m128i *)in + 24);
    __m128i v7 = _mm_load_si128((__m128i *)in + 28);
    sum0 = _mm_add_epi32(sum0, v0);
    sum1 = _mm_add_epi32(sum1, v1);
    sum2 = _mm_add_epi32(sum2, v2);
    sum3 = _mm_add_epi32(sum3, v3);
    sum4 = _mm_add_epi32(sum4, v4);
    sum5 = _mm_add_epi32(sum5, v5);
    sum6 = _mm_add_epi32(sum6, v6);
    sum7 = _mm_add_epi32(sum7, v7);
    it += 32;
  }

  __m128i t0 = _mm_hadd_epi32(sum0, sum1);
  __m128i t1 = _mm_hadd_epi32(sum2, sum3);
  __m128i t2 = _mm_hadd_epi32(sum4, sum5);
  __m128i t3 = _mm_hadd_epi32(sum6, sum7);
  __m128i t01 = _mm_hadd_epi32(t0, t1);
  __m128i t23 = _mm_hadd_epi32(t2, t3);
  __m128i t = _mm_hadd_epi32(t01, t23);

  t = _mm_hadd_epi32(t, t);
  t = _mm_hadd_epi32(t, t);
  return _mm_cvtsi128_si32(t);
}
#endif // #ifdef __ARM_NEON__

enum {ARR_CAPACITY = 12 * 1024 * 1024};
ALIGNED(128) u32 s_arr[ARR_CAPACITY] = {0}; // big enough
u64 s_arr_size = 8192;

#define GEN_BENCHMARK_FUNC_SUM(name)      \
  void benchmark_##name(void) {           \
    u32 sum = 0;                          \
    BENCH_ESCAPE(sum);                    \
    sum = (name)(s_arr_size, s_arr);      \
    BENCH_CLOBBER();                      \
  }

GEN_BENCHMARK_FUNC_SUM(sum_naive)
GEN_BENCHMARK_FUNC_SUM(sum_unroll2)
GEN_BENCHMARK_FUNC_SUM(sum_unroll4)
GEN_BENCHMARK_FUNC_SUM(sum_unroll8)
GEN_BENCHMARK_FUNC_SUM(sum_simd4)
GEN_BENCHMARK_FUNC_SUM(sum_simd8)
GEN_BENCHMARK_FUNC_SUM(sum_simd16)
GEN_BENCHMARK_FUNC_SUM(sum_simd32)

int main(int argc, char **argv) {
  if (argc >= 2) {
    s_arr_size = atoi(argv[1]);
    if (s_arr_size > ARR_CAPACITY) {
      fprintf(stderr, "Array size of '%ld' is too big.\n", s_arr_size);
      return 1;
    }
  }

  s_tsc_freq = get_tsc_freq();

  printf("Time Stamp Counter freq     %ldHz (%.0fMHz)\n",
      s_tsc_freq, s_tsc_freq * 1e-6);

  printf("Array size                  %ld\n", s_arr_size);
  printf("\nBenchmarks\n");

  // touch array and assign some values to it
  for (u64 i = 0; i < s_arr_size; ++i) {
    s_arr[i] =  i * 10;
  }

  // scale number of benchmark iterations on inverse array size
  u64 iter_count = (16000000000u / s_arr_size);

  struct {
    const char *name;
    void (*func)(void);
  } benchmarks[] = {
    {"sum_naive",   benchmark_sum_naive},
    {"sum_unroll2", benchmark_sum_unroll2},
    {"sum_unroll4", benchmark_sum_unroll4},
    {"sum_unroll8", benchmark_sum_unroll8},
    {"sum_simd4",   benchmark_sum_simd4},
    {"sum_simd8",   benchmark_sum_simd8},
    {"sum_simd16",  benchmark_sum_simd16},
    {"sum_simd32",  benchmark_sum_simd32},
  };

  for (u64 i = 0; i < ARRAY_SIZE(benchmarks); ++i) {
    benchmark_print_result(
      benchmarks[i].name,
      (f64)benchmark(benchmarks[i].func, iter_count),
      s_arr_size);
  }

  return 0;
}
