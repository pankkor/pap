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

int simulate(struct stream *s, bool print_no_ip,
    const char *memory_dump_filename) {
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

  if (memory_dump_filename) {
    int dump_fd = open(memory_dump_filename, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR);

    if (dump_fd == -1) {
      fprintf(stderr, "Error: failed to open memory dump file '%s': ",
          memory_dump_filename);
      perror("");
      return 0;
    }

    if (write(dump_fd, state.memory, MEMORY_SIZE) != MEMORY_SIZE) {
      fprintf(stderr, "Error: failed to write memory dump file '%s': ",
          memory_dump_filename);
      perror("");
      return 0;
    }
  }

  return 1;
}

void print_usage() {
    fprintf(stderr,
        "Usage:\nsim86 <COMMAND> [OPTIONS] <input_binary_file>\n"
        "\n"
        "COMMAND\n"
        "    simulate             - simulate executing binary file and print\n"
        "                           results to stdout (default).\n"
        "    decode               - decode binary file to stdout using\n"
        "                           NASM syntax.\n"
        "\n"
        "OPTIONS\n"
        "'simualte' command options\n"
        "    --print-no-ip        - same as 'simulate', but doesn't print\n"
        "                           chagnes to IP register.\n"
        "    --dump <filename>    - after simulation has finished dump the\n"
        "                           content of memory to the file.\n"
        );
}

int main(int argc, char **argv) {
  if (argc < 2) {
    print_usage();
    return 1;
  }

  enum {SIMULATE, DECODE} command = SIMULATE;
  if (strcmp(argv[1], "decode") == 0) {
    command = DECODE;
  } else if (strcmp(argv[1], "simulate") == 0) {
    command = SIMULATE;
  } else {
    fprintf(stderr, "Error: unknown COMMAND '%s'\n", argv[1]);
    print_usage();
    return 1;
  }

  // options
  bool print_no_ip = false;
  const char *memory_dump_filename = NULL;

  int filename_argc = argc - 1;
  if (command == SIMULATE) {
    int cur_argc = 2;
    while (cur_argc < filename_argc) {
      if (strcmp(argv[cur_argc], "--print-no-ip") == 0) {
        print_no_ip = true;
      } else if (strcmp(argv[cur_argc], "--dump") == 0) {
        if (++cur_argc < filename_argc) {
          memory_dump_filename = argv[cur_argc];
        } else {
          fprintf(stderr, "Error: '--dump' option requires a filename arg\n");
          print_usage();
          return 1;
        }
      } else {
        fprintf(stderr, "Error: unrecognized option '%s'\n", argv[cur_argc]);
        print_usage();
        return 1;
      }
      ++cur_argc;
    }
  }

  const char *filepath = argv[filename_argc];
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
    res = simulate(&s, print_no_ip, memory_dump_filename);
  }

  munmap(file_data, file_size);
  return res ? 0 : 1;
}
