// Performance-Aware-Programming Course
// https://www.computerenhance.com/p/table-of-contents
//
// Part 3
// Profile read overhead

#include <assert.h>     // assert
#include <stdio.h>      // fprintf fopen fread stderr
#include <stdlib.h>     // malloc free abort
#include <string.h>     // strcmp
#include <sys/stat.h>   // stat

#include "types.h"
#include "os.h"
#include "tester.h"

// Begin unity build
#include "os.c"
#include "timer.c"
#include "tester.c"
// End unity build

struct buf_u8 {
  u8 *data;
  u64 size;
};

// --------------------------------------
// Tests
// --------------------------------------
struct test_param {
  struct buf_u8 buf;
  const char *filepath;
};

enum alloc_type {
  ALLOC_TYPE_NONE,
  ALLOC_TYPE_MALLOC,
  ALLOC_TYPE_VIRTUAL_ALLOC,
  ALLOC_TYPE_VIRTUAL_LARGE_ALLOC,

  ALLOC_TYPE_COUNT,
};

const char * alloc_type_to_cstr(enum alloc_type alloc_type) {
  switch (alloc_type) {
    case ALLOC_TYPE_NONE:                 return "no allocation";
    case ALLOC_TYPE_MALLOC:               return "malloc";
    case ALLOC_TYPE_VIRTUAL_ALLOC:        return "virtual_alloc";
    case ALLOC_TYPE_VIRTUAL_LARGE_ALLOC:  return "virtual_large_alloc";
    case ALLOC_TYPE_COUNT:                return "<error>";
  }
  return "";
}

static void do_allocation(enum alloc_type alloc_type, struct buf_u8 *buf) {
  switch (alloc_type) {
    case ALLOC_TYPE_NONE:
      break;
    case ALLOC_TYPE_MALLOC:
      buf->data = (u8 *)malloc(buf->size);
      break;
    case ALLOC_TYPE_VIRTUAL_ALLOC:
      buf->data = (u8 *)os_virtual_alloc(buf->size);
      break;
    case ALLOC_TYPE_VIRTUAL_LARGE_ALLOC:
      buf->data = (u8 *)os_virtual_large_alloc(&buf->size);
      break;
    case ALLOC_TYPE_COUNT:
      assert(0 && "Unknown allocation type");
      abort();
      break;
  }
}

static b32 do_free(enum alloc_type alloc_type, struct buf_u8 *buf) {
  switch (alloc_type) {
    case ALLOC_TYPE_NONE:
      return true;
    case ALLOC_TYPE_MALLOC:
      free(buf->data);
      buf->data = 0;
      return true;
    case ALLOC_TYPE_VIRTUAL_ALLOC:
    case ALLOC_TYPE_VIRTUAL_LARGE_ALLOC: {
      b32 res = os_virtual_free(buf->data, buf->size);
      buf->data = 0;
      return res;
    }
    case ALLOC_TYPE_COUNT:
      assert(0 && "Unknown allocation type");
      abort();
  }
  return false;
}

static void test_write_all(struct tester *tester, enum alloc_type alloc_type,
    struct test_param *param) {
  struct buf_u8 buf = param->buf;
  u64 touch_size    = param->buf.size;

  while (tester_step(tester)) {
    do_allocation(alloc_type, &buf);
    if (buf.data) {
      tester_zone_begin(tester);
      for (u64 i = 0; i < touch_size; ++i) {
        buf.data[i] = (u8)i; // write something
      }
      tester_zone_end(tester);

      tester_count_bytes(tester, touch_size);

      if (!do_free(alloc_type, &buf)) {
        os_print_last_error("Error: memory free failed");
        tester_error(tester, "Error: memory free failed");
      }
    } else {
      os_print_last_error("Error: memory allocation failed");
      tester_error(tester, "Error: memory allocation failed");
    }
  }
}

static void test_write_all_backwards(struct tester *tester,
    enum alloc_type alloc_type, struct test_param *param) {
  struct buf_u8 buf = param->buf;
  u64 touch_size    = param->buf.size;

  while (tester_step(tester)) {
    do_allocation(alloc_type, &buf);
    if (buf.data) {
      tester_zone_begin(tester);
      for (u64 i = touch_size; i--;) {
        buf.data[i] = (u8)i; // write something
      }
      tester_zone_end(tester);

      tester_count_bytes(tester, touch_size);

      do_free(alloc_type, &buf);
    } else {
      os_print_last_error("mmap() failed");
      tester_error(tester, "Error: memory allocation failed");
    }
  }
}

static void test_fread(struct tester *tester, enum alloc_type alloc_type,
    struct test_param *param) {
  struct buf_u8 buf     = param->buf;
  u64 touch_size        = param->buf.size;
  const char *filepath  = param->filepath;
  FILE *f = 0;
  int err = 0;

  while (tester_step(tester)) {
    f = fopen(filepath, "rb");
    if (!f) {
      tester_error(tester, "Error: fopen() failed");
      break;
    }

    do_allocation(alloc_type, &buf);
    if (buf.data) {
      tester_zone_begin(tester);
      err = fread(buf.data, 1, touch_size, f) != touch_size;
      tester_zone_end(tester);

      tester_count_bytes(tester, touch_size);

      do_free(alloc_type, &buf);
    } else {
      os_print_last_error("mmap() failed");
      tester_error(tester, "Error: memory allocation failed");
    }
    fclose(f);

    if (err) {
      tester_error(tester, "Error: fread() failed");
    }
  }
}

static void test_file_mmap(struct tester *tester, struct test_param *param) {
  struct buf_u8 buf     = param->buf;
  u64 touch_size        = param->buf.size;
  const char *filepath  = param->filepath;

  while (tester_step(tester)) {
    struct os_buf map = os_file_mmap(filepath);
    if (map.data && map.size) {

      tester_zone_begin(tester);
      for (u64 i = 0; i < touch_size; ++i) {
        buf.data[i] = (u8)i; // write something
      }
      tester_zone_end(tester);

      tester_count_bytes(tester, touch_size);

      os_file_munmap(map);
    } else {
      tester_error(tester, "Error: os_file_mmap() failed");
      os_print_last_error("os_file_mmap() failed");
    }
  }
}

typedef void test_func_t(struct tester *, enum alloc_type, struct test_param *);

struct test
{
  const char *name;
  test_func_t *func;

};

static struct test s_tests[] =
{
  {"test_write_all", test_write_all},
  {"test_write_all_backwards", test_write_all_backwards},
  {"test_fread", test_fread},
};

// --------------------------------------
// Main
// --------------------------------------
static void print_usage(void) {
  fprintf(stderr, "Usage:\n    read_overhead <filename> [testname]\n");
}

int main(int argc, char **argv) {
  g_os_validator = (struct os_validator){
    .log_error = puts,
    .trap_on_error = 0,
  };

  const char *filepath;
  const char *testname;

  if (argc > 1 && strcmp(argv[1], "-h") == 0) {
    print_usage();
    return 0;
  }
  if (argc < 2) {
    print_usage();
    return 1;
  }

  filepath = argv[1];
  testname = argc > 2 ? argv[2] : 0;

  u64 file_size = os_file_size_bytes(filepath);
  if (!file_size) {
    fprintf(stderr, "Error: empty or non-existing file '%s'\n", filepath);
    return 1;
  }

  struct buf_u8 buf = {
    .data = malloc(file_size),
    .size = file_size,
  };

  struct test_param param = {
    .filepath = filepath,
    .buf = buf,
  };

  u64 cpu_timer_freq = get_or_estimate_cpu_timer_freq(300);
  u64 try_duration_tsc = 10 * cpu_timer_freq; // 10 seconds

  // File mmap test has a different structure and has it's own tester
  struct tester file_mmap_tester = {0};

  // Initialize testers
  struct tester testers[ARRAY_COUNT(s_tests)][ALLOC_TYPE_COUNT] = {0};

  for (u64 test_index = 0; test_index < ARRAY_COUNT(s_tests); ++test_index) {
    for (i64 alloc_type = 0; alloc_type < ALLOC_TYPE_COUNT; ++alloc_type) {
        struct tester *tester = &testers[test_index][alloc_type];
        tester->try_duration_tsc = try_duration_tsc;
        tester->expected_bytes   = buf.size;
    }
  }

  // Run tests forever
  u64 number_of_runs = (u64)-1; // run tests forever
  for (u64 run_index = 0; run_index < number_of_runs; ++run_index) {
    fprintf(stderr, "------------------------------------------------------\n");
    fprintf(stderr, "RUN %-20llu\n", run_index);
    fprintf(stderr, "------------------------------------------------------\n");

    // File mmap test has a different structure and is not part of
    // enum alloc_type.
    fprintf(stderr, "--- Test test_file_mmap ---\n");
    if (testname && strcmp(testname,"test_file_mmap") != 0) {
      fprintf(stderr, "Skipped\n\n");
    } else {
      file_mmap_tester.run = (struct tester_run){0};
      test_file_mmap(&file_mmap_tester, &param);

      tester_print(&file_mmap_tester, cpu_timer_freq);
      fprintf(stderr, "\n");

      if (file_mmap_tester.run.state == TESTER_STATE_ERROR) {
        goto cleanup;
      }
    }

    // For the rest of tests
    for (u64 test_index = 0; test_index < ARRAY_COUNT(s_tests); ++test_index) {
      struct test *test = s_tests + test_index;
      if (testname && strcmp(testname, test->name) != 0) {
        fprintf(stderr, "--- Test %s ---\n", test->name);
        fprintf(stderr, "Skipped\n\n");
        continue;
      }

      // For all allocation types
      for (i64 alloc_type = 0; alloc_type < ALLOC_TYPE_COUNT; ++alloc_type) {
        struct tester *tester = &testers[test_index][alloc_type];
        tester->run = (struct tester_run){0};

        fprintf(stderr, "--- Test %s, %s ---\n",
            test->name, alloc_type_to_cstr(alloc_type));

// TODO: implement and use os_large_page_size() to define if large allocations
// are supported by the platform. For now use this #if switch.
#if 1
        if (alloc_type == ALLOC_TYPE_VIRTUAL_LARGE_ALLOC) {
          fprintf(stderr, "Skipped\n\n");
          continue;
        }
#endif

        test->func(tester, alloc_type, &param);

        tester_print(tester, cpu_timer_freq);
        fprintf(stderr, "\n");

        if (tester->run.state == TESTER_STATE_ERROR) {
          goto cleanup; // break outside of multiple loops
        }
      }
    }
  }

cleanup:
  free(buf.data);
  buf = (struct buf_u8){0};

  return 0;
}
