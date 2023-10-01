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
#include <stdlib.h>     // atol
#include <string.h>     // strcmp

static void print_usage(void) {
  fprintf(stderr, "Usage:\n    estimate_cpu_timer_freq <time_to_run_ms>\n");
}

int main(int argc, char **argv) {
  if (argc > 1 && strcmp(argv[1], "-h") == 0) {
    print_usage();
    return 0;
  }
  if (argc < 2) {
    print_usage();
    return 1;
  }

  u64 time_to_run_ms = atol(argv[1]);

  u64 os_timer_freq = get_os_timer_freq();
  printf("OS timer frequency:                   %luf (%.2fMHz)\n",
      os_timer_freq, os_timer_freq * 1e-6f);

  u64 estimated_cpu_timer_freq = estimate_cpu_timer_freq(time_to_run_ms);
  printf("Estimated CPU timer frequency:        %luf (%.2fMHz)\n",
      estimated_cpu_timer_freq, estimated_cpu_timer_freq * 1e-6f);

  u64 cpu_timer_freq = get_cpu_timer_freq();
  printf("CPU timer frequency from the system:  %luf (%.2fMHz)\n",
      cpu_timer_freq, cpu_timer_freq * 1e-6f);

  return 0;
}
