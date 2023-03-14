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
#define SWAP(a, b)            \
  do {                        \
    __typeof__(a) tmp = (a);  \
    (a) = (b);                \
    (b) = tmp;                \
  } while(0)

const char s_reg_b_strs[][3] = {"al", "cl", "dl", "bl", "ah", "ch", "dh", "bh"};
const char s_reg_w_strs[][3] = {"ax", "cx", "dx", "bx", "sp", "bp", "si", "di"};
const char s_seg_strs[][3]   = {"es", "cs", "ss", "ds"};

FORCE_INLINE const char *str_reg_byte(u8 o) {
  assert(o < ARRAY_COUNT(s_reg_b_strs) && "Octal expected");
  return s_reg_b_strs[o];
}

FORCE_INLINE const char *str_reg_word(u8 o) {
  assert(o < ARRAY_COUNT(s_reg_w_strs) && "Octal expected");
  return s_reg_w_strs[o];
}

FORCE_INLINE const char *str_seg_reg(u8 o) {
  assert(o < ARRAY_COUNT(s_seg_strs) && "Octal expected");
  return s_seg_strs[o];
}

const char *str_instr(u8 o) {
  assert(o < 8 && "Octal expected");
  switch (o) {
    case 0: return "add";
    case 2: return "adc";
    case 3: return "sbb";
    case 5: return "sub";
    case 7: return "cmp";
  }
  return "<error>";
}

// file stream

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
// |    Byte 0     | |    Byte 1     |
// |7 6 5 4 3 2|1 0| |7 6|5 4 3|2 1 0| ...
// | OPCODE    |D|W| |MOD| REG | R/M |
//
// Byte 2-5 encode optionasl displacement or immediate.
// Special case instructions might miss some flags and have them at different
// positions.

// Effective address calculation
enum ea_type {
  EA_DISPL_0 =   0,
  EA_DISPL_B =   1,
  EA_DISPL_W =   2,
  EA_DIR_ADDR =  3,
};

struct ea {
  const char *op0;
  const char *op1;
  union {
    i16 displ;  // type: EA_DISPL_B, EA_DISPL_W
    u16 addr;   // type: EA_DIR_ADDR
  };
  enum ea_type type;
};

struct ea effective_address(u8 rm, u8 mod) {
  assert(rm < 8 && "Octal expected");
  assert(mod < 3 && "2 bit mod expected");

  if (rm == 6 && mod == 0) {
    return (struct ea){ .type = EA_DIR_ADDR };
  }

  switch (rm) {
    case 0: return (struct ea){"bx", "si", .displ = 0, mod};
    case 1: return (struct ea){"bx", "di", .displ = 0, mod};
    case 2: return (struct ea){"bp", "si", .displ = 0, mod};
    case 3: return (struct ea){"bp", "di", .displ = 0, mod};
    case 4: return (struct ea){"si", NULL, .displ = 0, mod};
    case 5: return (struct ea){"di", NULL, .displ = 0, mod};
    case 6: return (struct ea){"bp", NULL, .displ = 0, mod};
    case 7: return (struct ea){"bx", NULL, .displ = 0, mod};
  }
  return (struct ea){0};
}

// Instruction data format
enum instr_fmt {
  INSTR_FMT_NO_DATA,
  INSTR_FMT_DATASW,
  INSTR_FMT_DATAW,
  INSTR_FMT_DATA8,
  INSTR_FMT_DATA16,
  INSTR_FMT_ADDR,
};

struct instr_table_row {
  const char *str;
  u8 opcode;        // masked first instr byte instruction should match opcode
  u8 with_acc;
  u8 has_displ;
  enum instr_fmt fmt;
};

// Instruction tables
struct instr_table_row s_E7_instrs[] = {
  {"push",  0x06, 0, 0, INSTR_FMT_NO_DATA}, // 000s g110    - push SEG
  {"pop",   0x07, 0, 0, INSTR_FMT_NO_DATA}  // 000s g111    - pop SEG
};

struct instr_table_row s_F0_instrs[] = {
  {"mov",   0xB0, 0, 0, INSTR_FMT_DATAW},   // 1011 wreg    - mov IMM to REG
};

struct instr_table_row s_F8_instrs[] = {
  {"push",  0x50, 0, 0, INSTR_FMT_NO_DATA}, // 0101 0reg    - push REG
  {"pop",   0x58, 0, 0, INSTR_FMT_NO_DATA}, // 0101 1reg    - pop REG
};

struct instr_table_row s_FC_instrs[] = {
  {"mov",   0x88, 0, 1, INSTR_FMT_NO_DATA}, // 1000 10dw    - mov R/M to/f REG
  {"mov",   0xA0, 1, 0, INSTR_FMT_ADDR},    // 1010 00dw    - mov MEM to/f ACC
  {"add",   0x00, 0, 1, INSTR_FMT_NO_DATA}, // 0000 00dw    - add R/M + REG
  {"add",   0x80, 0, 1, INSTR_FMT_DATASW},  // 1000 00sw    - add R/M + IMM
  {"sub",   0x80, 0, 1, INSTR_FMT_DATASW},  // 1000 00sw    - sub R/M - IMM
};

struct instr_table_row s_FE_instrs[] = {
  {"mov",   0xC6, 0, 1, INSTR_FMT_DATAW},   // 1100 011w    - mov IMM to R/M
  {"add",   0x04, 1, 0, INSTR_FMT_DATA16},  // 0000 010w    - add ACC + IMM
};

struct instr_table_row s_FF_instrs[] = {
  {"mov",   0x8C, 0, 1, INSTR_FMT_NO_DATA}, // 1000 1100    - mov SEG to R/M
  {"mov",   0x8E, 0, 1, INSTR_FMT_NO_DATA}, // 1000 1110    - mov R/M to SEG
  {"pop",   0x8F, 0, 1, INSTR_FMT_NO_DATA}, // 1000 1111    - pop R/M
  {"push",  0xFF, 0, 1, INSTR_FMT_NO_DATA}, // 1111 1111    - push R/M
};

// Instruction mask determines location of flags (d, w, etc) in the first byte
enum instr_mask {
  INSTR_MASK_E7,    // 111X X111
  INSTR_MASK_F0,    // 1111 XXXX
  INSTR_MASK_F8,    // 1111 1XXX
  INSTR_MASK_FC,    // 1111 11XX
  INSTR_MASK_FE,    // 1111 111X
  INSTR_MASK_FF,    // 1111 1111
  INSTR_MASK_COUNT
};

u8 s_instr_masks[] = { 0xE7, 0xF0, 0xF8, 0xFC, 0xFE, 0xFF };

struct instrs_table {
  struct instr_table_row *rows;
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

_Static_assert(ARRAY_COUNT(s_instr_masks) == INSTR_MASK_COUNT, "Bad masks");
_Static_assert(ARRAY_COUNT(s_instr_tables) == INSTR_MASK_COUNT, "Bad tables");

// Operands
enum operand_type { OP_NONE, OP_EA, OP_DATA8, OP_DATA16, OP_REG };

struct operand {
  union {
    struct ea   ea;
    u8          data8;
    u16         data16;
    u16         addr;
    const char* reg;
  };
  enum operand_type type;
};

struct instr {
  const char *str;
  struct operand op0;
  struct operand op1;
};

// decode error check helper
#define CHECK(op, err_msg)        \
    if (!(op)) {                  \
      fprintf(stderr, (err_msg)); \
      return ret;                 \
    }

// Decode
// b0 = next byte from the byte stream
//
// find instruction row matching first byte b0
// for each row in instruction tables
//     mask b0 with row.mask and compare to row opcode
//
// depending on instruction row's mask decode d, w, reg flags from b0
//
// if instruction row has displacement
//   read displacement byte1 from the byte stream
//   depending on mod rm fields read subsequent displacement bytes
//   if instructino has no data
//       fill in reg flag
//
// if instruction row has data
//   read data from the byte stream according to row's data format
//
// if d flag is set
//   swap src and dst operands
//
// instruction is ready
struct instr decode_next_instr(struct stream *stream) {
  struct instr ret = {0};

  u8 b0;
  if (!stream_read_b(stream, &b0)) {
    return ret;
  }

  // decode byte 0
  struct instr_table_row *row = NULL;
  enum instr_mask instr_mask_type = 0;

  // for now just brute force, build index later when tables grow
  for (int i = 0; i < INSTR_MASK_COUNT; ++i) {
    struct instrs_table table = s_instr_tables[i];
    u8 mask = s_instr_masks[i];

    u8 opcode = b0 & mask;
    for (u64 j = 0; j < table.size; ++j) {
      if (table.rows[j].opcode == opcode) {
        row = &table.rows[j];
        instr_mask_type = i;
        goto instr_found; // break out of outer loop
      }
    }
  }
  if (!row) {
    return ret;
  }
instr_found:

  // decode remaing bytes
  ret.str = row->str; // TODO find instructions with part of opcode in byte1

  // byte0 flags
  u8 w = 1;
  // s & d share the same position in byte0
  u8 d = 0; // d - instruction has displacement w/o data
  u8 s = 0; // s - instruction has displacement with immediate
  switch (instr_mask_type) {
    // XXXs gXXX  - sg 2-bit segment register
    case INSTR_MASK_E7:
      ret.op0 = (struct operand){
        .reg = str_seg_reg((b0 & 0x18) >> 3),
        .type = OP_REG
      };
      break;

    // 1011 wreg
    case INSTR_MASK_F0:
      w = (b0 & 8) >> 3;
      // fallthrough

    // XXXX Xreg
    case INSTR_MASK_F8: {
      u8 reg = b0 & 0x07;
      ret.op0 = (struct operand){
        .reg = w ? str_reg_word(reg) : str_reg_byte(reg),
        .type = OP_REG
      };
    } break;

    // XXXX XXdw
    case INSTR_MASK_FC:
      s = (b0 & 0x02) >> 1;
      if ((row->has_displ && row->fmt == INSTR_FMT_NO_DATA)
          || row->fmt == INSTR_FMT_ADDR) {
        SWAP(s, d);
      }
      // fallthrough

    // XXXX XXXw
    case INSTR_MASK_FE:
      w = b0 & 0x01;
      break;

    // XXXX XXXX
    case INSTR_MASK_FF:
      break;

    case INSTR_MASK_COUNT:
      break;
  }

  if (row->with_acc) {
    assert(ret.op0.type == OP_NONE && "op0 was set above");
    ret.op0 = (struct operand){
      .reg = w ? "ax" : "al",
      .type = OP_REG
    };
  }

  // TODO
  u8 opcode_aux; // part of opcode in Byte 1

  // read displacement
  if (row->has_displ) {
    u8 b1;
    CHECK(stream_read_b(stream, &b1), "Error: failed to read displacement\n");

    u8 mod = (b1 & 0xC0) >> 6;
    u8 reg = (b1 & 0x38) >> 3;
    u8 rm  = b1 & 0x07;

    struct operand reg_op = {
        .reg = w ? str_reg_word(reg) : str_reg_byte(reg),
        .type = OP_REG
      };
    struct operand rm_op = {
        .reg = w ? str_reg_word(rm) : str_reg_byte(rm),
        .type = OP_REG
      };

    if (row->fmt == INSTR_FMT_NO_DATA) {
      // treat reg as one of the operands if no data follows
      if (mod == 3) {
        ret.op0 = rm_op;
      } else {
        struct ea ea = effective_address(rm, mod);
        if (ea.type != EA_DISPL_0) {
          CHECK(stream_read_sign(stream, &ea.displ, ea.type != EA_DISPL_B),
              "Error: mov no displacement\n");
        }
        ret.op0 = (struct operand){.ea = ea, .type = OP_EA};
      }
      ret.op1 = reg_op;
    } else {
      // treat reg as auxiliary part of the opcode
      opcode_aux = reg;
      if (mod == 3) {
          ret.op0 = rm_op;
      } else {
        struct ea ea = effective_address(rm, mod);
        if (ea.type != EA_DISPL_0) {
          CHECK(stream_read_sign(stream, &ea.displ, ea.type != EA_DISPL_B),
              "Error: mov no displacement\n");
        }
        ret.op0 = (struct operand){.ea = ea, .type = OP_EA};
      }
    }
  }

  // read data
  bool read_word = w;
  bool should_read_data = true;
  switch (row->fmt) {
    case INSTR_FMT_DATASW: read_word = !s && w; break;
    case INSTR_FMT_DATAW: read_word = w; break;
    case INSTR_FMT_DATA8: read_word = false; break;
    case INSTR_FMT_DATA16: read_word = true; break;
    case INSTR_FMT_ADDR: read_word = true; break;
    default: should_read_data = false; break;
  }

  if (should_read_data) {
    u16 data = 0;
    CHECK(stream_read(stream, &data, read_word), "Error: failed to read data\n");
    // TODO handle s and sign extension

    if (row->fmt == INSTR_FMT_ADDR) {
      struct ea ea = { .addr = data, .type = EA_DIR_ADDR };
      ret.op1 = (struct operand){.ea = ea, .type = OP_EA};
    } else {
      ret.op1 = w ? (struct operand){.data16 = data, .type = OP_DATA16}
                  : (struct operand){.data8 = data, .type = OP_DATA8};
    }
  }

  if (d) {
    SWAP(ret.op0, ret.op1);
  }

  return ret;
}

void print_ea(const struct ea *ea) {
  if (ea->type == EA_DIR_ADDR) {
    printf("[%u]", ea->addr);
    return;
  }

  printf("[%s", ea->op0);
  if (ea->op1) {
    printf(" + %s", ea->op1);
  }
  if (ea->displ != EA_DISPL_0) {
    const char *op = ea->displ < 0 ? "-" : "+";
    u16 udispl = ea->displ < 0 ? ~ea->displ + 1 : ea->displ;
    printf(" %s %u", op, udispl);
  }
  printf("]");
}

void print_operand(const struct operand *op) {
  switch (op->type) {
    case OP_NONE:
      break;
    case OP_EA:
      print_ea(&op->ea);
      break;
    case OP_DATA8:
    case OP_DATA16:
      printf("%u", op->data16);
      break;
    case OP_REG:
      printf("%s", op->reg);
      break;
  }
}

void print_instr(const struct instr *instr) {
  const struct operand *op0 = &instr->op0;
  const struct operand *op1 = &instr->op1;

  printf("%s", instr->str);
  if (op0->type != OP_NONE) {
    printf(" ");
    print_operand(&instr->op0);
  }
  if (op1->type != OP_NONE) {
    printf(", ");
    if (op0->type == OP_EA
        && (op1->type == OP_DATA8 || op1->type == OP_DATA16)) {
      printf("%s ", op1->type == OP_DATA16 ? "word" : "byte");
    }
    print_operand(&instr->op1);
  }
  printf("\n");
}

int decode(struct stream *s) {
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
