#pragma once

#include "types.h"

// Repetition tester
// Repeatedly perfom tests to get min and max elapsed times.
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

enum tester_state {
  TESTER_STATE_NOT_RUN = 0,
  TESTER_STATE_RUNNING,
  TESTER_STATE_COMPLETED,
  TESTER_STATE_ERROR,
};

struct tester_stats {
  u64 step_count;
  u64 total_tsc;
  u64 max_tsc;
  u64 min_tsc;
  u64 bytes;
};

// Print tester stats
// Prints in .csv format if `csv` is `true`.
void tester_stats_print(struct tester_stats stats, u64 cpu_timer_freq, b32 csv);

// Tester internal run data. You can get `stats` from here.
// 0 is initialization
struct tester_run {
  struct tester_stats stats;  // run results
  u64 step_elapsed_tsc;       // tsc elapsed for this step
  u64 step_bytes;             // number of bytes accumulated on this step
  u64 begin_tsc;              // tsc at which tester run started
  i64 open_zone_count;        // safe check for unbalanced begin/end
  const char *error_message;
  enum tester_state state;
};

// Repetition tester
// 0-initialize `.run` for a new run or set `run.state = TESTER_STATE_NOT_RUN`
struct tester {
  u64 try_duration_tsc;       // reset on finding new minimum time
  u64 extected_bytes;         // expect this amount of bytes to be provided
                              // during test step via tester_count_bytes()
  struct tester_run run;      // initialize to {0} before for a new run
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
