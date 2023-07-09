#pragma once

#include "types.h"

#define PROFILE_FUNC() PROFILE_FRAME(FUNC_NAME)

#define PROFILE_FRAME(name) \
  __attribute__((unused)) CLEANUP(cleanup_profile_frame_end)              \
  struct profile_frame_begin_mark XCONCAT(tmp_p_frame_, __LINE__)         \
    = profile_frame_begin(__COUNTER__ + 2, name)

#define PROFILE_USED_FRAME_COUNT_STATIC_ASSERT()                          \
  static_assert(__COUNTER__ + 1 < PROFILE_FRAMES_SIZE_MAX,                \
      "Number of profile frames exceeds size of profiler frames array")

// Frame index 0 - unused
// Frame index 1 - Main frame between `profile_begin()` and `profile_end()`
enum {PROFILE_FRAMES_SIZE_MAX = 1024 * 1024};

struct profile_frame_begin_mark {
  const char *name;
  u64 begin_tsc;
  u64 prev_total_tsc;
  u64 index; // TODO u32
  u64 parent_index;
};

// Start profiling main frame
void profile_begin(void);

// Finish profiling main frame
void profile_end(void);

// Start profiler frame with name, at `index` into profile frames array
struct profile_frame_begin_mark profile_frame_begin(u64 index, const char *name);

// End profiler frame
void profile_frame_end(struct profile_frame_begin_mark *begin_stat);

// Print profile stats to stderr
// Prints in .csv format if `csv` is `true`.
// Prints additional time in seconds if cpu_timer_freq is not zero
void profile_print_stats(u64 cpu_timer_freq, b32 csv);

static FORCE_INLINE void cleanup_profile_frame_end(
    struct profile_frame_begin_mark *begin_stat) {
  profile_frame_end(begin_stat);
}
