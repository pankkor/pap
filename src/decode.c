// Performance-Aware-Programming Course
// https://www.computerenhance.com/p/table-of-contents
//
// Part 1
// 8086 instructions decode
//
// Run 'test_decode.sh' that tests decoding on all the listings in listings dir

#include <assert.h>     // assert
#include <unistd.h>     // lseek
#include <fcntl.h>      // open
#include <sys/types.h>  // legacy
#include <sys/mman.h>   // mmap
#include <stdio.h>      // printf
#include <stdbool.h>    // bool true false

typedef signed char     i8;
typedef unsigned char   u8;
typedef short           i16;
typedef unsigned short  u16;
typedef unsigned long   u64;

#define FORCE_INLINE    inline __attribute__((always_inline))

FORCE_INLINE const char *str_reg_byte(u8 o) {
  assert(o < 8 && "Octal expected");
  switch (o) {
    case 0: return "al";
    case 1: return "cl";
    case 2: return "dl";
    case 3: return "bl";
    case 4: return "ah";
    case 5: return "ch";
    case 6: return "dh";
    case 7: return "bh";
  }
  return "<error>";
}

FORCE_INLINE const char *str_reg_word(u8 o) {
  assert(o < 8 && "Octal expected");
  switch (o) {
    case 0: return "ax";
    case 1: return "cx";
    case 2: return "dx";
    case 3: return "bx";
    case 4: return "sp";
    case 5: return "bp";
    case 6: return "si";
    case 7: return "di";
  }
  return "<error>";
}

// TODO: unused
FORCE_INLINE const char *str_seg_reg(u8 o) {
  assert(o < 8 && "Octal expected");
  switch (o) {
    case 0: return "es";
    case 1: return "cs";
    case 2: return "ss";
    case 3: return "ds";
    case 4:
    case 5:
    case 6:
    case 7: return "<reserved>";
  }
  return "<error>";
}

struct stream {
  u8 * restrict data;
  u8 * restrict end;
};

bool stream_read_b(struct stream *s, u8 * restrict out_byte) {
  if (s->data < s->end) {
    *out_byte = *s->data++;
    return true;
  }
  return false;
}

bool stream_read_w(struct stream *s, u16 * restrict out_word) {
  if (s->data + 1 < s->end) {
    *out_word = *((u16 *)s->data);
    s->data += 2;
    return true;
  }
  return false;
}

FORCE_INLINE bool stream_read(struct stream *s, u16 * restrict out_word,
    bool is_word) {
  return is_word ? stream_read_w(s, out_word) : stream_read_b(s, (u8 *)out_word);
}

// read byte is sign extended
FORCE_INLINE bool stream_read_sign(struct stream *s, i16 * restrict out_word,
    bool is_word) {
  if (is_word) {
    return stream_read_w(s, (u16 *)out_word);
  } else {
    i8 lo;
    bool res = stream_read_b(s, (u8 *)&lo);
    *out_word = lo; // sign extend
    return res;
  }
}

// Generic 8086 instruction encoding: 1-6 bytes
//
// |    Byte 1     | |    Byte 2     |
// |7 6 5 4 3 2|1 0| |7 6|5 4 3|2 1 0| ...
// | OPCODE    |D|W| |MOD| REG | R/M |
//
// Byte 3-6 encode optionasl displacement or immediate.
// Special case instructions might miss some flags and have them at different
// positions.
FORCE_INLINE u8 decode_opcode(u8 b)         { return (b & 0xFC) >> 2; }
FORCE_INLINE u8 decode_d(u8 b)              { return (b & 0x02) >> 1; }
FORCE_INLINE u8 decode_w(u8 b)              { return b & 0x01; }
FORCE_INLINE u8 decode_rm(u8 b)             { return b & 0x07; }
FORCE_INLINE u8 decode_reg(u8 b)            { return (b & 0x38) >> 3; }
FORCE_INLINE u8 decode_mod(u8 b)            { return (b & 0xC0) >> 6; }
FORCE_INLINE u8 decode_bit(u8 b, u8 pos)    { return (b & (1 << pos)) >> pos; }
FORCE_INLINE u8 decode_3bits(u8 b, u8 pos)  { return (b & (7 << pos)) >> pos; }

// effective address and displacement
enum displ {
  DISPL_0 = 0,
  DISPL_B = 1,
  DISPL_W = 2,
};

struct ea {
  const char *op0;
  const char *op1;
  enum displ displ;
};

struct ea effective_address(u8 rm, u8 mod) {
  assert(rm < 8 && "Octal expected");
  assert(mod < 3 && "2 bit mod expected");

  if (rm == 6 && mod == 0) {
    return (struct ea){ .displ = DISPL_W };
  }

  switch (rm) {
    case 0: return (struct ea) {"bx", "si", mod};
    case 1: return (struct ea) {"bx", "di", mod};
    case 2: return (struct ea) {"bp", "si", mod};
    case 3: return (struct ea) {"bp", "di", mod};
    case 4: return (struct ea) {"si", NULL, mod};
    case 5: return (struct ea) {"di", NULL, mod};
    case 6: return (struct ea) {"bp", NULL, mod};
    case 7: return (struct ea) {"bx", NULL, mod};
  }
  return (struct ea){0};
}

void print_ea(struct ea ea, i16 displ) {
  if (!ea.op0) {
    printf("[%d]", displ);
    return;
  }

  printf("[%s", ea.op0);
  if (ea.op1) {
    printf(" + %s", ea.op1);
  }
  if (ea.displ != DISPL_0) {
    const char *op = displ < 0 ? "-" : "+";
    u16 udispl = displ < 0 ? ~displ + 1 : displ;
    printf(" %s %u", op, udispl);
  }
  printf("]");
}

// decode error check helper
#define CHECK(op, err_msg)        \
    if (!(op)) {                  \
      fprintf(stderr, (err_msg)); \
      return 0;                   \
    }

int decode(struct stream *s) {
  u8 *sbegin = s->data;
  u8 b0;
  while (stream_read_b(s, &b0)) {
    if ((b0 & 0xFC) == 0x88) {
      // mov reg/mem to/from reg
      u8 b1;
      CHECK(stream_read_b(s, &b1), "Error: mov requires a second byte\n");

      u8 d = decode_d(b0);
      u8 w = decode_w(b0);
      u8 mod = decode_mod(b1);
      u8 reg = decode_reg(b1);
      u8 rm  = decode_rm(b1);

      if (mod == 3) {
        // registers, no displacement follows
        u8 src = d ? rm  : reg;
        u8 dst = d ? reg : rm;
        const char *dst_str = w ? str_reg_word(dst) : str_reg_byte(dst);
        const char *src_str = w ? str_reg_word(src) : str_reg_byte(src);
        printf("mov %s, %s\n", dst_str, src_str);
      } else {
        struct ea ea = effective_address(rm, mod);

        i16 displ = 0;
        if (ea.displ != DISPL_0) {
          CHECK(stream_read_sign(s, &displ, ea.displ == DISPL_W),
              "Error: mov no displacement\n");
        }

        if (!d) {
          printf("mov ");
          print_ea(ea, displ);
          printf(", %s\n", w ? str_reg_word(reg) : str_reg_byte(reg));
        } else {
          printf("mov %s, ", w ? str_reg_word(reg) : str_reg_byte(reg));
          print_ea(ea, displ);
          printf("\n");
        }
      }
    } else if ((b0 & 0xF0) == 0xB0) {
      // mov imm to reg
      u8 w = decode_bit(b0, 3);
      u8 reg = decode_3bits(b0, 0);

      u16 imm = 0;
      CHECK(stream_read(s, &imm, w), "Error: mov no immediate\n");

      const char *dst_str = w ? str_reg_word(reg) : str_reg_byte(reg);
      printf("mov %s, %d\n", dst_str, imm);
    } else if ((b0 & 0xFE) == 0xC6) {
      // mov imm to reg/mem
      u8 b1;
      CHECK(stream_read_b(s, &b1), "Error: mov requires a second byte\n");

      u8 w = decode_w(b0);
      u8 mod = decode_mod(b1);
      u8 rm  = decode_rm(b1);

      if (mod == 3) {
        // registers, no displacement follows
        printf("mov %s,", w ? str_reg_word(rm) : str_reg_byte(rm));
      } else {
        struct ea ea = effective_address(rm, mod);

        i16 displ = 0;
        if (ea.displ != DISPL_0) {
          CHECK(stream_read_sign(s, &displ, ea.displ == DISPL_W),
              "Error: mov no displacement\n");
        }

        printf("mov ");
        print_ea(ea, displ);
        printf(", %s", w ? "word" : "byte");
      }

      u16 imm = 0;
      CHECK(stream_read(s, &imm, w), "Error: mov no immediate\n");

      printf(" %d\n", imm);
    } else if ((b0 & 0xFC) == 0xA0) {
      // mem to/from acc
      u8 d = decode_d(b0);
      u8 w = decode_w(b0);

      u16 addr;
      CHECK(stream_read_w(s, &addr), "Error: mov no address word\n");
      const char *reg = w ? "ax" : "al";

      if (d) {
        printf("mov [%u], %s\n", addr, reg);
      } else {
        printf("mov %s, [%u]\n", reg, addr);
      }
    } else {
      fprintf(stderr, "Error: unsupported instruction '0x%02X' at %ld\n",
          b0, s->data - sbegin);
      return 0;
    }
  }
  return 1;
}

int main(int argc, char **argv) {
  if (argc < 2) {
    fprintf(stderr, "Usage:\ndecode [binary_file_to_decode]\n");
    return 1;
  }

  const char *filename = argv[1];
  int fd = open(filename, O_RDONLY);
  if (fd == -1) {
    perror("failed to open file:");
    return 1;
  }

  off_t file_size = lseek(fd, 0, SEEK_END);
  u8 *file_data = mmap(NULL, file_size, PROT_READ, MAP_PRIVATE, fd, 0);
  close(fd);
  if (file_data == MAP_FAILED) {
    perror("Error: mmap failed");
    return 1;
  }

  struct stream s = {file_data, file_data + file_size};

  printf("bits 16\n");
  int res = decode(&s);

  munmap(file_data, file_size);
  return res ? 0 : 1;
}
