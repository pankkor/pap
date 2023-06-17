// Performance-Aware-Programming Course
// https://www.computerenhance.com/p/table-of-contents
//
// Part 2
// Harvestine distance

// Begin unity build
#include "timer.c"
// End unity build

#include "types.h"
#include "timer.h"

#include <stdio.h>      // printf fprints fopen fread fseek ftell
#include <string.h>     // strcmp

u64 file_write(const char *filepath, u8 *buf, u64 buf_size) {
  u64 written = 0;

  FILE *f = fopen(filepath, "wb");
  if (!f) {
    perror("Error: fopen() failed");
    return written;
  }

  written = fwrite(buf, 1, buf_size, f);
  if (written != buf_size) {
    perror("Error: fwrite() failed");
  }

  fclose(f);
  return written;
}

u64 file_read(const char *filepath, u8 *buf, u64 buf_size) {
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
  if (file_size > buf_size) {
    fprintf(stderr,
        "Error: binary file is too big to be loaded into the buffer. File size:"
        "%ld bytes, buffer size: %lu bytes\n", file_size, buf_size);
    goto file_read_failed;
  }

  if (fseek(f, 0, SEEK_SET)) {
    perror("Error: fseek() failed");
    goto file_read_failed;
  }

  if (fread(buf, 1, file_size, f) != file_size) {
    perror("Error: fread() failed");
    goto file_read_failed;
  }

  fclose(f);
  return file_size;

file_read_failed:
  if (f) {
    fclose(f);
  }
  return 0;
}

void print_usage(void) {
  fprintf(stderr, "Usage:\nharvestine <input_file>\n");
}

int main(int argc, char **argv) {
  if (argc < 2) {
    print_usage();
    return 1;
  }

  const char *filepath = argv[1];
  (void)filepath;

  u64 os_time = read_os_timer();
  (void)os_time;

  return 0;
}
