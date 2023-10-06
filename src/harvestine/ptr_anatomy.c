// Performance-Aware-Programming Course
// https://www.computerenhance.com/p/table-of-contents
//
// Part 3
// Pointer anatomy
// https://www.computerenhance.com/p/four-level-paging

#include <stdio.h>      // printf
#include <string.h>     // string
#include <stdlib.h>     // atol

#include "types.h"
#include "os.h"

// Begin unity build
#include "os.c"
// End unity build

// --------------------------------------
// Address anatomy
// --------------------------------------
enum { ADDR_ANATOMY_CHUNK_COUNT = 6 };

// Values encoded in 64 bit address chunks
struct addr_anatomy_chunks {
  u32 e[ADDR_ANATOMY_CHUNK_COUNT];
};

// Bit per chunk
struct addr_anatomy_bit_counts {
  u8 e[ADDR_ANATOMY_CHUNK_COUNT];
};

// Labels
struct addr_anatomy_labels {
  const char *e[ADDR_ANATOMY_CHUNK_COUNT];
};

// Decoded pointer address according to address chunks
struct addr_anatomy {
  struct addr_anatomy_chunks      chunks;
  struct addr_anatomy_bit_counts  bit_counts; // 0 bits = empty chunk
  u64                             addr;
};

static struct addr_anatomy addr_anatomy_from_bit_counts(void *p,
    struct addr_anatomy_bit_counts bit_counts) {
  u64 addr = (u64)p;

  struct addr_anatomy ret = {
    .bit_counts = bit_counts,
    .addr = addr,
  };

  u32 top = 64;
  for (u32 i = 0; i < ADDR_ANATOMY_CHUNK_COUNT; ++i) {
    u8 bit_count = bit_counts.e[i];
    if (bit_count) {
      top -= bit_count;
      ret.chunks.e[i] = (addr >> top) & ((1LLU << bit_count) - 1);
    }
  }

  return ret;
}

static void print_addr_anatomy_values(struct addr_anatomy aa, b32 hex) {
  for (u32 i = 0; i < ADDR_ANATOMY_CHUNK_COUNT; ++i) {
    u8 bit_count = aa.bit_counts.e[i];
    if (bit_count) {
      printf(hex ? "%*x|" :"%*u|", aa.bit_counts.e[i], aa.chunks.e[i]);
    }
  }
  printf("\n");
}

static void print_addr_anatomy_binary(struct addr_anatomy aa) {
  u32 top = 64;
  for (u32 i = 0; i < ADDR_ANATOMY_CHUNK_COUNT; ++i) {
    u8 bit_count = aa.bit_counts.e[i];
    if (bit_count) {
      u32 bottom = top - bit_count;
      for (; top > bottom;) {
        --top;
        u64 bit = aa.addr >> top & 1;
        printf("%c", bit ? '1' : '0');
      }
      printf("|");
    }
  }
  printf("\n");
}

static void print_addr_anatomy_labels(struct addr_anatomy_bit_counts bit_counts,
    struct addr_anatomy_labels labels) {
  for (u32 i = 0; i < ADDR_ANATOMY_CHUNK_COUNT; ++i) {
    u8 bit_count = bit_counts.e[i];
    if (bit_count) {
      printf("%*s|", bit_counts.e[i], labels.e[i]);
    }
  }
  printf("\n");
}

// --------------------------------------
// Dissect runs
// --------------------------------------
struct dissect {
  struct addr_anatomy_labels      labels;
  struct addr_anatomy_bit_counts  bit_counts;
  const char                      *message;
};

static struct dissect s_dissects[] = {
  #if defined(__x86_64__)
  {
    .labels = { "unused", "pml4", "dir_ptr", "directory", "table", "offset" },
    .bit_counts = { 16, 9, 9, 9, 9, 12, },
    .message = "x86_64 with 4KB pages"
  },
  {
    .labels = { "unused", "pml4", "dir_ptr", "directory", "table", "offset" },
    .bit_counts = { 16, 9, 9, 9, 0, 21, },
    .message = "x86_64 with 2MB pages"
  },
  {
    .labels = { "unused", "pml4", "dir_ptr", "directory", "table", "offset" },
    .bit_counts = { 16, 9, 9, 0, 0, 30, },
    .message = "x86_64 with 1GB pages"
  },
  #else
  #error not implemented
  #endif // #if defined(__x86_64__)
};

// --------------------------------------
// Main
// --------------------------------------
static void print_usage(void) {
  fprintf(stderr,
    "Pointer anatomy. Print out pointer internal structure.\n"
    "Usage:\n"
    "    ptr_anatomy [alloc_count=10] [alloc_size_b=native]\n");
}

int main(int argc, char **argv) {
  if (argc > 1 && strcmp(argv[1], "-h") == 0) {
    print_usage();
    return 0;
  }

  u64 os_page_size_b  = os_get_page_size();

  u64 alloc_count     = argc > 1 ? atol(argv[1]) : 8;
  u64 alloc_size_b    = argc > 2 ? atol(argv[2]) : (i64)os_page_size_b;

  if (!os_init()) {
    fprintf(stderr,
        "Failed to initialize performance counters. Try with super user.");
  }

  void *prev_p = 0;
  for (u64 i = 0; i < alloc_count; ++i) {
    void *p = os_virtual_alloc(alloc_size_b);
    i64 offset = (u64)p - (u64)prev_p;
    prev_p = p;
    printf("---------------------------------------------------------------\n");
    printf("Allocated address:  %p (%llu)\n", p, (u64)p);
    printf("Offset from prev:   %lld\n", offset);
    printf("\n");

    for (u64 dissect_i = 0; dissect_i < ARRAY_COUNT(s_dissects); ++dissect_i) {
      struct dissect *d = &s_dissects[dissect_i];
      printf("%s\n", d->message);
      struct addr_anatomy aa = addr_anatomy_from_bit_counts(p, d->bit_counts);
      print_addr_anatomy_labels(d->bit_counts, d->labels);
      print_addr_anatomy_binary(aa);
      print_addr_anatomy_values(aa, true);
      print_addr_anatomy_values(aa, false);
      printf("\n");
    }
  }

  // NOTE: leak, no os_virtual_free() for brevity

  return 0;
}
