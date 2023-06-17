// Performance-Aware-Programming Course
// https://www.computerenhance.com/p/table-of-contents
//
// Part 2
// Estimate CPU time stamp counter frequency

// Begin unity build
#include "timer.c"
// End unity build

#include "types.h"
#include "timer.h"

#include <stdio.h>      // printf
#include <stdlib.h>     // atoi

void print_usage(void) {
  fprintf(stderr, "Usage:\nestimate_cpu_timer_freq <time_to_run_ms>\n");
}

int main(int argc, char **argv) {
  if (argc < 2) {
    print_usage();
    return 1;
  }

  u64 time_to_run_ms = atol(argv[1]);
  f32 time_to_run_s = (f32)time_to_run_ms * 1e-3f;

  u64 os_timer_freq = get_os_timer_freq();
  f32 time_to_run_os_tsc = time_to_run_s * os_timer_freq;

  printf("Estimating CPU timer frequency for %lums (%.3fs)...\n",
      time_to_run_ms, time_to_run_s);

  u64 elapsed_os_tsc = 0;
  u64 begin_os_tsc = read_os_timer();
  u64 begin_cpu_tsc = read_cpu_timer();

  while (elapsed_os_tsc < time_to_run_os_tsc) {
    elapsed_os_tsc = read_os_timer() - begin_os_tsc;
  }

  u64 end_cpu_tsc = read_cpu_timer();

  f32 elapsed_time_s = (f32)elapsed_os_tsc / os_timer_freq;
  u64 cpu_timer_freq = (end_cpu_tsc - begin_cpu_tsc) / elapsed_time_s;
  printf("Estimated CPU timer frequency:        %luf (%.2fMHz)\n",
      cpu_timer_freq, cpu_timer_freq * 1e-6f);

  // Migth just return cpu frequency from the register without estimating
  u64 cpu_timer_freq_2 = estimate_cpu_timer_freq(time_to_run_ms);
  printf("Result of estimate_cpu_timer_freq(s): %luf (%.2fMHz)\n",
      cpu_timer_freq_2, cpu_timer_freq_2 * 1e-6f);

  return 0;
}
