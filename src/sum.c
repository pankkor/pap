// Performance-Aware-Programming Course
// https://www.computerenhance.com/p/table-of-contents
//
// Prologue
// sum of array of u32 numbers

#ifndef __aarch64__
#error Architectures other than AArch64 are not supported
#endif // #ifndef __aarch64__

#include <arm_neon.h>
#include <stdio.h> // printf
#include <stdlib.h> // atoi

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
#define ARRAY_COUNT(x)  (u64)(sizeof(x) / sizeof(x[0]))

#define DSB(option)     __asm__ volatile ("dsb " #option : : : "memory")

#define NO_OPT          __attribute__((optnone))
#define LOOP_NO_OPT    _Pragma(\
    "clang loop unroll(disable) vectorize(disable) interleave(disable)")

static FORCE_INLINE u64 rdtsc(void) {
  u64 val;
  // use isb to avoid speculative read of cntvct_el0
  __asm__ volatile("isb;\n\tmrs %0, cntvct_el0" : "=r" (val) :: "memory");
  return val;
}

static FORCE_INLINE u64 tsc_freq(void) {
  u64 val;
  __asm__ volatile("mrs %0, cntfrq_el0" : "=r" (val));
  return val;
}

static FORCE_INLINE f64 tsc_to_s(f64 tsc) {
  return tsc / tsc_freq();
}

static u64 benchmark(void (func)(void), u64 count) {
  u64 min_cycles = -1;
  for (u64 i = 0; i < count; ++i) {
    u64 beg = rdtsc();

    DSB(nsh);
    func();
    DSB(nsh);

    u64 end = rdtsc();

    u64 cycles = end - beg;
    if (cycles < min_cycles) {
      min_cycles = cycles;
    }
  }
  return min_cycles;
}

// Note: it's not trivial to obtain CPU frequency on Apple silicon.
// Core freqency varies depending on the type of a core,
// how many cores are active within the cluster and core's power state.
// To keep things simple just hardcore 3237Mhz
// (highest frequency value for M1 Pro obtained from IORegistry)
u64 CPU_FREQ_HZ = 3237460535;

static void benchmark_print_result(
    const char *benchmark_name,
    f64 tsc,
    u64 iters) {
  f64 ns = tsc_to_s(tsc * 1e9);
  f64 ipc = 1.0 / tsc_to_s(CPU_FREQ_HZ * tsc);

  printf("--- %s ---\n", benchmark_name);
  printf("Total time                  %.8fns\n", ns);
  printf("Time per iteration          %.8fns\n", ns / iters);
  printf("Instructions per cycle      %.8f\n\n", ipc);
  fflush(stdout);
}

// benchmnarks
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

static FORCE_INLINE u32 sum_neon4(u64 count, u32 * restrict in) {
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

static FORCE_INLINE u32 sum_neon8(u64 count, u32 * restrict in) {
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

static FORCE_INLINE u32 sum_neon16(u64 count, u32 * restrict in) {
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

static FORCE_INLINE u32 sum_neon32(u64 count, u32 * restrict in) {
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

enum { ARR_CAPACITY = 12 * 1024 * 1024 };
ALIGNED(128) u32 s_arr[ARR_CAPACITY] = {0}; // big enough
u64 s_arr_size = 4096;

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
GEN_BENCHMARK_FUNC_SUM(sum_neon4)
GEN_BENCHMARK_FUNC_SUM(sum_neon8)
GEN_BENCHMARK_FUNC_SUM(sum_neon16)
GEN_BENCHMARK_FUNC_SUM(sum_neon32)

int main(int argc, char **argv) {
  if (argc >= 2) {
    s_arr_size = atoi(argv[1]);
    if (s_arr_size > ARR_CAPACITY) {
      fprintf(stderr, "Array size of '%ld' is too big.\n", s_arr_size);
      return 1;
    }
  }

  printf("Approx CPU freq             %ldHz (%.0fMHz)\n",
      CPU_FREQ_HZ, CPU_FREQ_HZ * 1e-6);
  printf("Time Stamp Counter freq     %ldHz (%.0fMHz)\n",
      tsc_freq(), tsc_freq() * 1e-6);

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
    {"sum_neon4",   benchmark_sum_neon4},
    {"sum_neon8",   benchmark_sum_neon8},
    {"sum_neon16",  benchmark_sum_neon16},
    {"sum_neon32",  benchmark_sum_neon32},
  };

  for (u64 i = 0; i < ARRAY_COUNT(benchmarks); ++i) {
    benchmark_print_result(
      benchmarks[i].name,
      (f64)benchmark(benchmarks[i].func, iter_count) / s_arr_size,
      iter_count);
  }

  return 0;
}
