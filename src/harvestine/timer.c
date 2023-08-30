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
  return 1e9;
}

u64 read_os_timer(void) {
  struct timespec tp;
  if (UNLIKELY(clock_gettime(CLOCK_MONOTONIC_RAW, &tp))) {
    return -1;
  }
  return tp.tv_sec * 1e9 + tp.tv_nsec;
}

#else

#include <sys/time.h>

u64 get_os_timer_freq(void) {
  return 1e6;
}

u64 read_os_timer(void) {
  struct timeval tv;
  if (UNLIKELY(gettimeofday(&tv, NULL))) {
    return -1;
  }
  return tv.tv_sec * 1e6 + tv.tv_usec;
}

#endif // #if _WIN32

u64 estimate_cpu_timer_freq(u64 time_to_run_ms) {
  f32 time_to_run_s = (f32)time_to_run_ms * 1e3f;

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
}

u64 get_or_estimate_cpu_timer_freq(u64 time_to_run_ms) {
  u64 cpu_timer_freq = get_cpu_timer_freq();
  if (!cpu_timer_freq) {
    cpu_timer_freq = estimate_cpu_timer_freq(time_to_run_ms);
  }
  return cpu_timer_freq;
}
