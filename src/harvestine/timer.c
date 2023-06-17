#pragma once

#include "timer.h"

#if _WIN32

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

u64 get_os_timer_freq(void) {
  LARGE_INTEGER freq;
  QueryPerformanceFrequency(&freq);
  return freq.QuadPart;
}

u64 read_os_timer(void) {
  LARGE_INTEGER value;
  QueryPerformanceCounter(&value);
  return value.QuadPart;
}

#elif __APPLE__

#include <time.h>

u64 get_os_timer_freq(void) {
  return 1000000000;
}

u64 read_os_timer(void) {
  struct timespec tp;
  if (UNLIKELY(clock_gettime(CLOCK_MONOTONIC_RAW, &tp))) {
	  return -1;
  }
  return tp.tv_sec * 1000000000 + tp.tv_nsec;
}

#else

#include <sys/time.h>

u64 get_os_timer_freq_usec(void) {
	return 1000000;
}

u64 read_os_timer_usec(void) {
  struct timeval tv;
  if (UNLIKELY(gettimeofday(&tv, NULL))) {
	return -1;
  }
  return tv.tv_sec * 1000000 + tv.tv_usec;
}

#endif // #if _WIN32

u64 estimate_cpu_timer_freq(u64 time_to_run_ms) {
#ifdef __aarch64__
  (void)time_to_run_ms;
  u64 val;
  __asm__ volatile("mrs %0, cntfrq_el0" : "=r" (val));
  return val;
#else
  f32 time_to_run_s = (f32)time_to_run_ms / 1000;

  u64 os_timer_freq = get_os_timer_freq();
  f32 time_to_run_os_tsc = time_to_run_s * os_timer_freq;

  u64 os_tsc;
  u64 cpu_tsc;
  u64 begin_os_tsc = read_os_timer();
  u64 begin_cpu_tsc = read_cpu_timer();

  do {
    os_tsc = read_os_timer();
    cpu_tsc = read_cpu_timer();
  } while (os_tsc - begin_os_tsc < time_to_run_os_tsc);

  f32 elapsed_time_s = (f32)(os_tsc - begin_os_tsc) / os_timer_freq;
  return (cpu_tsc - begin_cpu_tsc) / elapsed_time_s;
#endif // #ifdef __aarch64__
}

