#include <assert.h>     // assert
#include <stdio.h>      // fprintf fopen fread stderr
#include <stdlib.h>     // malloc free abort
#include <sys/stat.h>   // stat

#include "types.h"
#include "tester.h"

// Begin unity build
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
  ALLOCATION_TYPE_NONE,
  ALLOCATION_TYPE_MALLOC,

  ALLOCATION_TYPE_COUNT,
};

const char * alloc_type_to_cstr(enum alloc_type alloc_type) {
  switch (alloc_type) {
    case ALLOCATION_TYPE_NONE: return "no allocation";
    case ALLOCATION_TYPE_MALLOC: return "malloc";
    case ALLOCATION_TYPE_COUNT: return "<error>";
  }
  return "";
}

static void do_allocation(enum alloc_type alloc_type, struct buf_u8 *buf) {
  switch (alloc_type) {
    case ALLOCATION_TYPE_NONE:
      break;
    case ALLOCATION_TYPE_MALLOC:
      buf->data = (u8 *)malloc(buf->size);
      break;
    case ALLOCATION_TYPE_COUNT:
      assert(0 && "Unknown allocation type");
      abort();
      break;
  }
}

static void do_free(enum alloc_type alloc_type, struct buf_u8 *buf) {
  switch (alloc_type) {
    case ALLOCATION_TYPE_NONE:
      break;
    case ALLOCATION_TYPE_MALLOC:
      free(buf->data);
      buf->data = 0;
      break;
    case ALLOCATION_TYPE_COUNT:
      assert(0 && "Unknown allocation type");
      abort();
      break;
  }
}

static void test_fread(struct tester *tester, enum alloc_type alloc_type,
    struct test_param *param) {
  struct buf_u8 buf = param->buf;
  FILE *f = 0;
  int err = 0;

  while (tester_step(tester)) {
    f = fopen(param->filepath, "rb");
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

// --------------------------------------
// Main
// --------------------------------------
static void print_usage(void) {
  fprintf(stderr, "Usage:\n    read_overhead <filename>\n");
}

int main(int argc, char **argv) {
  const char *filepath;
  int err;

  if (argc < 1) {
    print_usage();
    return 1;
  }

  filepath = argv[1];

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

  struct tester testers[ALLOCATION_TYPE_COUNT] = {
    (struct tester){
      .try_duration_tsc = try_duration_tsc,
      .expected_bytes   = buf.size,
    },
    (struct tester){
      .try_duration_tsc = try_duration_tsc,
      .expected_bytes   = buf.size,
    },
  };

  // Run tests for all allocation types
  for (;;) {
    for (int alloc_type = 0; alloc_type < ALLOCATION_TYPE_COUNT; ++alloc_type) {
      struct tester *tester = testers + alloc_type;
      tester->run = (struct tester_run){0};

      fprintf(stderr, "--- Test, %s ---\n", alloc_type_to_cstr(alloc_type));
      test_fread(tester, alloc_type, &param);

      tester_print(tester, cpu_timer_freq);
      fprintf(stderr, "\n");
    }
  }

  free(buf.data);
  buf = (struct buf_u8){0};

  return 0;
}
