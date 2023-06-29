#include "profiler.h"

#include "timer.h"

#include <assert.h>   // assert
#include <stdio.h>    // fprintf fileno
#include <string.h>   // strcmp

struct profile_stat {
  const char *name;
  u64 begin_tsc;
  u64 end_tsc;
};

static struct profile_stat s_main_stat;
static struct profile_stat s_stats[PROFILE_FRAMES_SIZE_MAX];

static void profile_print_stat(struct profile_stat *stat, u64 tsc_total,
    u64 cpu_timer_freq) {
  u64 tsc = stat->end_tsc - stat->begin_tsc;

  if (cpu_timer_freq) {
    fprintf(stderr, "%-32s %10lutsc, %4.4fs (%4.2f%%)\n", stat->name,
        tsc, (f32)tsc / cpu_timer_freq, (f32)tsc / tsc_total * 100);
  } else {
    fprintf(stderr, "%-32s %10lutsc, (%4.2f%%)\n", stat->name,
        tsc, (f32)tsc / tsc_total * 100);
  }
}

void profile_print_stats(u64 cpu_timer_freq) {
  if (!s_main_stat.name) {
    fprintf(stderr, "Profiler error: profile_begin() was never called\n");
    return;
  }
  if (!s_main_stat.end_tsc) {
    fprintf(stderr, "Profiler error: profile_end() was never called\n");
    return;
  }

  fprintf(stderr, "----------------------------------------\n");
  fprintf(stderr, "Instrumentation Profiler Stats\n");
  fprintf(stderr, "----------------------------------------\n");
  fprintf(stderr, "CPU timer frequency: %luf (%.2fMHz)\n",
      cpu_timer_freq, cpu_timer_freq * 1e-6f);
  fprintf(stderr, "\n");
  fprintf(stderr, "Frames\n");
  fprintf(stderr, "--------------\n");

  u64 total_tsc = s_main_stat.end_tsc - s_main_stat.begin_tsc;

  for (u64 i = 0; i < PROFILE_FRAMES_SIZE_MAX; ++i) {
    if (s_stats[i].name && s_stats[i].end_tsc) {
      profile_print_stat(&s_stats[i], total_tsc, cpu_timer_freq);
    }
  }
  profile_print_stat(&s_main_stat, total_tsc, cpu_timer_freq);
  fprintf(stderr, "----------------------------------------\n");
}

void profile_begin(void) {
  s_main_stat = (struct profile_stat){"TOTAL", read_cpu_timer(), 0};
}

void profile_end(void) {
  s_main_stat.end_tsc = read_cpu_timer();
}

u64 profile_frame_begin(u64 h, const char *name) {
  assert(h < PROFILE_FRAMES_SIZE_MAX && "Frame handle out of bounds");
  s_stats[h] = (struct profile_stat){name, read_cpu_timer(), 0};
  return h;
}

void profile_frame_end(u64 h) {
  assert(h < PROFILE_FRAMES_SIZE_MAX && "Frame handle out of bounds");
  assert(s_stats[h].begin_tsc != 0 && "Ending frame, that has not began");
  s_stats[h].end_tsc = read_cpu_timer();
}
