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


// // TODO: it's x86_64 anatomy, might not me right for Aarch64
// struct addr_anatomy {
//     u16 unused;
//     u16 pml4;
//     u16 directory_ptr;
//     u16 directory;
//     u16 table;
//     u32 offset;
// };
//
// static struct addr_anatomy addr_anatomy_from_ptr_4k(void *p) {
//   struct addr_anatomy ret;
//   u64 addr = (u64)p;
//
//   ret.unused        = (addr >> 48) & 0xffff;
//   ret.pml4          = (addr >> 39) & 0x1ff;
//   ret.directory_ptr = (addr >> 30) & 0x1ff;
//   ret.directory     = (addr >> 21) & 0x1ff;
//   ret.table         = (addr >> 12) & 0x1ff;
//   ret.offset        = (addr >>  0) & 0xfff;
//
//   return ret;
// }
//
// static struct addr_anatomy addr_anatomy_from_ptr_16k(void *p) {
//   struct addr_anatomy ret;
//   u64 addr = (u64)p;
//
//   ret.unused        = (addr >> 51) & 0xffff;
//   ret.pml4          = (addr >> 41) & 0x1ff;
//   ret.directory_ptr = (addr >> 32) & 0x1ff;
//   ret.directory     = (addr >> 23) & 0x1ff;
//   ret.table         = (addr >> 14) & 0x1ff;
//   ret.offset        = (addr >>  0) & 0x3fff;
//
//   return ret;
// }
//
// struct addr_anatomy_format {
//   u8 e[6];
// };
//
// static void print_addr_anatomy(struct addr_anatomy *aa) {
//    printf("|%16u|%9u|%9u|%9u|%9u|%12u|",
//        aa->unused, aa->pml4, aa->directory_ptr, aa->directory, aa->table,
//        aa->offset);
// }
//
// static void print_addr_anatomy_fmt(struct addr_anatomy *aa,
//     struct addr_anatomy_format fmt) {
//    // printf("|%16u|%9u|%9u|%9u|%9u|%12u|",
//    //     aa->unused, aa->pml4, aa->directory_ptr, aa->directory, aa->table,
//    //     aa->offset);
//
//    printf("%*u|", fmt.e[0], aa->unused);
//    printf("%*u|", fmt.e[1], aa->pml4);
//    printf("%*u|", fmt.e[2], aa->directory_ptr);
//    printf("%*u|", fmt.e[3], aa->directory);
//    if (fmt.e[4]) {
//      printf("%*u|", fmt.e[4], aa->table);
//    } else {
//      printf("|");
//    }
//    printf("%*u|", fmt.e[5], aa->offset);
//    printf("\n");
// }
//
// static void print_binary(u64 v, u32 first_bit, u32 bit_count) {
//   for (u32 i = first_bit + bit_count; i-- > first_bit;) {
//     u64 bit = (v >> i) & 1;
//     printf("%c", bit ? '1' : '0');
//   }
// }
//
// static void print_binary_fmt(u64 v, struct addr_anatomy_format fmt) {
//   u32 bit_left = 64;
//   for (u32 i = 0; i < ARRAY_COUNT(fmt.e); ++i) {
//     u8 bit_count = fmt.e[i];
//     u32 bit_index = bit_left - bit_count;
//     bit_left = bit_index;
//     print_binary(v, bit_index, bit_count);
//     printf("|");
//   }
//   printf("\n");
// }

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

struct addr_anatomy2 {
  struct addr_anatomy_chunks      chunks;
  struct addr_anatomy_bit_counts  bit_counts; // 0 bits = empty chunk
  u64 addr;
};

static struct addr_anatomy_labels s_x86_64_labels = {
  "unused", "pml4", "dir_ptr", "directory", "table", "offset"
};

static struct addr_anatomy_bit_counts s_x86_64_4kb_bit_counts = {
  16, 9, 9, 9, 9, 12,
};

static struct addr_anatomy_bit_counts s_x86_64_2mb_bit_counts = {
  16, 9, 9, 9, 0, 21,
};

static struct addr_anatomy2 addr_anatomy_from_bit_counts(void *p,
    struct addr_anatomy_bit_counts bit_counts) {
  u64 addr = (u64)p;

  struct addr_anatomy2 ret = {
    .bit_counts = bit_counts,
    .addr = addr,
  };

  for (u32 i = 0; i < ADDR_ANATOMY_CHUNK_COUNT; ++i) {
    u8 bit_count = bit_counts.e[i];
    if (!bit_count) {
      continue;
    }
    ret.chunks.e[i] = (addr >> (64 - bit_count)) & ((1 << bit_count) - 1);
  }

  return ret;
}

static void print_addr_anatomy2_values(struct addr_anatomy2 aa) {
  for (u32 i = 0; i < ADDR_ANATOMY_CHUNK_COUNT; ++i) {
    u8 bit_count = aa.bit_counts.e[i];
    if (bit_count) {
      printf("%*u|", aa.bit_counts.e[i], aa.chunks.e[i]);
    } else {
      printf("|");
    }
  }
  printf("\n");
}

static void print_addr_anatomy2_binary(struct addr_anatomy2 aa) {
  u32 left = 64;
  for (u32 i = 0; i < ADDR_ANATOMY_CHUNK_COUNT; ++i) {
    u8 bit_count = aa.bit_counts.e[i];
    u32 right = left - bit_count;

    for (; left-- > right;) {
      u64 bit = aa.addr & (1 << left);
      printf("%c", bit ? '1' : '0');
    }
    printf("|");
  }
  printf("\n");
}

static void print_addr_anatomy_labels(struct addr_anatomy_bit_counts bit_counts,
    struct addr_anatomy_labels labels) {
  for (u32 i = 0; i < ADDR_ANATOMY_CHUNK_COUNT; ++i) {
    u8 bit_count = bit_counts.e[i];
    if (bit_count) {
      printf("%*s|", bit_counts.e[i], labels.e[i]);
    } else {
      printf("|");
    }
  }
  printf("\n");
}

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

  for (u64 i = 0; i < alloc_count; ++i) {
    void *p = os_virtual_alloc(alloc_size_b);
    printf("Allocated %p\n", p);
    print_addr_anatomy_labels(s_x86_64_4kb_bit_counts, s_x86_64_labels);

    struct addr_anatomy2 x86_64_4k_aa
      = addr_anatomy_from_bit_counts(p, s_x86_64_4kb_bit_counts);
    print_addr_anatomy2_binary(x86_64_4k_aa);
    print_addr_anatomy2_values(x86_64_4k_aa);

    printf("\n");

    struct addr_anatomy2 x86_64_2mb_aa
      = addr_anatomy_from_bit_counts(p, s_x86_64_2mb_bit_counts);
    print_addr_anatomy2_binary(x86_64_2mb_aa);
    print_addr_anatomy2_values(x86_64_2mb_aa);

    printf("\n");
  }

  // NOTE: leak, no os_virtual_free() for brevity

  return 0;
}
