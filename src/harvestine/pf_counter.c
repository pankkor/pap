// Performance-Aware-Programming Course
// https://www.computerenhance.com/p/table-of-contents
//
// Part 3
// Page fault test counter

#include <stdio.h>      // printf perror stderr
#include <string.h>     // string
#include <stdlib.h>     // atol

#include "types.h"
#include "os.h"

// Begin unity build
#include "os.c"
// End unity build

// --------------------------------------
// Parse args
// --------------------------------------
struct options {
  union {
    struct {
      const char *help;
      const char *page_count;
      const char *page_size_kb;
      const char *lock_in_ram;
      const char *large_pages;
    } name;
    const char *e[5];
  };
};

static struct options s_options = {
  .e = {
    "-h",
    "--page_count=",
    "--page_size_kb=",
    "--lock_in_ram",
    "--large_pages",
  }
};

static void print_usage(void) {
  fprintf(stderr,
      "Virtual alloc memory, touch it and calculate page faults.\n"
      "Usage:\n"
      "    pf_counter <OPTIONS>\n"
      "\n"
      "OPTIONS\n");

  for (u64 opt_idx = 0; opt_idx < ARRAY_COUNT(s_options.e); ++opt_idx) {
    fprintf(stderr, "    %s\n", s_options.e[opt_idx]);
  }
}

static const char *parse_arg(const char *arg, const char *option) {
  u64 len = strlen(option);
  if (strncmp(option, arg, len) == 0) {
    return arg + len;
  }
  return 0;
}

static struct options parse_args(int argc, char **argv, struct options options) {
  struct options ret = {0};
  for (int i = 1; i < argc; ++i) {
    b32 known_arg = false;
    for (u64 opt_idx = 0; opt_idx < ARRAY_COUNT(options.e); ++opt_idx) {
      const char *option = options.e[opt_idx];
      const char *value = parse_arg(argv[i], option);
      if (value) {
        ret.e[opt_idx] = value;
        known_arg = true;
        break;
      }
    }
    if (!known_arg) {
      fprintf(stderr, "Error: unknown option '%s'\n", argv[i]);
      print_usage();
      exit(1);
    }
  }
  return ret;
}

// --------------------------------------
// Main
// --------------------------------------
int main(int argc, char **argv) {
  u64 os_page_size_kb = os_get_page_size() / 1024;
  u64 page_count      = 1024;
  u64 page_size_kb    = os_page_size_kb;
  b32 lock_in_ram     = false;
  b32 large_pages     = false;

  struct options args = parse_args(argc, argv, s_options);
  if (args.name.help) {
    print_usage();
    return 0;
  }
  if (args.name.page_count) {
    page_count = atol(args.name.page_count);
  }
  if (args.name.page_size_kb) {
    page_size_kb = atol(args.name.page_size_kb);
  }
  if (args.name.lock_in_ram) {
    lock_in_ram = true;
  }
  if (args.name.large_pages) {
    large_pages = true;
  }

  u64 page_size_b = page_size_kb * 1024;

  // print to stderr run information
  fprintf(stderr, "Page count:    %llu\n", page_count);
  fprintf(stderr, "Page size      %llu KB\n", page_size_kb);
  fprintf(stderr, "OS Page size:  %llu KB\n", os_page_size_kb);
  fprintf(stderr, "Lock memory:   %s\n", lock_in_ram ? "true" : "false");
  fprintf(stderr, "Large pages:   %s\n", large_pages ? "true" : "false");

  if (!os_perf_init()) {
    fprintf(stderr,
        "Failed to initialize performance counters. Try with super user.");
  }

  // print to stdout in csv format
  printf("Page Count, Page Size KB, Touch Count, Fault Count, Extra Faults\n");

  for (u64 touch_p_count = 1; touch_p_count <= page_count; ++touch_p_count) {
    u64 touch_mem_size = touch_p_count * page_size_b;
    u64 mem_size = touch_mem_size;
    u8 *buf = large_pages
      ? os_virtual_large_alloc(&mem_size)
      : os_virtual_alloc(mem_size);

    if (!buf) {
      os_print_last_error("alloc failed");
      return 1;
    }

    u64 begin_pf_count = os_read_page_fault_count();

    if (lock_in_ram) {
      if (!os_virtual_lock((void *)buf, mem_size)) {
        os_print_last_error("os_virtual_lock failed");
        return 1;
      }
    }

    // touch memory
    for (u64 i = 0; i < touch_mem_size; ++i) {
      buf[i] = (u8)i;
    }

    u64 end_pf_count = os_read_page_fault_count();
    u64 pf_count = end_pf_count - begin_pf_count;
    u64 extra_pf_count = pf_count - touch_p_count;

    // no need for os_virtiual_unlock()
    os_virtual_free(buf, mem_size);

    printf("%llu, %llu, %llu, %llu, %lld\n",
        page_count, page_size_kb, touch_p_count, pf_count, extra_pf_count);
  }

  return 0;
}
