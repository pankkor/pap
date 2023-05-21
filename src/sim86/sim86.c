// Performance-Aware-Programming Course
// https://www.computerenhance.com/p/table-of-contents
//
// Part 1
// 8086 instructions decode

// unity build begin
#include "sim86_decode.c"
#include "sim86_instr.c"
#include "sim86_print.c"
#include "sim86_simulate.c"
#include "sim86_stream.c"
// unity build end

#include "sim86_decode.h"
#include "sim86_print.h"
#include "sim86_simulate.h"
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
  printf("bits 16\n");
  u8 *sbegin = s->data;

  while (s->data < s->end) {
    struct instr instr = decode_next_instr(s);
    if (!instr.str) {
      fprintf(stderr, "Error: unsupported instruction at byte number %ld\n",
          s->data - sbegin);
      return 0;
    }

    print_instr(&instr);
    printf("\n");
  }
  return 1;
}

int simulate(struct stream *s, bool print_no_ip) {
  u8 *sbegin = s->data;

  struct state state = {0};
  while (s->data < s->end) {
    struct instr instr = decode_next_instr(s);
    if (!instr.str) {
      fprintf(stderr, "Error: unsupported instruction at byte number %ld\n",
          s->data - sbegin);
      return 0;
    }

    struct state old_state = state;
    state = state_simulate_instr(&old_state, &instr);
    s->data = sbegin + state.ip; // read next instruction based on simulated IP

    print_instr(&instr);
    printf(" ; ");
    print_state_diff(&old_state, &state, print_no_ip);
    printf("\n");
  }
  printf("\nFinal registers:\n");
  print_state_registers(&state, print_no_ip);
  printf("\n");
  return 1;
}

int main(int argc, char **argv) {
  if (argc < 2) {
    fprintf(stderr,
        "Usage:\nsim86 [COMMAND] <input_binary_file>\n"
        "\n"
        "COMMAND\n"
        "    simulate             - simulate executing binary file and print\n"
        "                           results to stdout (default).\n"
        "    simulate-print-no-ip - same as 'simulate', but doesn't print\n"
        "                           chagnes to IP register.\n"
        "    decode               - decode binary file to stdout using\n"
        "                           NASM syntax.\n"
        );
    return 1;
  }

  enum {SIMULATE, SIMULATE_PRINT_NO_IP, DECODE} command = SIMULATE;
  if (argc == 3) {
    if (strcmp(argv[1], "decode") == 0) {
      command = DECODE;
    } else if (strcmp(argv[1], "simulate-print-no-ip") == 0) {
      command = SIMULATE_PRINT_NO_IP;
    }
  }

  const char *filepath = argv[argc - 1];
  int fd = open(filepath, O_RDONLY);
  if (fd == -1) {
    fprintf(stderr, "Error: failed to open file '%s': ", filepath);
    perror("");
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

  decode_build_index();

  int res;
  if (command == DECODE) {
    res = decode(&s);
  } else {
    struct sv filename_sv = basename(filepath);
    printf("--- test\\%.*s execution ---\n",
        (i32)(filename_sv.end - filename_sv.begin), filename_sv.begin);
    res = simulate(&s, command == SIMULATE_PRINT_NO_IP);
  }

  munmap(file_data, file_size);
  return res ? 0 : 1;
}
