#pragma once

#include "types.h"

#ifndef PROFILE_ENABLED

#else

#endif // #ifndef PROFILE_ENABLED

#ifndef PROFILER_ZONES_SIZE_MAX
// Zone index 0 - unused
// Zone index 1 - Main zone between `profile_begin()` and `profile_end()`
#define PROFILER_ZONES_SIZE_MAX 1024
#endif // #ifndef PROFILE_ZONES_SIZE_MAX

// BEGIN/END macroses
#define PROFILE_FUNC_BEGIN()      PROFILE_ZONE_BEGIN_V(FUNC_NAME, tmp_profile_func_zone_)
#define PROFILE_FUNC_END()        PROFILE_ZONE_END_V(FUNC_NAME, tmp_profile_func_zone_)

#define PROFILE_ZONE_BEGIN(name)  PROFILE_ZONE_BEGIN_V(name, tmp_profile_zone_)
#define PROFILE_ZONE_END(name)    PROFILE_ZONE_END_V(tmp_profile_zone_)

// Accepts custom name for temp zone variable
#define PROFILE_ZONE_BEGIN_V(name, var)                                       \
  struct profiler_zone_mark var = profiler_zone_begin(__COUNTER__ + 2, name)

#define PROFILE_ZONE_END_V(var)   profiler_zone_end(&var)

// Scoped macroses using gcc attribute cleanup extension
#define PROFILE_FUNC()            PROFILE_ZONE(FUNC_NAME)
#define PROFILE_ZONE(name) \
  __attribute__((unused)) CLEANUP(cleanup_profiler_zone_end)                  \
  struct profiler_zone_mark XCONCAT(tmp_p_zone_, __LINE__)                    \
    = profiler_zone_begin(__COUNTER__ + 2, name)

#define PROFILER_USED_ZONE_COUNT_STATIC_ASSERT()                              \
  static_assert(__COUNTER__ + 1 < PROFILER_ZONES_SIZE_MAX,                    \
      "Number of profile zones exceeds size of profiler zones array")

struct profiler_zone_mark {
  const char *name;
  u64 begin_tsc;
  u64 prev_total_tsc;
  u32 index;
  u32 parent_index;
};

// Start profiling main zone
void profiler_begin(void);

// Finish profiling main zone
void profiler_end(void);

// Start profiler zone with name, at `index` into profile zones array
struct profiler_zone_mark profiler_zone_begin(u32 index, const char *name);

// End profiler zone
void profiler_zone_end(struct profiler_zone_mark *begin_stat);

// Print profile stats to stderr
// Prints in .csv format if `csv` is `true`.
// Prints additional time in seconds if cpu_timer_freq is not zero
void profiler_print_stats(u64 cpu_timer_freq, b32 csv);

static FORCE_INLINE void cleanup_profiler_zone_end(
    struct profiler_zone_mark *mark) {
  profiler_zone_end(mark);
}
