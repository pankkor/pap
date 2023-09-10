#pragma once

#include "types.h"

// Repetition tester
// Repeatedly perfom tests to get statistic values
//
// Usage:
//  struct tester t = {
//    .try_duration_tsc = seconds * cpu_timer_freq,
//  };
//
//  // Note that tester_step should be called before anything else
//  while (tester_step(&t)) {
//    ...
//    tester_zone_begin(&t);
//    your_function_under_test(&t);
//    tester_zone_end(&t);
//
//    if (...) {
//      tester_error(&t, "something bad happended");
//    }
//  }
//  tester_print(&t);

// stat values
enum tester_value
{
  TESTER_VALUE_STEP_COUNT,
  TESTER_VALUE_TSC,
  TESTER_VALUE_BYTES,            // number of bytes accumulated
  TESTER_VALUE_MEM_PAGE_FAULTS,

  TESTER_VALUE_COUNT,
};

struct tester_values {
  u64 e[TESTER_VALUE_COUNT];
};

// tester accumulated stats across run
struct tester_stats {
  struct tester_values total;         // values with total, min and max elapsed
  struct tester_values max;           // tsc
  struct tester_values min_plus_one;  // we want to initialize `min` to u64_max,
                                      // for 0-init use `min + 1` instead, since
                                      // u64_int + 1 == 0
};

// Print tester stats
// Prints in .csv format if `csv` is `true`.
void tester_stats_print(struct tester_stats *stats, u64 cpu_timer_freq, b32 csv);

enum tester_state {
  TESTER_STATE_START_RUN = 0,
  TESTER_STATE_RUNNING,
  TESTER_STATE_COMPLETED,
  TESTER_STATE_ERROR,
};
// Tester internal run data.
// 0 initializable
struct tester_run {
  struct tester_values step_values; // per step values stats
  u64 begin_tsc;                    // tsc at which tester run started
  i64 open_zone_count;              // safe check for unbalanced begin/end
  const char *error_message;
  enum tester_state state;
};

// Repetition tester
// 0-initialize `run` for a new run or set `run.state = TESTER_STATE_START_RUN`
// `stats` are accumulated across different runs; reset to `{0}` if needed
struct tester {
  u64 try_duration_tsc;       // reset on finding new minimum time
  u64 expected_bytes;         // expect this amount of bytes to be provided
                              // during test step via tester_count_bytes()
  struct tester_stats stats;  // accumulated statistics across runs
  struct tester_run run;      // initialize to {0} for a new run
};

// Calculates run stats and advances to the next iteration
// Returns 1 when testing should continue, returns 0 when testing has completed
// or failed
b32 tester_step(struct tester *tester);

// Begin time zone
void tester_zone_begin(struct tester *tester);

// End time zone
void tester_zone_end(struct tester *tester);

// Add bytes count for bandwidth stats
void tester_count_bytes(struct tester *tester, u64 bytes);

// Set tester to error state
void tester_error(struct tester *tester, const char *error);

// Print tester results
void tester_print(struct tester *tester, u64 cpu_timer_freq);
