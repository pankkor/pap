#pragma once

#include "timer.h"

#ifndef PROFILER_ENABLED

#define PROFILER_BEGIN()
#define PROFILER_END()
#define PROFILER_PRINT_STATS(cpu_timer_freq, csv)

#define PROFILE_FUNC_BEGIN(bytes)
#define PROFILE_FUNC_END()

#define PROFILE_ZONE_BEGIN(name, bytes)
#define PROFILE_ZONE_END(name)

#define PROFILE_ZONE_BEGIN_V(name, bytes, var)
#define PROFILE_ZONE_END_V(var)

#define PROFILE_FUNC(bytes)
#define PROFILE_ZONE(name, bytes)

#define PROFILER_USED_ZONE_COUNT_STATIC_ASSERT

#else

#include "types.h"

#ifndef PROFILER_ZONES_SIZE_MAX
// Zone index 0 - unused
// Zone index 1 - Main zone between `profile_begin()` and `profile_end()`
#define PROFILER_ZONES_SIZE_MAX   4096
#endif // #ifndef PROFILE_ZONES_SIZE_MAX

#define PROFILER_BEGIN()          profiler_begin()
#define PROFILER_END()            profiler_end()
#define PROFILER_PRINT_STATS(cpu_timer_freq, csv) \
  profiler_print_stats(cpu_timer_freq, csv)

// BEGIN/END macros
#define PROFILE_ZONE_BEGIN(name, bytes)  PROFILE_ZONE_BEGIN_V(name, bytes, tmp_profile_zone_)
#define PROFILE_ZONE_END(name)    PROFILE_ZONE_END_V(tmp_profile_zone_)

#define PROFILE_FUNC_BEGIN(bandwidth)  \
  PROFILE_ZONE_BEGIN_V(FUNC_NAME, bytes, tmp_profile_func_zone_)
#define PROFILE_FUNC_END()  \
  PROFILE_ZONE_END_V(FUNC_NAME, tmp_profile_func_zone_)

// Accepts custom name for temp zone variable
#define PROFILE_ZONE_BEGIN_V(name, bytes, var)  \
  struct profiler_zone_mark var                 \
    = profiler_zone_begin(__COUNTER__ + 2, name, bytes)
#define PROFILE_ZONE_END_V(var)   profiler_zone_end(&var)

// Scoped macros using gcc attribute cleanup extension
#define PROFILE_FUNC(bytes)       PROFILE_ZONE(FUNC_NAME, bytes)
#define PROFILE_ZONE(name, bytes)                             \
  __attribute__((unused)) CLEANUP(cleanup_profiler_zone_end)  \
  struct profiler_zone_mark XCONCAT(tmp_p_zone_, __LINE__)    \
    = profiler_zone_begin(__COUNTER__ + 2, name, bytes)

#define PROFILER_USED_ZONE_COUNT_STATIC_ASSERT                \
  static_assert(__COUNTER__ + 1 < PROFILER_ZONES_SIZE_MAX,    \
      "Number of profile zones exceeds size of profiler zones array");

struct profiler_zone_mark {
  const char *name;
  u64 begin_tsc;
  u64 prev_total_tsc;
  u32 index;
  u32 parent_index;
  u64 bytes;
};

// Start profiling main zone
void profiler_begin(void);

// Finish profiling main zone
void profiler_end(void);

// Start profiler zone with name, at `index` into profile zones array and
// bytes to process
struct profiler_zone_mark profiler_zone_begin(u32 index, const char *name,
    u64 bytes);

// End profiler zone
void profiler_zone_end(struct profiler_zone_mark *mark);

// Print profile stats to stderr
// Prints in .csv format if `csv` is `true`.
// Prints additional time in seconds if cpu_timer_freq is not zero
void profiler_print_stats(u64 cpu_timer_freq, b32 csv);

static FORCE_INLINE void cleanup_profiler_zone_end(
    struct profiler_zone_mark *mark) {
  profiler_zone_end(mark);
}

#endif // #ifndef PROFILER_ENABLED
