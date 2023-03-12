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
#define ARRAY_COUNT(x)  (u64)(sizeof(x) / sizeof(x[0]))

const char s_reg_b_strs[][3] = {"al", "cl", "dl", "bl", "ah", "ch", "dh", "bh"};
const char s_reg_w_strs[][3] = {"ax", "cx", "dx", "bx", "sp", "bp", "si", "di"};
const char s_seg_strs[][3]   = {"es", "cs", "ss", "ds"};

const char *str_instr(u8 o) {
  assert(o < 8 && "Octal expected");
  switch(o) {
    case 0: return "add";
    case 2: return "adc";
    case 3: return "sbb";
    case 5: return "sub";
    case 7: return "cmp";
  }
  return "<error>";
}

FORCE_INLINE const char *str_reg_byte(u8 o) {
  assert(o < ARRAY_COUNT(s_reg_b_strs) && "Octal expected");
  return s_reg_b_strs[o];
}

FORCE_INLINE const char *str_reg_word(u8 o) {
  assert(o < ARRAY_COUNT(s_reg_w_strs) && "Octal expected");
  return s_reg_w_strs[o];
}

// TODO: unused
FORCE_INLINE const char *str_seg_reg(u8 o) {
  assert(o < ARRAY_COUNT(s_seg_strs) && "Octal expected");
  return s_seg_strs[o];
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
    case 0: return (struct ea){"bx", "si", mod};
    case 1: return (struct ea){"bx", "di", mod};
    case 2: return (struct ea){"bp", "si", mod};
    case 3: return (struct ea){"bp", "di", mod};
    case 4: return (struct ea){"si", NULL, mod};
    case 5: return (struct ea){"di", NULL, mod};
    case 6: return (struct ea){"bp", NULL, mod};
    case 7: return (struct ea){"bx", NULL, mod};
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


// 1st byte -> whole instruciton
//          -> instruction data part1
// -> (2nd byte (src, dst,  )

// decode 1st byte
// jump to (RM_REG
// RM_REG

enum instr_fmt {
  INSTR_FMT_NO_DATA = 0,
  INSTR_FMT_DISPL,
  INSTR_FMT_DISPL_DATA_W,
  INSTR_FMT_DATA_W,
  INSTR_FMT_DATA_8,
  INSTR_FMT_DATA_16,
};

struct instr {
  u8 instr;
  const char *str;
  enum instr_fmt fmt;
};

// Instructio tables
struct instr s_E7_instrs[] = {
  {0x06, "push",  INSTR_FMT_NO_DATA},     // 000r g110    - push SEG
  {0x07, "pop",   INSTR_FMT_NO_DATA}      // 000r g111    - pop SEG
};

struct instr s_F0_instrs[] = {
  {0xB0, "mov",   INSTR_FMT_DATA_W},      // 1011 wreg    - mov IMM to REG
};

struct instr s_F8_instrs[] = {
  {0x50, "push",  INSTR_FMT_NO_DATA},     // 0101 0reg    - push REG
  {0x58, "pop",   INSTR_FMT_NO_DATA},     // 0101 1reg    - pop REG
};

struct instr s_FC_instrs[] = {
  {0x88, "mov",   INSTR_FMT_DISPL},       // 1000 10dw    - mov R/M to/from REG
  {0xA0, "mov",   INSTR_FMT_DATA_16},     // 1010 00dw    - mov mem to/from ACC
  {0x00, "add",   INSTR_FMT_DISPL},       // 0000 00dw    - add R/M + REG
  {0x80, "add",   INSTR_FMT_DISPL_DATA_W},// 0000 00dw    - add imm + R/M
};

struct instr s_FE_instrs[] = {
  {0xC6, "mov",   INSTR_FMT_DISPL_DATA_W},// 1100 011w    - mov imm to R/M
  {0x04, "add",   INSTR_FMT_DATA_16},     // 0000 00dw    - add imm + ACC
};

struct instr s_FF_instrs[] = {
  {0x8C, "mov",   INSTR_FMT_DISPL},       // 1000 1100    - mov SEG to R/M
  {0x8E, "mov",   INSTR_FMT_DISPL},       // 1000 1110    - mov R/M to SEG
  {0x8F, "pop",   INSTR_FMT_DISPL},       // 1000 1111    - pop R/M
  {0xFF, "push",  INSTR_FMT_DISPL},       // 1111 1111    - push R/M
};

enum instr_mask {
  INSTR_MASK_E7 = 0,  // 111X X111
  INSTR_MASK_F0 = 1,  // 1111 XXXX
  INSTR_MASK_F8 = 2,  // 1111 1XXX
  INSTR_MASK_FC = 3,  // 1111 11XX
  INSTR_MASK_FE = 4,  // 1111 111X
  INSTR_MASK_FF = 5,  // 1111 1111
  INSTR_MASK_COUNT = 6
};

u8 s_instr_masks[] = { 0xE7, 0xF0, 0xF8, 0xFC, 0xFE, 0xFF };

struct instrs_table {
  struct instr *instrs;
  u64 size;
};

struct instrs_table s_instr_tables[] = {
  {s_E7_instrs, ARRAY_COUNT(s_E7_instrs)},
  {s_F0_instrs, ARRAY_COUNT(s_F0_instrs)},
  {s_F8_instrs, ARRAY_COUNT(s_F8_instrs)},
  {s_FC_instrs, ARRAY_COUNT(s_FC_instrs)},
  {s_FE_instrs, ARRAY_COUNT(s_FE_instrs)},
  {s_FF_instrs, ARRAY_COUNT(s_FF_instrs)},
};

_Static_assert(ARRAY_COUNT(s_instr_masks) == INSTR_MASK_COUNT, "Wrong masks");
_Static_assert(ARRAY_COUNT(s_instr_tables) == INSTR_MASK_COUNT, "Wrong tables");

struct instr decode_instr(u8 b) {
  for (int i = 0; i < INSTR_MASK_COUNT; ++i) {
    struct instrs_table table = s_instr_tables[i];
    u8 mask = s_instr_masks[i];

    // for now just brute force, build index later when tables grow
    u8 instr = b & mask;
    for (u64 i = 0; i < table.size; ++i) {
      if (table.instrs[i].instr == instr) {
        return table.instrs[i];
      }
    }
  }
  return (struct instr){0, "unsupported instruction", INSTR_FMT_NO_DATA};
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
    struct instr instr = decode_instr(b0);

    if (instr.instr == 0) {
      fprintf(stderr, "Error: unsupported instruction '0x%02X' at %ld\n",
          b0, s->data - sbegin);
      return 0;
    }

    printf("%s ", instr.str);

    switch(instr.fmt) {
      case INSTR_FMT_NO_DATA:
        break;
      case INSTR_FMT_DISPL: {
// {0x88, "mov",   INSTR_FMT_DISPL},       // 1000 10dw    - mov RM to/from REG
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
          printf(" %s, %s\n", dst_str, src_str);
        } else {
          struct ea ea = effective_address(rm, mod);

          i16 displ = 0;
          if (ea.displ != DISPL_0) {
            CHECK(stream_read_sign(s, &displ, ea.displ == DISPL_W),
                "Error: mov no displacement\n");
          }

          if (!d) {
            print_ea(ea, displ);
            printf(", %s\n", w ? str_reg_word(reg) : str_reg_byte(reg));
          } else {
            printf(" %s, ", w ? str_reg_word(reg) : str_reg_byte(reg));
            print_ea(ea, displ);
            printf("\n");
          }
        }
      } break;
      case INSTR_FMT_DATA_W: {
// {0xB0, "mov",   INSTR_FMT_DATA_W},      // 1011 wreg    - mov IMM to REG
        u8 w = decode_bit(b0, 3);
        u8 reg = decode_3bits(b0, 0);

        u16 imm = 0;
        CHECK(stream_read(s, &imm, w), "Error: mov no immediate\n");

        const char *dst_str = w ? str_reg_word(reg) : str_reg_byte(reg);
        printf(" %s, %d\n", dst_str, imm);
      } break;
      case INSTR_FMT_DISPL_DATA_W: {
// {0xC6, "mov",   INSTR_FMT_DISPL_DATA_W},  // 1100 011w    - mov imm to R/M
        u8 b1;
        CHECK(stream_read_b(s, &b1), "Error: mov requires a second byte\n");

        u8 w = decode_w(b0);
        u8 mod = decode_mod(b1);
        u8 rm  = decode_rm(b1);

        if (mod == 3) {
          // registers, no displacement follows
          printf(" %s,", w ? str_reg_word(rm) : str_reg_byte(rm));
        } else {
          struct ea ea = effective_address(rm, mod);

          i16 displ = 0;
          if (ea.displ != DISPL_0) {
            CHECK(stream_read_sign(s, &displ, ea.displ == DISPL_W),
                "Error: mov no displacement\n");
          }

          print_ea(ea, displ);
          printf(" , %s", w ? "word" : "byte");
        }

        u16 imm = 0;
        CHECK(stream_read(s, &imm, w), "Error: mov no immediate\n");

        printf(" %d\n", imm);
      } break;
      case INSTR_FMT_DATA_16: {
// {0xA0, "mov",   INSTR_FMT_DISPL_DATA_W},  // 1010 00dw    - mov mem to/from ACC
        // mem to/from acc
        u8 d = decode_d(b0);
        u8 w = decode_w(b0);

        u16 addr;
        CHECK(stream_read_w(s, &addr), "Error: mov no address word\n");
        const char *reg = w ? "ax" : "al";

        if (d) {
          printf(" [%u], %s\n", addr, reg);
        } else {
          printf(" %s, [%u]\n", reg, addr);
        }
      } break;
    }

    // TODO extract arguments from above?


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
