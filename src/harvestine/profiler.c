#include "profiler.h"

#ifdef PROFILER_ENABLED

#include <assert.h>   // assert
#include <stdio.h>    // fprintf stderr

static const char * const s_delim100 =
  "--------------------------------------------------"
  "--------------------------------------------------\n";

struct profiler_zone {
  const char *name;
  u64 hit_count;
  u64 self_tsc;   // excludes elapsed children
  u64 total_tsc;  // includes elapsed children
};

static struct profiler_zone s_zones[PROFILER_ZONES_SIZE_MAX];
static struct profiler_zone_mark s_total_mark;
static u64 s_last_zone_index;

void profiler_begin(void) {
  s_total_mark = profiler_zone_begin(1, "Main");
}

void profiler_end(void) {
  profiler_zone_end(&s_total_mark);
}

struct profiler_zone_mark profiler_zone_begin(u32 index,
    const char *name) {
  assert(index < PROFILER_ZONES_SIZE_MAX && "Zone index out of bounds");

  u64 prev_total_tsc = s_zones[index].total_tsc;

  struct profiler_zone_mark mark = {
    name,
    read_cpu_timer(),
    prev_total_tsc,
    index,
    s_last_zone_index
  };

  s_last_zone_index = index;
  return mark;
}

void profiler_zone_end(struct profiler_zone_mark *mark) {
  assert(mark->index < PROFILER_ZONES_SIZE_MAX && "Zone index out of bounds");
  assert(mark->begin_tsc != 0 && "Ending zone, that has not began");

  u64 elapsed_tsc = read_cpu_timer() - mark->begin_tsc;

  s_zones[mark->index].name            = mark->name;
  s_zones[mark->index].hit_count       += 1;
  s_zones[mark->index].self_tsc        += elapsed_tsc;
  s_zones[mark->index].total_tsc       = mark->prev_total_tsc + elapsed_tsc;

  s_zones[mark->parent_index].self_tsc -= elapsed_tsc;

  s_last_zone_index = mark->parent_index;
}

static void profiler_print_titles(b32 csv) {
  fprintf(stderr, csv ?  "%s"   :  "%-30s",  "Zone");
  fprintf(stderr, csv ? ",%s"   : "|%9s",    "Hits #");
  fprintf(stderr, csv ? ",%s"   : "|%12s",   "Total tsc");
  fprintf(stderr, csv ? ",%s"   : "|%9s",    "Total s");
  fprintf(stderr, csv ? ",%s"   : "|%6s",    "Total%");
  fprintf(stderr, csv ? ",%s"   : "|%12s",   "Self tsc");
  fprintf(stderr, csv ? ",%s"   : "|%9s",    "Self s");
  fprintf(stderr, csv ? ",%s"   : "|%6s",    "Self %");
  fprintf(stderr, "\n");
}

static void profiler_print_zone(struct profiler_zone *pf, u64 total_tsc,
    u64 cpu_timer_freq, b32 csv) {

  f32 total_percent = (f32)pf->total_tsc / total_tsc * 100.0f;
  f32 self_percent  = (f32)pf->self_tsc / total_tsc * 100.0f;
  f32 total_sec     = (f32)pf->total_tsc / cpu_timer_freq;
  f32 self_sec      = (f32)pf->self_tsc / cpu_timer_freq;

  fprintf(stderr, csv ?  "%s"   :  "%-30s",  pf->name);
  fprintf(stderr, csv ? ",%lu"  : "|%9lu",   pf->hit_count);
  fprintf(stderr, csv ? ",%lu"  : "|%12lu",  pf->total_tsc);
  fprintf(stderr, csv ? ",%f"   : "|%9.5f",  total_sec);
  fprintf(stderr, csv ? ",%f"   : "|%6.2f",  total_percent);
  fprintf(stderr, csv ? ",%lu"  : "|%12lu",  pf->self_tsc);
  fprintf(stderr, csv ? ",%f"   : "|%9.5f",  self_sec);
  fprintf(stderr, csv ? ",%f"   : "|%6.2f",  self_percent);
  fprintf(stderr, "\n");
}

void profiler_print_stats(u64 cpu_timer_freq, b32 csv) {
  u64 total_tsc = s_zones[1].total_tsc;
  f32 total_sec = (f32)total_tsc / cpu_timer_freq;

  if (!csv) {
    fprintf(stderr, s_delim100);
    fprintf(stderr, "Instrumentation Profiler Stats\n");
    fprintf(stderr, s_delim100);
    fprintf(stderr, "%-24s", "CPU timer frequency: ");
    if (cpu_timer_freq) {
      fprintf(stderr, "%-4.2fMHz\n", cpu_timer_freq * 1e-6f);
    } else {
      fprintf(stderr, "???\n");
    }

    fprintf(stderr, "%-24s", "Total time: ");
    if (total_tsc) {
      fprintf(stderr, "%-12lu (%9.5f sec)\n", total_tsc, total_sec);
    } else {
      fprintf(stderr, "??? [!] profiler_begin() / profiler_end() not called\n");
    }
    fprintf(stderr, "\n");
    fprintf(stderr, s_delim100);
  }

  profiler_print_titles(csv);
  if (!csv) {
    fprintf(stderr, s_delim100);
  }

  for (u64 i = 1; i < PROFILER_ZONES_SIZE_MAX; ++i) {
    if (s_zones[i].name) {
      profiler_print_zone(&s_zones[i], total_tsc, cpu_timer_freq, csv);
    }
  }

  if (!csv) {
    fprintf(stderr, s_delim100);
    fprintf(stderr, "\n");
  }
}
#endif // #ifdef PROFILER_ENABLED
