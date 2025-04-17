#pragma once

#include "types.h"

u64 get_os_timer_freq(void);
u64 read_os_timer(void);

// Returns timer frequency without esitmation.
// AArch64 implementation immediately returns the value from cntfrq_el0 register
// x86_64 returns 0
u64 get_cpu_timer_freq(void);
u64 read_cpu_timer(void);

// Estimate CPU timer frequency running up to `time_to_run_ms` milliseconds
u64 estimate_cpu_timer_freq(u64 time_to_run_ms);

// Get cpu timer frequency. If it's not available
// Estimate CPU timer frequency running up to `time_to_run_ms` milliseconds
u64 get_or_estimate_cpu_timer_freq(u64 time_to_run_ms);

#ifdef __aarch64__

FORCE_INLINE u64 get_cpu_timer_freq(void) {
  u64 val;
  __asm__ volatile("mrs %0, cntfrq_el0" : "=r" (val));
  return val;
}

FORCE_INLINE u64 read_cpu_timer(void) {
  u64 val;
  // use isb to avoid speculative read of cntvct_el0
  __asm__ volatile("isb;\n\tmrs %0, cntvct_el0" : "=r" (val) :: "memory");
  return val;
}

#elif defined(__x86_64__)
#include <x86intrin.h>

FORCE_INLINE u64 get_cpu_timer_freq(void) {
  return 0;
}

FORCE_INLINE u64 read_cpu_timer(void) {
  return __rdtsc();
}

#else
#error Unsupported architecture
#endif // #ifdef __aarch64__ #elif defined(__x86_64__)
