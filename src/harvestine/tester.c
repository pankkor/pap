#include "tester.h"
#include "os.h"
#include "timer.h"

#include <stdio.h>    // fprintf stderr

enum {TESTER_DEFAULT_TRY_DURATION_TSC = 240000000};

static const char * const s_delim =
  "--------------------------------------------------"
  "-----------------";

b32 tester_step(struct tester *tester) {
  u64 current_tsc = read_cpu_timer();

  switch (tester->run.state) {
    case TESTER_STATE_START_RUN:
      if (!tester->try_duration_tsc) {
        tester->try_duration_tsc = TESTER_DEFAULT_TRY_DURATION_TSC;
      }

      tester->run = (struct tester_run){0};
      tester->run.begin_tsc = current_tsc;
      tester->run.state = TESTER_STATE_RUNNING;

      if (!os_perf_init()) {
        tester_error(tester,
            "Failed to initialize performance counters. Try with super user.");
      }
      break;

    case TESTER_STATE_RUNNING: {
      if (tester->run.open_zone_count != 0) {
        tester_error(tester, "Unbalanced tester_zone_begin/end");
        break;
      }

      struct tester_values *step          = &tester->run.step_values;
      struct tester_values *total         = &tester->stats.total;
      struct tester_values *max           = &tester->stats.max;
      struct tester_values *min_plus_one  = &tester->stats.min_plus_one;

      u64 step_tsc = step->e[TESTER_VALUE_TSC];
      step->e[TESTER_VALUE_STEP_COUNT] += 1;

      for (int val_type = 0; val_type < TESTER_VALUE_COUNT; ++val_type) {
        total->e[val_type] += step->e[val_type];
      }

      // adjust (min + 1) back to (min) by substracting 1
      if (min_plus_one->e[TESTER_VALUE_TSC] - 1 > step_tsc) {
        *min_plus_one = *step;
        min_plus_one->e[TESTER_VALUE_TSC] = step_tsc + 1;
        // reset running timer on finding min tsc
        tester->run.begin_tsc = current_tsc;
      }

      if (max->e[TESTER_VALUE_TSC] < step_tsc) {
        *max = *step;
      }

      if (tester->expected_bytes &&
          tester->expected_bytes != step->e[TESTER_VALUE_BYTES]) {
        tester_error(tester, "Processed bytes count mismatch");
      }

      // reset step specific counters
      *step = (struct tester_values){0};

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
  tester->run.step_values.e[TESTER_VALUE_TSC] -= read_cpu_timer();
  tester->run.step_values.e[TESTER_VALUE_MEM_PAGE_FAULTS] -= os_read_page_fault_count();
}

void tester_zone_end(struct tester *tester) {
  tester->run.open_zone_count -= 1;
  tester->run.step_values.e[TESTER_VALUE_TSC] += read_cpu_timer();
  tester->run.step_values.e[TESTER_VALUE_MEM_PAGE_FAULTS] += os_read_page_fault_count();
}

void tester_count_bytes(struct tester *tester, u64 bytes) {
  tester->run.step_values.e[TESTER_VALUE_BYTES] += bytes;
}

void tester_error(struct tester *tester, const char *error_message) {
    tester->run.state = TESTER_STATE_ERROR;
    tester->run.error_message = error_message;
}

static void tester_stats_print_titles(b32 csv) {
  fprintf(
      stderr,
      csv ? "%s,%s,%s,%s,%s,%s\n" : "%-10s|%10s|%10s|%10s|%12s|%10s\n",
      "Stat", "tsc", "ms", "GB/s", "Mem PF", "kB/Mem PF");
}

static void tester_values_print(const char *label, struct tester_values values,
    u64 cpu_timer_freq, b32 csv) {
  f32 step_count  = (f32)values.e[TESTER_VALUE_STEP_COUNT];
  u64 tsc         = values.e[TESTER_VALUE_TSC]              / step_count;
  u64 bytes       = values.e[TESTER_VALUE_BYTES]            / step_count;
  f32 mem_pf      = values.e[TESTER_VALUE_MEM_PAGE_FAULTS]  / step_count;

  f32 sec         = (f32)tsc / cpu_timer_freq;
  f32 ms          = sec * 1e3f;
  f32 gb_p_sec    = bytes / (sec * 1024 * 1024 * 1024); // bandwidth in GB
  f32 kb_p_mem_pf = bytes / (mem_pf * 1024);            // kB per mem page fault

  fprintf(
      stderr,
      csv ? "%s,%llu,%f,%f,%f,%f\n"
          : "%-10s|%10llu|%10.4f|%10.4f|%12.4f|%10.4f\n",
      label, tsc, ms, gb_p_sec, mem_pf, kb_p_mem_pf);
}

void tester_stats_print(struct tester_stats *stats, u64 cpu_timer_freq,
    b32 csv) {
  struct tester_values min = stats->min_plus_one;
  min.e[TESTER_VALUE_TSC] -= 1;

  tester_values_print("Min",  min,           cpu_timer_freq, csv);
  tester_values_print("Max",  stats->max,    cpu_timer_freq, csv);
  tester_values_print("Avg",  stats->total,  cpu_timer_freq, csv);
}

void tester_print(struct tester *tester, u64 cpu_timer_freq) {
  switch (tester->run.state) {
    case TESTER_STATE_START_RUN:
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

      if (tester->expected_bytes) {
        fprintf(stderr, "%-24s", "Bytes to process: ");
        if (cpu_timer_freq) {
          fprintf(stderr, "%-4.2f MB\n",
              (f32)tester->expected_bytes / 1024.0f / 1024.0f);
        } else {
          fprintf(stderr, "???\n");
        }
      }

      fprintf(stderr, "%-24s%-4llu\n",
          "Steps taken: ", tester->stats.total.e[TESTER_VALUE_STEP_COUNT]);

      b32 is_csv = false;
      fprintf(stderr, "\n");
      tester_stats_print_titles(is_csv);
      fprintf(stderr, "%s\n", s_delim);
      tester_stats_print(&tester->stats, cpu_timer_freq, is_csv);
      fprintf(stderr, "\n");
      break;
    }

    case TESTER_STATE_ERROR:
      fprintf(stderr, "Tester: error '%s'\n", tester->run.error_message);
      break;
  }
}
