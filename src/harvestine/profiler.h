#pragma once

#include "types.h"

#define PROFILE_FUNC() PROFILE_FRAME(FUNC_NAME)

#define PROFILE_FRAME(name) \
  __attribute__((unused)) CLEANUP(cleanup_profile_frame_end) volatile   \
  u64 XCONCAT(tmp_p_frame_, __LINE__) = profile_frame_begin(__COUNTER__, name)

enum {PROFILE_FRAMES_SIZE_MAX = 1024 * 1024};

// Start profiling main frame
void profile_begin(void);

// Finish profiling main frame
void profile_end(void);

// Start profiler frame with name, referenced by a handle
// Returns the same handler that was passed
u64 profile_frame_begin(u64 h, const char *name);

// End profiler frame referenced by a handle
void profile_frame_end(u64 h);

// Print profile stats to stderr
// Print additional time in seconds if cpu_timer_freq is not zero
void profile_print_stats(u64 cpu_timer_freq);

static FORCE_INLINE void cleanup_profile_frame_end(volatile u64 *h) {
  profile_frame_end(*h);
}
