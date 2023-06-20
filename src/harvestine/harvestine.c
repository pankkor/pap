// Performance-Aware-Programming Course
// https://www.computerenhance.com/p/table-of-contents
//
// Part 2
// Harvestine distance input file generator

// Begin unity build
#include "timer.c"
#include "calc_harvestine.c"
// End unity build

#include "calc_harvestine.h"

#include "types.h"
#include "timer.h"

#include <stdio.h>      // printf fprintf fopen fread fseek ftell
#include <stdlib.h>     // malloc

static u8 *alloc_buf_file_read(const char *filepath, u64 *out_buf_size) {
  u8 *buf = 0;

  FILE *f = fopen(filepath, "rb");
  if (!f) {
    perror("Error: fopen() failed");
    goto file_read_failed;
  }

  if (fseek(f, 0, SEEK_END)) {
    perror("Error: fseek() failed");
    goto file_read_failed;
  }

  u64 file_size = ftell(f);
  if (fseek(f, 0, SEEK_SET)) {
    perror("Error: fseek() failed");
    goto file_read_failed;
  }

  buf = malloc(file_size);
  if (!buf) {
    goto file_read_failed;
  }

  if (fread(buf, 1, file_size, f) != file_size) {
    perror("Error: fread() failed");
    goto file_read_failed;
  }
  fclose(f);

  *out_buf_size = file_size;
  return buf;

file_read_failed:
  if (f) {
    fclose(f);
  }
  free(buf);
  return 0;
}

static void print_usage(void) {
  fprintf(stderr, "Usage:\nharvestine <input_file>\n");
}

int main(int argc, char **argv) {
  if (argc < 2) {
    print_usage();
    return 1;
  }

  const char *filepath = argv[1];
  u64 buf_size = 0;
  u8 *buf = alloc_buf_file_read(filepath, &buf_size);
  printf("READ %lu bytes\n--------\n%s\n-------------\n", buf_size, buf);


  // TODO predictive parse json

  return 0;
}
