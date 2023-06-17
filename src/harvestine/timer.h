#pragma once

#include "types.h"

u64 get_os_timer_freq(void);
u64 read_os_timer(void);

// Estimate CPU timer frequency running up to `time_to_run_ms` milliseconds
// AArch64 implementation immediately returns the value from cntfrq_el0 register
u64 estimate_cpu_timer_freq(u64 time_to_run_ms);
FORCE_INLINE u64 read_cpu_timer(void);

#ifdef __aarch64__

FORCE_INLINE u64 read_cpu_timer(void) {
  u64 val;
  // use isb to avoid speculative read of cntvct_el0
  __asm__ volatile("isb;\n\tmrs %0, cntvct_el0" : "=r" (val) :: "memory");
  return val;
}

#elif defined(__x86_64__)
#include <immintrin.h>

FORCE_INLINE u64 read_cpu_timer(void) {
  return __rdtsc();
}

#else
#error Unsupported architecture
#endif // #ifdef __aarch64__ #elif defined(__x86_64__)
