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
// Main
// --------------------------------------
static void print_usage(void) {
  fprintf(stderr,
    "Virtual alloc memory, touch it and calculate page faults.\n"
    "Usage:\n"
    "    pf_counter [page_count=1024] [page_size_kb=native] [lock_in_ram=0]\n");
}

int main(int argc, char **argv) {
  if (argc > 1 && strcmp(argv[1], "-h") == 0) {
    print_usage();
    return 0;
  }

  if (!os_init()) {
    fprintf(stderr,
        "Failed to initialize performance counters. Try with super user.");
  }

  u64 os_page_size_kb = os_get_page_size() / 1024;

  u64 page_count      = argc > 1 ? atol(argv[1]) : 1024;
  u64 page_size_kb    = argc > 2 ? atol(argv[2]) : (i64)os_page_size_kb;
  b32 lock_in_ram     = argc > 3 ? atoi(argv[3]) : false;

  u64 page_size_b     = page_size_kb * 1024;

  // print to stderr run information
  fprintf(stderr, "OS Page size:  %llu KB\n", os_page_size_kb);
  fprintf(stderr, "Lock memory:   %s\n", lock_in_ram ? "true" : "false");

  // print to stdout in csv format
  printf("Page Count, Page Size KB, Touch Count, Fault Count, Extra Faults\n");

  for (u64 touch_p_count = 0; touch_p_count <= page_count; ++touch_p_count) {
    u64 mem_size = touch_p_count * page_size_b;
    u8 *buf = os_virtual_alloc(mem_size);

    u64 begin_pf_count = os_read_page_fault_count();

    if (lock_in_ram) {
      if (!os_virtual_lock((void *)buf, mem_size)) {
        perror("os_virtual_lock failed");
        return 1;
      }
    }

    // touch memory
    for (u64 i = 0; i < mem_size; ++i) {
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
