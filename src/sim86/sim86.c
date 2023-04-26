// Performance-Aware-Programming Course
// https://www.computerenhance.com/p/table-of-contents
//
// Part 1
// 8086 instructions decode
//
// Run 'test_decode.sh' that tests decoding on all the listings in listings dir

// unity build begin
#include "sim86_decode.c"
#include "sim86_instr.c"
#include "sim86_print.c"
#include "sim86_stream.c"
// unity build end

#include "sim86_decode.h"
#include "sim86_print.h"
#include "sim86_stream.h"

#include <unistd.h>     // lseek
#include <fcntl.h>      // open
#include <sys/types.h>  // legacy
#include <sys/mman.h>   // mmap munmap
#include <stdio.h>      // printf
#include <string.h>     // strcmp

// string view
struct sv {
  const char *begin;
  const char *end;
};

const char *s_dot = ".";

// Custom basename returning string view
struct sv basename(const char *path) {
  if (*path == 0) {
    return (struct sv){s_dot, s_dot + 1};
  }

  struct sv prev_sv = {path, path + 1};
  struct sv sv = {path, path + 1};

  do {
    if (*path == '\\' || *path == '/') {
      prev_sv = sv;
      sv.begin = path + 1;
    }
    sv.end = ++path;
  } while (*path != 0);
  sv.end = path;

  return sv.end - sv.begin > 0 ? sv : prev_sv;
}

int decode(struct stream *s) {
  decode_build_index();

  u8 *sbegin = s->data;

  while (s->data < s->end) {
    struct instr instr = decode_next_instr(s);
    if (!instr.str) {
      fprintf(stderr, "Error: unsupported instruction at byte number %ld\n",
          s->data - sbegin);
      return 0;
    }

    print_instr(&instr);
  }
  return 1;
}

int main(int argc, char **argv) {
  if (argc < 2) {
    fprintf(stderr, "Usage:\nsim86 [decode] <input_binary_file>\n");
    return 1;
  }

  enum { SIMULATE, DECODE } command = SIMULATE;
  if (argc == 3) {
    command = strcmp(argv[1], "decode") == 0 ? DECODE : SIMULATE;
  }

  const char *filepath = argv[argc - 1];
  int fd = open(filepath, O_RDONLY);
  if (fd == -1) {
    perror("failed to open file:");
    return 1;
  }

  off_t file_size = lseek(fd, 0, SEEK_END);
  u8 *file_data = mmap(NULL, file_size, PROT_READ, MAP_PRIVATE, fd, 0);
  close(fd);
  if (file_data == MAP_FAILED) {
    perror(file_size ? "Error: mmap failed" : "Error: mmap failed, empty file");
    return 1;
  }

  struct stream s = {file_data, file_data + file_size};

  int res;
  if (command == DECODE) {
    printf("bits 16\n");
    res = decode(&s);
  } else {
    struct sv filename_sv = basename(filepath);
    printf("--- test\\%.*s execution ---\n",
        (i32)(filename_sv.end - filename_sv.begin), filename_sv.begin);
    printf(
"mov ax, 1 ; ax:0x0->0x1 \n"
"mov bx, 2 ; bx:0x0->0x2 \n"
"mov cx, 3 ; cx:0x0->0x3 \n"
"mov dx, 4 ; dx:0x0->0x4 \n"
"mov sp, 5 ; sp:0x0->0x5 \n"
"mov bp, 6 ; bp:0x0->0x6 \n"
"mov si, 7 ; si:0x0->0x7 \n"
"mov di, 8 ; di:0x0->0x8 \n"
"\n"
"Final registers:\n"
"      ax: 0x0001 (1)\n"
"      bx: 0x0002 (2)\n"
"      cx: 0x0003 (3)\n"
"      dx: 0x0004 (4)\n"
"      sp: 0x0005 (5)\n"
"      bp: 0x0006 (6)\n"
"      si: 0x0007 (7)\n"
"      di: 0x0008 (8)\n"
"\n"
);
    res = 1;
  }

  munmap(file_data, file_size);
  return res ? 0 : 1;
}
