// Performance-Aware-Programming Course
// https://www.computerenhance.com/p/table-of-contents
//
// Part 3
// Run microbenchmarks

#include <stdio.h>      // fprintf stderr
#include <stdlib.h>     // malloc free
#include <string.h>     // strcmp

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
typedef void test_func_t(u8 *data, u64 bytes);
// static void test_nop_3x1_all_bytes(u8 *data, u64 bytes);
// static void test_nop_1x3_all_bytes(u8 *data, u64 bytes);
// static void test_nop_1x9_all_bytes(u8 *data, u64 bytes);
extern void test_asm_2(u8 *data, u64 bytes);

// TODO <<<<<<<<<<<<<

// static void test_all_bytes_opt(u8 *data, u64 bytes) {
//   for (u64 i = 0; i < bytes; ++i) {
//     data[i] = (u8)i;
//   }
// }
//
// static void test_all_bytes_noopt(u8 *data, u64 bytes) {
//   LOOP_NO_OPT
//   for (u64 i = 0; i < bytes; ++i) {
//     data[i] = (u8)i;
//   }
// }
//
// NO_OPT
// static void test_nop_3x1_all_bytes(u8 *data, u64 bytes) {
//   for (u64 i = 0; i < bytes; ++i) {
//     data[i] = (u8)i;
//   }
// }
//
// static void test_nop_1x3_all_bytes(u8 *data, u64 bytes) {
//   LOOP_NO_OPT
//   for (u64 i = 0; i < bytes; ++i) {
//     data[i] = (u8)i;
//   }
// }
//
// static void test_nop_1x9_all_bytes(u8 *data, u64 bytes) {
//   for (u64 i = 0; i < bytes; ++i) {
//     data[i] = (u8)i;
//   }
// }

static void test_asm(u8 *data, u64 bytes) {
#if defined(__aarch64__)
  // x0 - data
  // x1 - bytes
  __asm__ volatile (
      "cbz %[bytes], 1f\n\t"
      "mov x9, #0\n\t"
      "0: strb w9, [x0, x9]\n\t"
      "add x9, x9, #1\n\t"
      "cmp %[bytes], x9\n\t"
      "b.ne 0b\n\t"
      "1:\n\t"
      : /* no output */
      : [data] "r" (data), [bytes] "r" (bytes)
      : "memory", "x9");
#elif defined(__x86_64__)
  __asm__ volatile (
      "testq %[bytes], %[bytes]\n\t"
      "je 1f\n\t"
      "xorl %%eax, %%eax\n\t"
      "0: movb %%al, (%[data],%%rax)\n\t"
      "incq %%rax\n\t"
      "cmpq %%rax, %[bytes]\n\t"
      "jne 0b\n\t"
      "1:\n\t"
      : /* no output */
      : [data] "r" (data), [bytes] "r" (bytes)
      : "memory", "rax");
#endif
}
// TODO >>>>>>>>>>>>>

struct test
{
  const char *name;
  test_func_t *func;
};

static struct test s_tests[] =
{
  {"test_asm",                test_asm},
  {"test_asm_2",              test_asm_2},
  // {"test_all_bytes_opt",      test_all_bytes_opt},
  // {"test_all_bytes_noopt",    test_all_bytes_noopt},
  // {"test_nop_3x1_all_bytes",  test_nop_3x1_all_bytes},
  // {"test_nop_1x3_all_bytes",  test_nop_1x3_all_bytes},
  // {"test_nop_1x9_all_bytes",  test_nop_1x9_all_bytes},
};

// --------------------------------------
// Main
// --------------------------------------
static void print_test_list(void) {
  fprintf(stderr, "Tests:\n");
  for (u64 i = 0; i < ARRAY_COUNT(s_tests); ++i) {
    fprintf(stderr, "    '%s',\n", s_tests[i].name);
  }
}

static void print_usage(void) {
  fprintf(stderr,
    "Run all microbenchmarks, or only one specified by [testname].\n"
    "Usage:\n"
    "    microbenchmarks [OPTIONS] [testname]\n"
    "\n"
    "OPTIONS\n"
    "    -h     - this help.\n"
    "    --list - list all tests.\n"
    "\n");
  print_test_list();
}

int main(int argc, char **argv) {
  g_os_validator = (struct os_validator){
    .log_error = puts,
    .trap_on_error = 0,
  };


  // Parse args
  if (argc > 1 && strcmp(argv[1], "-h") == 0) {
    print_usage();
    return 0;
  }
  if (argc > 1 && strcmp(argv[1], "--list") == 0) {
    print_test_list();
    return 0;
  }

  i64 run_only_test_index = -1;
  if (argc > 1) {
    const char *test_name = argv[1];
    for (u64 i = 0; i < ARRAY_COUNT(s_tests); ++i) {
      if (strcmp(test_name, s_tests[i].name) == 0) {
         run_only_test_index = i;
         break;
      }
    }
    if (run_only_test_index == -1) {
      fprintf(stderr, "Error: unknown testname '%s'.\n", test_name);
      print_test_list();
      return 1;
    }
  }

  enum { BUF_SIZE = 1 * 1024 * 1024 * 1024 };
  struct buf_u8 buf = {
    .data = malloc(BUF_SIZE),
    .size = BUF_SIZE,
  };

  u64 cpu_timer_freq = get_or_estimate_cpu_timer_freq(300);
  u64 try_duration_tsc = 1 * cpu_timer_freq; // 10 seconds

  struct tester testers[ARRAY_COUNT(s_tests)] = {0};
  for (u64 i = 0; i < ARRAY_COUNT(testers); ++i) {
      struct tester *tester = testers + i;
      tester->try_duration_tsc = try_duration_tsc;
      tester->expected_bytes   = buf.size;
  }

  // Run tests
  u64 number_of_runs = (u64)-1; // run tests forever
  for (u64 run_index = 0; run_index < number_of_runs; ++run_index) {
    fprintf(stderr, "------------------------------------------------------\n");
    fprintf(stderr, "RUN %-20llu\n", run_index);
    fprintf(stderr, "------------------------------------------------------\n");

    for (u64 test_index = 0; test_index < ARRAY_COUNT(s_tests); ++test_index) {
      struct test *test = s_tests + test_index;
      struct tester *tester = testers + test_index;

      fprintf(stderr, "--- Test %s ---\n", test->name);
      if (run_only_test_index > 0 && (u64)run_only_test_index != test_index) {
        fprintf(stderr, "Skipped\n\n");
      } else {
        tester->run = (struct tester_run){0};
        while (tester_step(tester)) {
          tester_zone_begin(tester);
          test->func(buf.data, buf.size);
          tester_zone_end(tester);

          tester_count_bytes(tester, buf.size);
        }

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
