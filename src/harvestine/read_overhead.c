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

  ALLOC_TYPE_COUNT,
};

const char * alloc_type_to_cstr(enum alloc_type alloc_type) {
  switch (alloc_type) {
    case ALLOC_TYPE_NONE:    return "no allocation";
    case ALLOC_TYPE_MALLOC:  return "malloc";
    case ALLOC_TYPE_COUNT:   return "<error>";
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
    case ALLOC_TYPE_COUNT:
      assert(0 && "Unknown allocation type");
      abort();
      break;
  }
}

static void do_free(enum alloc_type alloc_type, struct buf_u8 *buf) {
  switch (alloc_type) {
    case ALLOC_TYPE_NONE:
      break;
    case ALLOC_TYPE_MALLOC:
      free(buf->data);
      buf->data = 0;
      break;
    case ALLOC_TYPE_COUNT:
      assert(0 && "Unknown allocation type");
      abort();
      break;
  }
}

static void test_write_all(struct tester *tester, enum alloc_type alloc_type,
    struct test_param *param) {
  struct buf_u8 buf = param->buf;

  while (tester_step(tester)) {
    do_allocation(alloc_type, &buf);

    tester_zone_begin(tester);
    for (u64 i = 0; i < buf.size; ++i) {
      buf.data[i] = (u8)i; // write something
    }
    tester_zone_end(tester);

    tester_count_bytes(tester, buf.size);

    do_free(alloc_type, &buf);

  }
}

static void test_write_all_backwards(struct tester *tester,
    enum alloc_type alloc_type, struct test_param *param) {
  struct buf_u8 buf = param->buf;

  while (tester_step(tester)) {
    do_allocation(alloc_type, &buf);

    tester_zone_begin(tester);
    for (u64 i = buf.size; i--;) {
      buf.data[i] = (u8)i; // write something
    }
    tester_zone_end(tester);

    tester_count_bytes(tester, buf.size);

    do_free(alloc_type, &buf);

  }
}

static void test_fread(struct tester *tester, enum alloc_type alloc_type,
    struct test_param *param) {
  struct buf_u8 buf     = param->buf;
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

    tester_zone_begin(tester);
    err = fread(buf.data, 1, buf.size, f) != buf.size;
    tester_zone_end(tester);

    tester_count_bytes(tester, buf.size);

    do_free(alloc_type, &buf);
    fclose(f);

    if (err) {
      tester_error(tester, "Error: fread() failed");
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
  const char *filepath;
  const char *testname;
  int err;

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

 // get file size
#if _WIN32
  struct __stat64 st;
  err = _stat64(filepath, &st);
#else
  struct stat st;
  err = stat(filepath, &st);
#endif
  if (err) {
    perror("Error: stat() failed");
    return 1;
  }

  struct buf_u8 buf = {
    .data = malloc(st.st_size),
    .size = st.st_size,
  };

  struct test_param param = {
    .filepath = filepath,
    .buf = buf,
  };

  u64 cpu_timer_freq = get_or_estimate_cpu_timer_freq(300);
  u64 try_duration_tsc = 10 * cpu_timer_freq; // 10 seconds

  // initialize testers
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
    fprintf(stderr, "RUN %-20lu\n", run_index);
    fprintf(stderr, "------------------------------------------------------\n");

    // for all tests
    for (u64 test_index = 0; test_index < ARRAY_COUNT(s_tests); ++test_index) {
      struct test *test = s_tests + test_index;
      if (testname && strcmp(testname, test->name) != 0) {
        continue;
      }

      // for all allocation types
      for (i64 alloc_type = 0; alloc_type < ALLOC_TYPE_COUNT; ++alloc_type) {
        struct tester *tester = &testers[test_index][alloc_type];
        tester->run = (struct tester_run){0};

        fprintf(stderr, "--- Test %s, %s ---\n",
            test->name, alloc_type_to_cstr(alloc_type));

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
