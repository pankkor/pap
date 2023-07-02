// Performance-Aware-Programming Course
// https://www.computerenhance.com/p/table-of-contents
//
// Part 1
// 8086 instructions decode

// Begin unity build
#include "sim86_clocks.c"
#include "sim86_decode.c"
#include "sim86_instr.c"
#include "sim86_print.c"
#include "sim86_simulate.c"
#include "sim86_stream.c"
// End unity build

#include "sim86_clocks.h"
#include "sim86_decode.h"
#include "sim86_print.h"
#include "sim86_simulate.h"
#include "sim86_stream.h"

#include <stdio.h>      // printf fprintf fopen fread fwrite fseek ftell
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

bool decode(struct memory *memory, u64 file_size) {
  struct stream s = {memory->data, memory->data + file_size};
  u8 *sbegin = s.data;

  printf("bits 16\n");

  while (s.data < s.end) {
    struct instr instr = decode_next_instr(&s);
    if (!instr.str) {
      fprintf(stderr, "Error: unsupported instruction at byte number %ld\n",
          s.data - sbegin);
      return false;
    }

    print_instr(&instr);
    printf("\n");
  }
  return true;
}

enum opt_clocks {
  OPT_CLOCKS_NONE,
  OPT_CLOCKS_8086,
  OPT_CLOCKS_8088
};

bool simulate(struct memory *memory, u64 file_size, bool print_no_ip,
    const char *memory_dump_filename, enum opt_clocks opt_clocks) {
    struct stream s = {memory->data, memory->data + file_size};
    struct state state = {.memory = memory};

    u32 clocks_total = 0;

    u8 *sbegin = s.data;
    while (s.data < s.end) {
      struct instr instr = decode_next_instr(&s);
      if (!instr.str) {
        fprintf(stderr, "Error: unsupported instruction at byte number %ld\n",
            s.data - sbegin);
        return false;
      }

      struct state old_state = state;
      state = state_simulate_instr(&old_state, &instr);
      s.data = sbegin + state.ip; // read next instruction based on simulated IP

      struct clocks clocks = calc_clocks(&instr, opt_clocks == OPT_CLOCKS_8088);
      clocks_total += clocks.base + clocks.ea + clocks.p;

      print_instr(&instr);
      printf(" ; ");

      if (opt_clocks != OPT_CLOCKS_NONE) {
        print_clocks(clocks, clocks_total);
      }

      print_state_diff(&old_state, &state, print_no_ip);
      printf("\n");
    }
    printf("\nFinal registers:\n");
    print_state_registers(&state, print_no_ip);
    printf("\n");

  if (memory_dump_filename) {
    if (file_write(memory_dump_filename, state.memory->data, MEMORY_SIZE)
        != MEMORY_SIZE) {
      fprintf(stderr, "Error: failed to write memory dump file '%s'\n",
          memory_dump_filename);
      return false;
    }
  }

  return true;
}

void print_usage(void) {
  fprintf(stderr,
      "Usage:\n"
      "    sim86 <COMMAND> [OPTIONS] <input_binary_file>\n"
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
      "    --print-clocks-8086  - print cycle count estimation for 8086\n"
      "    --print-clocks-8088  - print cycle count estimation for 8088\n"
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
  enum opt_clocks opt_clocks = OPT_CLOCKS_NONE;

  int filename_argc = argc - 1;
  if (command == SIMULATE) {
    int cur_argc = 2;
    while (cur_argc < filename_argc) {
      if (strcmp(argv[cur_argc], "--print-no-ip") == 0) {
        print_no_ip = true;
      } else if (strcmp(argv[cur_argc], "--print-clocks-8086") == 0) {
        opt_clocks = OPT_CLOCKS_8086;
      } else if (strcmp(argv[cur_argc], "--print-clocks-8088") == 0) {
        opt_clocks = OPT_CLOCKS_8088;
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

  struct memory memory = {0};
  u64 file_size = file_read(filepath, memory.data, MEMORY_SIZE);
  if (!file_size) {
    fprintf(stderr, "Error: failed to read contents of file '%s'\n", filepath);
    return 1;
  }

  decode_build_index();

  bool res;
  if (command == DECODE) {
    res = decode(&memory, file_size);
  } else {
    res = simulate(
        &memory, file_size, print_no_ip, memory_dump_filename, opt_clocks);
  }

  return res ? 0 : 1;

}
