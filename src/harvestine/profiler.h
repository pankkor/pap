#pragma once

#include "types.h"

#define PROFILE_FUNC() PROFILE_FRAME(FUNC_NAME)

#define PROFILE_FRAME(name)                                             \
  __attribute__((unused)) CLEANUP(cleanup_profile_frame_end) volatile   \
  u64 XCONCAT(tmp_p_frame_, __COUNTER__) = profile_frame_begin(name)

enum {PROFILE_FRAMES_SIZE_MAX = 1024 * 1024};

// start timing main frame
void profile_begin(void);

// Finish profiling main frame
void profile_end(void);

// Print profile stats to stderr
// Print additional time in seconds if cpu_timer_freq is not zero
void profile_print_stats(u64 cpu_timer_freq);

// Start profiler frame with name, at position
// Returns a handle to the frame that began
u64 profile_frame_begin(const char *s);

// Start profiler frame with name, referenced by a handle
void profile_frame_begin_h(u64 h, const char *frame_name);

// Start profiler frame referenced by a static string
void profile_frame_end(const char *s);

// End profiler frame referenced by a handle
void profile_frame_end_h(u64 h);

// Retrieve tsc stats for profiler frame by a static string
u64 profile_frame_get_tsc(const char *s);

// Get profiler frame stats referenced by a handle
u64 profile_frame_get_elapsed_tsc_h(u64 h);

static FORCE_INLINE void cleanup_profile_frame_end(volatile u64 *h) {
  profile_frame_end_h(*h);
}
