#include "tester.h"
#include "timer.h"

#include <stdio.h>    // fprintf stderr

b32 tester_step(struct tester *tester) {
  u64 current_tsc = read_cpu_timer();

  switch (tester->run.state) {
    case TESTER_STATE_NOT_RUN:
      tester->run = (struct tester_run){0};
      tester->run.stats.min_tsc = (u64)-1;
      tester->run.begin_tsc = current_tsc;
      tester->run.state = TESTER_STATE_RUNNING;
      break;

    case TESTER_STATE_RUNNING: {
      if (tester->run.open_zone_count != 0) {
        tester_error(tester, "Unbalanced tester_zone_begin/end");
        break;
      }

      if (tester->run.stats.min_tsc > tester->run.step_elapsed_tsc) {
        tester->run.stats.min_tsc = tester->run.step_elapsed_tsc;
        tester->run.begin_tsc = current_tsc; // reset running timer
      }

      if (tester->run.stats.max_tsc < tester->run.step_elapsed_tsc) {
        tester->run.stats.max_tsc = tester->run.step_elapsed_tsc;
      }

      if (tester->extected_bytes &&
          tester->extected_bytes != tester->run.step_bytes) {
        tester_error(tester, "Processed bytes count mismatch");
      }

      tester->run.stats.bytes = tester->run.step_bytes;

      tester->run.stats.step_count += 1;
      tester->run.stats.total_tsc += tester->run.step_elapsed_tsc;

     // reset step specific counters
      tester->run.step_bytes = 0;
      tester->run.step_elapsed_tsc = 0;

      if (current_tsc - tester->run.begin_tsc > tester->try_duration_tsc) {
        tester->run.state = TESTER_STATE_COMPLETED;
      }
      break;
    }

    case TESTER_STATE_COMPLETED:
    case TESTER_STATE_ERROR:
      break;
  }
  return tester->run.state == TESTER_STATE_RUNNING;
}

void tester_zone_begin(struct tester *tester) {
  tester->run.open_zone_count += 1;
  tester->run.step_elapsed_tsc -= read_cpu_timer();
}

void tester_count_bytes(struct tester *tester, u64 bytes) {
  tester->run.step_bytes += bytes;
}

void tester_zone_end(struct tester *tester) {
  tester->run.open_zone_count -= 1;
  tester->run.step_elapsed_tsc += read_cpu_timer();
}

void tester_error(struct tester *tester, const char *error_message) {
    tester->run.state = TESTER_STATE_ERROR;
    tester->run.error_message = error_message;
}

static void tester_print_tsc(const char *label, u64 tsc, u64 cpu_timer_freq,
    u64 bytes, b32 csv) {
  f32 sec = (f32)tsc / cpu_timer_freq;
  f32 ms  = sec * 1e3f;
  f32 gb_p_sec = bytes / (sec * 1024 * 1024 * 1024); // bandwidth

  fprintf(stderr, csv ? "%s,%lu,%f,%f\n" : "%-10s|%10lu|%10.5f|%10.5f\n",
      label, tsc, ms, gb_p_sec);
}

void tester_starts_print_titles(b32 csv) {
  fprintf(stderr, csv ? "%s,%s,%s,%s\n" : "%-10s|%10s|%10s|%10s\n",
      "Stat", "tsc", "ms", "GB/s");
}

void tester_stats_print(struct tester_stats stats, u64 cpu_timer_freq, b32 csv) {
  u64 avg_tsc = stats.total_tsc / stats.step_count;

  tester_print_tsc("Min", stats.min_tsc,  cpu_timer_freq, stats.bytes, csv);
  tester_print_tsc("Max", stats.min_tsc,  cpu_timer_freq, stats.bytes, csv);
  tester_print_tsc("Avg", avg_tsc,        cpu_timer_freq, stats.bytes, csv);
}

void tester_print(struct tester *tester, u64 cpu_timer_freq) {
  switch (tester->run.state) {
    case TESTER_STATE_NOT_RUN:
      fprintf(stderr, "Tester: not running. Call `tester_step()` to begin.\n");
      break;

    case TESTER_STATE_RUNNING:
      fprintf(stderr, "Tester: running...\n");
      break;

    case TESTER_STATE_COMPLETED: {
      fprintf(stderr, "Tester: completed\n");
      fprintf(stderr, "%-24s", "CPU timer frequency: ");

      if (cpu_timer_freq) {
        fprintf(stderr, "%-4.2f MHz\n", cpu_timer_freq * 1e-6f);
      } else {
        fprintf(stderr, "???\n");
      }

      fprintf(stderr, "%-24s", "Processed bytes: ");
      if (cpu_timer_freq) {
        fprintf(stderr, "%-4.2f MB\n",
            (f32)tester->run.stats.bytes / 1024.0f / 1024.0f);
      } else {
        fprintf(stderr, "???\n");
      }

      b32 is_csv = false;
      fprintf(stderr, "\n");
      tester_starts_print_titles(is_csv);
      fprintf(stderr,"-------------------------------------------\n");
      tester_stats_print(tester->run.stats, cpu_timer_freq, is_csv);
      fprintf(stderr, "\n");
      break;
    }

    case TESTER_STATE_ERROR:
      fprintf(stderr, "Tester: error '%s'\n", tester->run.error_message);
      break;
  }
}
