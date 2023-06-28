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

static struct profile_stat s_stats[PROFILE_FRAMES_SIZE_MAX];
static u64 s_stats_size = 0;

static u64 profile_frame_name_to_handle(const char *s) {
  for (u64 i = 0; i < s_stats_size; ++i) {
    if (strcmp(s, s_stats[i].name) == 0) {
      return i;
    }
  }
  return U64_MAX;
}

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

void profile_begin(void) {
  profile_frame_begin_h(0, "Total");
  ++s_stats_size;
}

void profile_end(void) {
  profile_frame_end_h(0);
}

void profile_print_stats(u64 cpu_timer_freq) {
  if (!s_stats[0].name) {
    fprintf(stderr, "Profiler error: profile_start() was never called\n");
    return;
  }
  if (!s_stats[0].end_tsc) {
    fprintf(stderr, "Profiler error: profile_finish() was never called\n");
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

  u64 total_tsc = s_stats[0].end_tsc - s_stats[0].begin_tsc;

  for (u64 i = 0; i < PROFILE_FRAMES_SIZE_MAX; ++i) {
    if (s_stats[i].name && s_stats[i].end_tsc) {
      profile_print_stat(&s_stats[i], total_tsc, cpu_timer_freq);
    }
  }
  fprintf(stderr, "----------------------------------------\n");
}

void profile_frame_begin_h(u64 h, const char *frame_name) {
  assert(h < PROFILE_FRAMES_SIZE_MAX && "Frame handle out of bounds");
  s_stats[h] = (struct profile_stat){
    frame_name,
    read_cpu_timer(),
    0,
  };
}

u64 profile_frame_begin(const char *frame_name) {
  profile_frame_begin_h(s_stats_size, frame_name);
  return s_stats_size++;
}

void profile_frame_end(const char *frame_name) {
  u64 h = profile_frame_name_to_handle(frame_name);
  profile_frame_end_h(h);
}

void profile_frame_end_h(u64 h) {
  u64 tsc = read_cpu_timer();

  assert(h < s_stats_size);
  if (h >= s_stats_size) {
    return;
  }
  assert(s_stats[h].begin_tsc != 0
      && "Profiler: ending frame, that has not began");

  s_stats[h].end_tsc = tsc;
}

u64 profile_get_elapsed_tsc(const char *frame_name) {
  i64 h = profile_frame_name_to_handle(frame_name);
  return profile_frame_get_elapsed_tsc_h(h);
}

u64 profile_frame_get_elapsed_tsc_h(u64 h) {
  assert(h < s_stats_size);
  if (h >= s_stats_size) {
    return 0;
  }
  return s_stats[h].end_tsc - s_stats[h].begin_tsc;
}
