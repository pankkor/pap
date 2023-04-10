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
#include <sys/mman.h>   // mmap munmap
#include <stdio.h>      // printf

typedef signed char     i8;
typedef unsigned char   u8;
typedef short           i16;
typedef unsigned short  u16;
typedef unsigned long   u64;
typedef _Bool           bool;
#define true            1
#define false           0

#define FORCE_INLINE    inline __attribute__((always_inline))
#define ARRAY_COUNT(x)  (u64)(sizeof(x) / sizeof(x[0]))
#define SWAP(a, b)            \
  do {                        \
    __typeof__(a) tmp = (a);  \
    (a) = (b);                \
    (b) = tmp;                \
  } while(0)

enum reg {
  REG_A = 0,
  REG_C = 1,
  REG_D = 2,
  REG_B = 3,
  REG_SP,
  REG_BP,
  REG_SI,
  REG_DI,
  REG_ES,
  REG_CS,
  REG_SS,
  REG_DS,
};

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

FORCE_INLINE const char *str_opcode_alu0(u8 o) {
  assert(o < 8 && "Octal expected");
  switch (o) {
    case 0: return "add";
    case 1: return "or";
    case 2: return "adc";
    case 3: return "sbb";
    case 4: return "and";
    case 5: return "sub";
    case 6: return "xor";
    case 7: return "cmp";
  }
  return "<error>";
}

const char s_test[] = "test";

FORCE_INLINE const char *str_opcode_alu1(u8 o) {
  assert(o < 8 && "Octal expected");
  switch (o) {
    case 0: return s_test;
    case 2: return "not";
    case 3: return "neg";
    case 4: return "mul";
    case 5: return "imul";
    case 6: return "div";
    case 7: return "idiv";
  }
  return "<error>";
}

FORCE_INLINE const char *str_opcode_alu2(u8 o) {
  assert(o < 8 && "Octal expected");
  switch (o) {
    case 0: return "inc";
    case 1: return "dec";
    case 2: return "call";
    case 3: return "call";
    case 4: return "jmp";
    case 5: return "jmp";
    case 6: return "push";
  }
  return "<error>";
}

FORCE_INLINE const char *str_opcode_shft(u8 o) {
  assert(o < 8 && "Octal expected");
  switch (o) {
    case 0: return "rol";
    case 1: return "ror";
    case 2: return "rcl";
    case 3: return "rcr";
    case 4: return "shl";
    case 5: return "shr";
    case 7: return "sar";
  }
  return "<error>";
}

// data stream
struct stream {
  u8 * restrict data;
  u8 * restrict end;
};

FORCE_INLINE bool stream_read_b(struct stream *s, u8 * restrict out_byte) {
  if (s->data < s->end) {
    *out_byte = *s->data++;
    return true;
  }
  return false;
}

FORCE_INLINE bool stream_read_w(struct stream *s, u16 * restrict out_word) {
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
struct compute_addr {
  const char *base_reg;
  const char *index_reg;
  i16 displ;
};

enum ea_type {EA_COMPUTE, EA_DIRECT};
struct ea {
  const char *seg;
  enum ea_type type;

  union {
    struct compute_addr compute_addr;
    u16 direct_addr;
  };
};

bool decode_effective_address(struct stream *s, u8 rm, u8 mod,
    struct ea *out) {

  assert(rm < 8 && "Octal expected");
  assert(mod < 3 && "2 bit mod expected");

  if (rm == 6 && mod == 0) {
    out->type = EA_DIRECT;
    if (!stream_read(s, &out->direct_addr, true)) {
      return false;
    }
  } else {
    out->type = EA_COMPUTE;
    switch (rm) {
      case 0: out->compute_addr = (struct compute_addr){"bx", "si", 0}; break;
      case 1: out->compute_addr = (struct compute_addr){"bx", "di", 0}; break;
      case 2: out->compute_addr = (struct compute_addr){"bp", "si", 0}; break;
      case 3: out->compute_addr = (struct compute_addr){"bp", "di", 0}; break;
      case 4: out->compute_addr = (struct compute_addr){NULL, "si", 0}; break;
      case 5: out->compute_addr = (struct compute_addr){NULL, "di", 0}; break;
      case 6: out->compute_addr = (struct compute_addr){"bp", NULL, 0}; break;
      case 7: out->compute_addr = (struct compute_addr){"bx", NULL, 0}; break;
    }
    // mod == 1: 8-bit displacement, mod == 2: 16-bit displacement
    if (mod == 1 || mod == 2) {
      if (!stream_read_sign(s, &out->compute_addr.displ, mod != 1)) {
        return false;
      }
    }
  }

  return true;
}

// Instruction data format
enum instr_fmt {
  INSTR_FMT_NO_DATA,
  INSTR_FMT_DATASW,
  INSTR_FMT_DATAW,
  INSTR_FMT_DATA8,
  INSTR_FMT_DATA16,
  INSTR_FMT_ADDR,
  INSTR_FMT_IP_INC8,
  INSTR_FMT_IP_INC16,
  INSTR_FMT_SEG_ADDR,
  INSTR_FMT_SWALLOW8,
};

enum displ_fmt {
  DISPL_FMT_NONE,
  DISPL_FMT_REG_RM,
  DISPL_FMT_SR_RM,
  DISPL_FMT_RM,
};

enum instr_flag_type {
  FLAG_NONE = 0,
  FLAG_D,
  FLAG_W,
  FLAG_S,
  FLAG_V,
  FLAG_REG,
  FLAG_SEG_REG,
  FLAG_COUNT
};

struct instr_flag {
  enum instr_flag_type type;
  bool is_implicit;
  union {
    // is_implicit == 0
    struct {
      u8 pos;
      u8 size_bits;
    };
    // is_implicit == 1
    struct {
      u8 value;
      u8 is_always_wide;
    };
  };
};

struct instr_table_row {
  u8 mask;
  const char *str;
  u8 opcode;              // masked first instr byte instruction should match opcode
  struct instr_flag flags[3];
  enum displ_fmt displ_fmt;
  enum instr_fmt fmt;
};

// struct instr_table_row {
  // const char *str;
  // u8 opcode;              // masked first instr byte instruction should match opcode
  // u8 with_acc;
  // u8 d;                   // exchange operands
  // enum displ_fmt displ_fmt;
  // enum instr_fmt fmt;
// };

// Instruction tables

// Some instructions have the same opcode in Byte0, and the rest of opcode is
// encoded in the following Byte1.
const char s_alu0[]  = "<alu0>"; // add, sub, etc
const char s_alu1[]  = "<alu1>"; // mul, div, etc
const char s_alu2[]  = "<alu2>"; // inc, dec, call, jmp
const char s_shft[]  = "<shft>"; // shifts and rotations

// Byte 0 flags
#define F_SEG_REG     (struct instr_flag){FLAG_SEG_REG,  0, .pos = 3,     .size_bits = 2}
#define F_REG         (struct instr_flag){FLAG_REG,      0, .pos = 0,     .size_bits = 3}
#define F_W           (struct instr_flag){FLAG_W,        0, .pos = 0,     .size_bits = 1}
#define F_W_ALT       (struct instr_flag){FLAG_W,        0, .pos = 3,     .size_bits = 1}
#define F_D           (struct instr_flag){FLAG_D,        0, .pos = 1,     .size_bits = 1}
#define F_S           (struct instr_flag){FLAG_S,        0, .pos = 1,     .size_bits = 1}
#define F_V           (struct instr_flag){FLAG_V,        0, .pos = 1,     .size_bits = 1}

#define F_IMPL_REG_A  (struct instr_flag){FLAG_REG,      1, .value = REG_A, .is_always_wide = 0}
#define F_IMPL_REG_D  (struct instr_flag){FLAG_REG,      1, .value = REG_D, .is_always_wide = 1}
#define F_IMPL_D      (struct instr_flag){FLAG_D,        1, .value = 1}

const struct instr_table_row s_instr_rows[] = {
  {0xE7, "push",  0x06, {F_SEG_REG},                        DISPL_FMT_NONE,   INSTR_FMT_NO_DATA}, // 000s r110    - push SEG
  {0xE7, "pop",   0x07, {F_SEG_REG},                        DISPL_FMT_NONE,   INSTR_FMT_NO_DATA}, // 000s r111    - pop SEG

  {0xF0, "mov",   0xB0, {F_W_ALT, F_REG},                   DISPL_FMT_NONE,   INSTR_FMT_DATAW},   // 1011 wreg    - mov IMM to REG

  {0xF8, "push",  0x50, {F_REG},                            DISPL_FMT_NONE,   INSTR_FMT_NO_DATA}, // 0101 0reg    - push REG
  {0xF8, "pop",   0x58, {F_REG},                            DISPL_FMT_NONE,   INSTR_FMT_NO_DATA}, // 0101 1reg    - pop REG
  {0xF8, "inc",   0x40, {F_REG},                            DISPL_FMT_NONE,   INSTR_FMT_NO_DATA}, // 0100 0reg    - inc REG
  {0xF8, "dec",   0x48, {F_REG},                            DISPL_FMT_NONE,   INSTR_FMT_NO_DATA}, // 0100 1reg    - dec REG
  {0xF8, "xchg",  0x90, {F_REG, F_IMPL_REG_A},              DISPL_FMT_NONE,   INSTR_FMT_NO_DATA}, // 1001 0reg    - xchg ACC & REG
  // TODO esc is not handled
  // {"esc",   0xD8, 0, 0, DISPL_FMT_REG_RM,   INSTR_FMT_NO_DATA}, // 1101 1xxx    - escape to ext device

  {0xFC, "mov",   0x88, {F_D, F_W},                         DISPL_FMT_REG_RM, INSTR_FMT_NO_DATA}, // 1000 10dw    - mov R/M to/f REG
  {0xFC, "mov",   0xA0, {F_D, F_W, F_IMPL_REG_A},           DISPL_FMT_NONE,   INSTR_FMT_ADDR},    // 1010 00dw    - mov MEM to/f ACC
  {0xFC, "add",   0x00, {F_D, F_W},                         DISPL_FMT_REG_RM, INSTR_FMT_NO_DATA}, // 0000 00dw    - add R/M + REG
  {0xFC, "or",    0x08, {F_D, F_W},                         DISPL_FMT_REG_RM, INSTR_FMT_NO_DATA}, // 0000 10dw    - or  R/M & REG
  {0xFC, "adc",   0x10, {F_D, F_W},                         DISPL_FMT_REG_RM, INSTR_FMT_NO_DATA}, // 0001 00dw    - adc R/M + REG
  {0xFC, "and",   0x20, {F_D, F_W},                         DISPL_FMT_REG_RM, INSTR_FMT_NO_DATA}, // 0010 00dw    - and R/M & REG
  {0xFC, "sub",   0x28, {F_D, F_W},                         DISPL_FMT_REG_RM, INSTR_FMT_NO_DATA}, // 0010 10dw    - sub R/M + REG
  {0xFC, "sbb",   0x18, {F_D, F_W},                         DISPL_FMT_REG_RM, INSTR_FMT_NO_DATA}, // 0001 10dw    - sbb R/M + REG
  {0xFC, "xor",   0x30, {F_D, F_W},                         DISPL_FMT_REG_RM, INSTR_FMT_NO_DATA}, // 0011 00dw    - xor R/M ^ REG
  {0xFC, "cmp",   0x38, {F_D, F_W},                         DISPL_FMT_REG_RM, INSTR_FMT_NO_DATA}, // 0011 10dw    - cmp R/M + REG
  {0xFC, s_alu0,  0x80, {F_S, F_W},                         DISPL_FMT_RM,     INSTR_FMT_DATASW},  // 1000 00sw    - (add|sub|..) R/M IMM
  {0xFC, s_shft,  0xD0, {F_V, F_W},                         DISPL_FMT_RM,     INSTR_FMT_NO_DATA}, // 1101 00vw    - (shl|shr|..) R/M

  {0xFE, "mov",   0xC6, {F_W},                              DISPL_FMT_RM,     INSTR_FMT_DATAW},   // 1100 011w    - mov IMM to R/M
  {0xFE, "xchg",  0x86, {F_W, F_IMPL_D},                    DISPL_FMT_REG_RM, INSTR_FMT_NO_DATA}, // 1000 011w    - xchg R/M w REG
  {0xFE, "add",   0x04, {F_W, F_IMPL_REG_A},                DISPL_FMT_NONE,   INSTR_FMT_DATAW},   // 0000 010w    - add ACC + IMM
  {0xFE, "or",    0x0C, {F_W, F_IMPL_REG_A},                DISPL_FMT_NONE,   INSTR_FMT_DATAW},   // 0000 110w    - or  ACC | IMM
  {0xFE, "adc",   0x14, {F_W, F_IMPL_REG_A},                DISPL_FMT_NONE,   INSTR_FMT_DATAW},   // 1000 000w    - adc ACC + IMM
  {0xFE, "sub",   0x2C, {F_W, F_IMPL_REG_A},                DISPL_FMT_NONE,   INSTR_FMT_DATAW},   // 0010 110w    - sub ACC - IMM
  {0xFE, "and",   0x24, {F_W, F_IMPL_REG_A},                DISPL_FMT_NONE,   INSTR_FMT_DATAW},   // 0010 010w    - and ACC & IMM
  {0xFE, "sbb",   0x1C, {F_W, F_IMPL_REG_A},                DISPL_FMT_NONE,   INSTR_FMT_DATAW},   // 0001 110w    - sbb ACC - IMM
  {0xFE, s_alu2,  0xFE, {F_W},                              DISPL_FMT_RM,     INSTR_FMT_NO_DATA}, // 1111 111w    - (inc|dec|..) R/M
  {0xFE, "xor",   0x34, {F_W, F_IMPL_REG_A},                DISPL_FMT_NONE,   INSTR_FMT_DATAW},   // 0011 010w    - xor ACC ^ IMM
  {0xFE, "cmp",   0x3C, {F_W, F_IMPL_REG_A},                DISPL_FMT_NONE,   INSTR_FMT_DATAW},   // 0011 110w    - cmp ACC & IMM
  {0xFE, "in",    0xE4, {F_W, F_IMPL_REG_A},                DISPL_FMT_NONE,   INSTR_FMT_DATA8},   // 1110 010w    - in  ACC IMM8
  {0xFE, "out",   0xE6, {F_W, F_IMPL_REG_A, F_IMPL_D},      DISPL_FMT_NONE,   INSTR_FMT_DATA8},   // 1110 011w    - out ACC IMM8
  {0xFE, s_alu1,  0xF6, {F_W},                              DISPL_FMT_RM,     INSTR_FMT_NO_DATA}, // 1111 011w    - (mul|div|..) R/M
  {0xFE, "test",  0x84, {F_W},                              DISPL_FMT_REG_RM, INSTR_FMT_NO_DATA}, // 1000 010w    - test R/M & REG
  {0xFE, "test",  0xA8, {F_W, F_IMPL_REG_A},                DISPL_FMT_NONE,   INSTR_FMT_DATAW},   // 1010 100w    - test ACC & IMM
  {0xFE, "in",    0xEC, {F_W, F_IMPL_REG_A, F_IMPL_REG_D},  DISPL_FMT_NONE,   INSTR_FMT_NO_DATA}, // 1110 110w    - in  ACC dx
  {0xFE, "out",   0xEE, {F_W, F_IMPL_REG_D, F_IMPL_REG_A},  DISPL_FMT_NONE,   INSTR_FMT_NO_DATA}, // 1110 111w    - out ACC dx

  {0xFF, "mov",   0x8C, {0},                                DISPL_FMT_SR_RM,  INSTR_FMT_NO_DATA}, // 1000 1100    - mov SEG to R/M
  {0xFF, "mov",   0x8E, {0},                                DISPL_FMT_SR_RM,  INSTR_FMT_NO_DATA}, // 1000 1110    - mov R/M to SEG
  {0xFF, "pop",   0x8F, {0},                                DISPL_FMT_RM,     INSTR_FMT_NO_DATA}, // 1000 1111    - pop R/M
  {0xFF, s_alu2,  0xFF, {0},                                DISPL_FMT_RM,     INSTR_FMT_NO_DATA}, // 1111 1111    - (inc|dec|..) R/M
  {0xFF, "je",    0x74, {0},                                DISPL_FMT_NONE,   INSTR_FMT_IP_INC8}, // 0111 0100    - je /jz   IP-INC8
  {0xFF, "jl",    0x7C, {0},                                DISPL_FMT_NONE,   INSTR_FMT_IP_INC8}, // 0111 1100    - jl /jnge IP-INC8
  {0xFF, "jle",   0x7E, {0},                                DISPL_FMT_NONE,   INSTR_FMT_IP_INC8}, // 0111 1110    - jle/jng  IP-INC8
  {0xFF, "jb",    0x72, {0},                                DISPL_FMT_NONE,   INSTR_FMT_IP_INC8}, // 0111 0010    - jb /jnae IP-INC8
  {0xFF, "jbe",   0x76, {0},                                DISPL_FMT_NONE,   INSTR_FMT_IP_INC8}, // 0111 0110    - jbe/jna  IP-INC8
  {0xFF, "jp",    0x7A, {0},                                DISPL_FMT_NONE,   INSTR_FMT_IP_INC8}, // 0111 1010    - jp /jpe  IP-INC8
  {0xFF, "jo",    0x70, {0},                                DISPL_FMT_NONE,   INSTR_FMT_IP_INC8}, // 0111 0000    - jo       IP-INC8
  {0xFF, "js",    0x78, {0},                                DISPL_FMT_NONE,   INSTR_FMT_IP_INC8}, // 0111 1000    - js       IP-INC8
  {0xFF, "jne",   0x75, {0},                                DISPL_FMT_NONE,   INSTR_FMT_IP_INC8}, // 0111 0101    - jne/jnz  IP-INC8
  {0xFF, "jnl",   0x7D, {0},                                DISPL_FMT_NONE,   INSTR_FMT_IP_INC8}, // 0111 1101    - jnl/jge  IP-INC8
  {0xFF, "jg",    0x7F, {0},                                DISPL_FMT_NONE,   INSTR_FMT_IP_INC8}, // 0111 1111    - jg /jnle IP-INC8
  {0xFF, "jnb",   0x73, {0},                                DISPL_FMT_NONE,   INSTR_FMT_IP_INC8}, // 0111 0011    - jnb/jae  IP-INC8
  {0xFF, "jnbe",  0x77, {0},                                DISPL_FMT_NONE,   INSTR_FMT_IP_INC8}, // 0111 0111    - jnb/jae  IP-INC8
  {0xFF, "jnp",   0x7B, {0},                                DISPL_FMT_NONE,   INSTR_FMT_IP_INC8}, // 0111 1011    - jnp/jpo  IP-INC8
  {0xFF, "jno",   0x71, {0},                                DISPL_FMT_NONE,   INSTR_FMT_IP_INC8}, // 0111 0001    - jno      IP-INC8
  {0xFF, "jns",   0x79, {0},                                DISPL_FMT_NONE,   INSTR_FMT_IP_INC8}, // 0111 1001    - jno      IP-INC8
  {0xFF, "jcxz",  0xE3, {0},                                DISPL_FMT_NONE,   INSTR_FMT_IP_INC8}, // 1110 0011    - jsxz     IP-INC8
  {0xFF, "loop",  0xE2, {0},                                DISPL_FMT_NONE,   INSTR_FMT_IP_INC8}, // 1110 0010    - loop CX times
  {0xFF, "loopz", 0xE1, {0},                                DISPL_FMT_NONE,   INSTR_FMT_IP_INC8}, // 1110 0001    - loopz/loope
  {0xFF, "loopnz",0xE0, {0},                                DISPL_FMT_NONE,   INSTR_FMT_IP_INC8}, // 1110 0000    - loopnz/loopne
  {0xFF, "jmp",   0xEB, {0},                                DISPL_FMT_NONE,   INSTR_FMT_IP_INC8}, // 1110 1001    - jmp in SEG short
  {0xFF, "call",  0xE8, {0},                                DISPL_FMT_NONE,   INSTR_FMT_IP_INC16},// 1110 1000    - call direct in SEG
  {0xFF, "jmp",   0xE9, {0},                                DISPL_FMT_NONE,   INSTR_FMT_IP_INC16},// 1110 1001    - jmp direct in SEG
  {0xFF, "ret",   0xC2, {0},                                DISPL_FMT_NONE,   INSTR_FMT_DATA16},  // 1100 0010    - ret SP + IMM16 in SEG
  {0xFF, "ret",   0xCA, {0},                                DISPL_FMT_NONE,   INSTR_FMT_DATA16},  // 1100 1010    - ret SP + IMM16 inter SEG
  {0xFF, "ret",   0xC3, {0},                                DISPL_FMT_NONE,   INSTR_FMT_NO_DATA}, // 1100 0011    - ret in SEG
  {0xFF, "ret",   0xCB, {0},                                DISPL_FMT_NONE,   INSTR_FMT_NO_DATA}, // 1100 1011    - ret inter SEG
  {0xFF, "xlat",  0xD7, {0},                                DISPL_FMT_NONE,   INSTR_FMT_NO_DATA}, // 1101 0111    - byte to AL
  {0xFF, "lea",   0x8D, {F_IMPL_D},                         DISPL_FMT_REG_RM, INSTR_FMT_NO_DATA}, // 1000 1101    - lea R/M to REG
  {0xFF, "lds",   0xC5, {F_IMPL_D},                         DISPL_FMT_REG_RM, INSTR_FMT_NO_DATA}, // 1100 0101    - load ptr to DS
  {0xFF, "les",   0xC4, {F_IMPL_D},                         DISPL_FMT_REG_RM, INSTR_FMT_NO_DATA}, // 1100 0100    - load ptr to ES
  {0xFF, "lahf",  0x9F, {0},                                DISPL_FMT_NONE,   INSTR_FMT_NO_DATA}, // 1001 1111    - load FLAGS to AH
  {0xFF, "sahf",  0x9E, {0},                                DISPL_FMT_NONE,   INSTR_FMT_NO_DATA}, // 1001 1110    - store AH to FLAGS
  {0xFF, "call",  0x9A, {0},                                DISPL_FMT_NONE,   INSTR_FMT_SEG_ADDR},// 1001 1010    - call direct intra SEG
  {0xFF, "jmp",   0xEA, {0},                                DISPL_FMT_NONE,   INSTR_FMT_SEG_ADDR},// 1110 1010    - jmp direct intra SEG
  {0xFF, "pushf", 0x9C, {0},                                DISPL_FMT_NONE,   INSTR_FMT_NO_DATA}, // 1001 1110    - push FLAGS
  {0xFF, "popf",  0x9D, {0},                                DISPL_FMT_NONE,   INSTR_FMT_NO_DATA}, // 1001 1110    - pop FLAGS
  {0xFF, "aaa",   0x37, {0},                                DISPL_FMT_NONE,   INSTR_FMT_NO_DATA}, // 0011 0111    - ASCII adjust for add
  {0xFF, "daa",   0x27, {0},                                DISPL_FMT_NONE,   INSTR_FMT_NO_DATA}, // 0010 0111    - Dec adjust for add
  {0xFF, "aas",   0x3F, {0},                                DISPL_FMT_NONE,   INSTR_FMT_NO_DATA}, // 0011 1111    - ASCII adjust for sub
  {0xFF, "das",   0x2F, {0},                                DISPL_FMT_NONE,   INSTR_FMT_NO_DATA}, // 0010 1111    - Dec adjust for sub
  {0xFF, "aam",   0xD4, {0},                                DISPL_FMT_NONE,   INSTR_FMT_SWALLOW8},// 1101 0100    - ASCII adjust for mul
  {0xFF, "aad",   0xD5, {0},                                DISPL_FMT_NONE,   INSTR_FMT_SWALLOW8},// 1101 0101    - ASCII adjust for div
  {0xFF, "cbw",   0x98, {0},                                DISPL_FMT_NONE,   INSTR_FMT_NO_DATA}, // 1001 0100    - byte -> word
  {0xFF, "cwd",   0x99, {0},                                DISPL_FMT_NONE,   INSTR_FMT_NO_DATA}, // 1001 0101    - word -> dword
  {0xFF, "movsb", 0xA4, {0},                                DISPL_FMT_NONE,   INSTR_FMT_NO_DATA}, // 1010 0100    - movs byte SI,DI
  {0xFF, "movsw", 0xA5, {0},                                DISPL_FMT_NONE,   INSTR_FMT_NO_DATA}, // 1010 0101    - movs word SI,DI
  {0xFF, "cmpsb", 0xA6, {0},                                DISPL_FMT_NONE,   INSTR_FMT_NO_DATA}, // 1010 0100    - cmps byte SI,DI
  {0xFF, "cmpsw", 0xA7, {0},                                DISPL_FMT_NONE,   INSTR_FMT_NO_DATA}, // 1010 0101    - cmps word SI,DI
  {0xFF, "scasb", 0xAE, {0},                                DISPL_FMT_NONE,   INSTR_FMT_NO_DATA}, // 1010 1110    - scas byte SI,DI
  {0xFF, "scasw", 0xAF, {0},                                DISPL_FMT_NONE,   INSTR_FMT_NO_DATA}, // 1010 1111    - scas word SI,DI
  {0xFF, "lodsb", 0xAC, {0},                                DISPL_FMT_NONE,   INSTR_FMT_NO_DATA}, // 1010 1100    - lods byte SI,DI
  {0xFF, "lodsw", 0xAD, {0},                                DISPL_FMT_NONE,   INSTR_FMT_NO_DATA}, // 1010 1101    - lods word SI,DI
  {0xFF, "stosb", 0xAA, {0},                                DISPL_FMT_NONE,   INSTR_FMT_NO_DATA}, // 1010 1010    - stos byte SI,DI
  {0xFF, "stosw", 0xAB, {0},                                DISPL_FMT_NONE,   INSTR_FMT_NO_DATA}, // 1010 1011    - stos word SI,DI
  {0xFF, "int3",  0xCC, {0},                                DISPL_FMT_NONE,   INSTR_FMT_NO_DATA}, // 1100 1100    - int3
  {0xFF, "int",   0xCD, {0},                                DISPL_FMT_NONE,   INSTR_FMT_DATA8},   // 1100 1101    - int IMM8
  {0xFF, "into",  0xCE, {0},                                DISPL_FMT_NONE,   INSTR_FMT_NO_DATA}, // 1100 1110    - int on overflow
  {0xFF, "iret",  0xCF, {0},                                DISPL_FMT_NONE,   INSTR_FMT_NO_DATA}, // 1100 1111    - return from int
  {0xFF, "cmc",   0xF5, {0},                                DISPL_FMT_NONE,   INSTR_FMT_NO_DATA}, // 1111 0101    - complement carry
  {0xFF, "clc",   0xF8, {0},                                DISPL_FMT_NONE,   INSTR_FMT_NO_DATA}, // 1111 1000    - clear carry
  {0xFF, "stc",   0xF9, {0},                                DISPL_FMT_NONE,   INSTR_FMT_NO_DATA}, // 1111 0101    - set carry
  {0xFF, "cld",   0xFC, {0},                                DISPL_FMT_NONE,   INSTR_FMT_NO_DATA}, // 1111 1100    - clear direction
  {0xFF, "std",   0xFD, {0},                                DISPL_FMT_NONE,   INSTR_FMT_NO_DATA}, // 1111 1101    - set direction
  {0xFF, "cli",   0xFA, {0},                                DISPL_FMT_NONE,   INSTR_FMT_NO_DATA}, // 1111 1010    - clear interrupt
  {0xFF, "sti",   0xFB, {0},                                DISPL_FMT_NONE,   INSTR_FMT_NO_DATA}, // 1111 1011    - set interrupt
  {0xFF, "hlt",   0xF4, {0},                                DISPL_FMT_NONE,   INSTR_FMT_NO_DATA}, // 1111 0100    - halt (wait for ext int)
  {0xFF, "wait",  0x9B, {0},                                DISPL_FMT_NONE,   INSTR_FMT_NO_DATA}, // 1001 1011    - wait
};

// Operands
enum operand_type {OP_NONE, OP_REG, OP_EA, OP_DATA, OP_IP_INC};

struct operand {
  union {
    const char* reg;
    struct ea   ea;
    u16         data;
    i16         ip_inc;
  };
  enum operand_type type;
};

enum prefix_type {
  PREFIX_NONE   = 0,
  PREFIX_LOCK   = 1 << 0,
  PREFIX_REP    = 1 << 1,
  PREFIX_REPNE  = 1 << 2,
  PREFIX_SEG    = 1 << 3,
};

struct prefixes {
  const char *seg;
  enum prefix_type types;
};

struct instr {
  const char *str;
  struct operand ops[2];
  struct prefixes prefixes;
  bool set_cs_addr;
  u16 cs_addr;          // set cs to this new value
  bool w;
};

const struct instr_table_row *s_b0_to_row[256] = {0};

// build index table for the first byte (b0)
void build_index() {
  u8 b0 = 0;
  do {
    const struct instr_table_row *found_row = NULL;

    for (u64 i = 0; i < ARRAY_COUNT(s_instr_rows); ++i) {
      const struct instr_table_row *row = &s_instr_rows[i];
      if (row->opcode == (b0 & row->mask)) {
        found_row = row;
        break;
      }
    }
    s_b0_to_row[b0] = found_row;
  } while (++b0 != 0);
}

FORCE_INLINE enum prefix_type decode_prefix(u8 b, struct prefixes *out) {
  enum prefix_type type = PREFIX_NONE;

  if ((b & 0xE7) == 0x26) {
    type = PREFIX_SEG;
    out->seg = str_seg_reg((b & 0x18) >> 3);
  } else {
    switch (b) {
      case 0xF0: type = PREFIX_LOCK; break;
      case 0xF2: type = PREFIX_REPNE; break;
      case 0xF3: type = PREFIX_REP; break;
    }
  }
  out->types |= type;
  return type;
}

// decode error check helper
#define CHECK(op, err_msg)        \
    if (!(op)) {                  \
      fprintf(stderr, (err_msg)); \
      return ret;                 \
    }

// Decode
// b0 = next byte from the byte stream
// is b0 prefix?
//   store prefix, b0 = read next byte
//
// find instruction row matching first byte b0
// for each row in instruction tables
//   mask b0 with row.mask and compare to row opcode
//
// depending on instruction row's mask decode d, w, reg flags from b0
//
// if instruction row has displacement
//   read displacement byte1 from the byte stream
//   depending on mod rm fields read subsequent displacement bytes
//   if instructino has no data
//     fill in reg flag
//
// if instruction row has data
//   read data from the byte stream according to row's data format
//
// if d flag is set
//   swap src and dst operands
//
// instruction is ready
struct instr decode_next_instr(struct stream *stream) {
  // FLAG_NONE = 0, FLAG_D = 1, FLAG_W = 2, FLAG_S = 3, FLAG_V = 4,
  int flags[5] = {-1 ,-1, -1, -1, -1};

  struct instr ret = {0};
  u8 b0;

  // decode prefixies
  enum prefix_type prefix_type;
  do {
    CHECK(stream_read_b(stream, &b0), "Error: no instruction or prefix");
    prefix_type = decode_prefix(b0, &ret.prefixes);
  } while (prefix_type != PREFIX_NONE);

  // decode instruction byte 0
  const struct instr_table_row *row = s_b0_to_row[b0];
  if (!row) {
    return ret;
  }

  // decode first byte flags
  flags[FLAG_W] = 1;
  u8 ops_size = 0;

  for (u64 i = 0; i < ARRAY_COUNT(row->flags); ++i) {
    struct instr_flag flag = row->flags[i];
    if (flag.type == FLAG_NONE) {
      break;
    }

    u8 f = flag.is_implicit
      ? flag.value
      : (b0 & (0xFF >> (8 - flag.size_bits)) << flag.pos) >> flag.pos;

    if (flag.type == FLAG_REG) {
      bool is_wide = flags[FLAG_W] || (flag.is_implicit && flag.is_always_wide);
      ret.ops[ops_size++] = (struct operand){
        .reg = is_wide ? str_reg_word(f) : str_reg_byte(f),
        .type = OP_REG
      };
    } else if (flag.type == FLAG_SEG_REG) {
      ret.ops[ops_size++] = (struct operand){
        .reg = str_seg_reg(f),
        .type = OP_REG
      };
    } else {
      flags[flag.type] = f;
    }
  }

  // byte0 flags
  u8 w = flags[FLAG_W] > 0;
  u8 d = flags[FLAG_D] > 0; // d - instruction has displacement w/o data
  u8 s = flags[FLAG_S] > 0; // s - instruction has displacement with immediate
  u8 v = flags[FLAG_V] > 0; // v = 1, rotate count in CL register

  ret.str = row->str;
  ret.w = w;

  u8 opcode_aux; // part of opcode in Byte 1

  // read displacement
  if (row->displ_fmt != DISPL_FMT_NONE) {
    u8 b1;
    CHECK(stream_read_b(stream, &b1), "Error: failed to read displacement\n");

    u8 mod = (b1 & 0xC0) >> 6;
    u8 reg = (b1 & 0x38) >> 3;
    u8 sr  = (b1 & 0x18) >> 3;
    u8 rm  = b1 & 0x07;

    struct operand reg_op = {
        .reg = w ? str_reg_word(reg) : str_reg_byte(reg),
        .type = OP_REG
      };
    struct operand rm_op = {
        .reg = w ? str_reg_word(rm) : str_reg_byte(rm),
        .type = OP_REG
      };

    if (row->displ_fmt == DISPL_FMT_REG_RM) {
      // treat reg as one of the operands if no data follows
      if (mod == 3) {
        ret.ops[ops_size++] = rm_op;
      } else {
        struct ea ea = {.seg = ret.prefixes.seg};
        CHECK(decode_effective_address(stream, rm, mod, &ea),
            "Error: mov no displacement\n");
        ret.ops[ops_size++] = (struct operand){.ea = ea, .type = OP_EA};
      }
      ret.ops[ops_size++] = reg_op;
    } else {
      // treat reg as auxiliary part of the opcode
      opcode_aux = reg;
      if (mod == 3) {
          ret.ops[ops_size++] = row->displ_fmt == DISPL_FMT_RM
            ? rm_op
            : (struct operand){.reg = str_seg_reg(sr), .type = OP_REG};
      } else {
        struct ea ea = {.seg = ret.prefixes.seg};
        CHECK(decode_effective_address(stream, rm, mod, &ea),
            "Error: mov no displacement\n");
        ret.ops[ops_size++] = (struct operand){.ea = ea, .type = OP_EA};
      }
    }
  }

  enum instr_fmt instr_fmt = row->fmt;

  // handle the rest of the opcode in Byte1
  if (ret.str == s_alu0) {
    ret.str = str_opcode_alu0(opcode_aux);
  } else if (ret.str == s_alu1) {
    ret.str = str_opcode_alu1(opcode_aux);
    // among all the instructions with the same prefix only test instruction
    // has datas; insane...
    if (ret.str == s_test) {
      instr_fmt = INSTR_FMT_DATAW;
    }
  } else if (ret.str == s_alu2) {
    ret.str = str_opcode_alu2(opcode_aux);
  } else if (ret.str == s_shft) {
    ret.str = str_opcode_shft(opcode_aux);
  }

  if (flags[FLAG_V] >= 0) {
    ret.ops[ops_size++] = v
      ? (struct operand){.reg = "cl", .type = OP_REG}
      : (struct operand){.data = 1, .type = OP_DATA};
  }

  // read data
  bool should_read_data = true;
  bool is_sign_extend = false;
  bool is_data_w = w;
  switch (instr_fmt) {
    case INSTR_FMT_ADDR:
    case INSTR_FMT_DATAW:
    case INSTR_FMT_SEG_ADDR:
      break;
    case INSTR_FMT_DATASW:
      is_sign_extend = s && w;
      is_data_w = is_sign_extend ? false : w;
      break;
    case INSTR_FMT_DATA8:
      is_data_w = false;
      break;
    case INSTR_FMT_DATA16:
      is_data_w = true;
      break;
    case INSTR_FMT_IP_INC8:
      is_sign_extend = true;
      is_data_w = false;
      break;
    case INSTR_FMT_IP_INC16:
      is_data_w = true;
      break;
    case INSTR_FMT_SWALLOW8:
      is_data_w = false;
      break;
    default:
      should_read_data = false;
      break;
  }

  if (should_read_data) {
    u16 data = 0;
    if (is_sign_extend) {
      CHECK(stream_read_sign(stream, (i16 *)&data, is_data_w),
          "Error: failed to read data\n");
    } else {
      CHECK(stream_read(stream, &data, is_data_w),
          "Error: failed to read data\n");
    }
    if (instr_fmt == INSTR_FMT_SWALLOW8) {
      // byte was read, do nothing
    } else if (instr_fmt == INSTR_FMT_ADDR || instr_fmt == INSTR_FMT_SEG_ADDR) {
      struct ea ea = {
        .type = EA_DIRECT,
        .seg = ret.prefixes.seg,
        .direct_addr = data};
      ret.ops[ops_size++] = (struct operand){.ea = ea, .type = OP_EA};
    } else if (instr_fmt == INSTR_FMT_IP_INC8
        || instr_fmt == INSTR_FMT_IP_INC16) {
      ret.ops[ops_size++] = (struct operand){
        .ip_inc = data,
        .type = OP_IP_INC
      };
    } else {
      ret.ops[ops_size++] = (struct operand){.data = data, .type = OP_DATA};
    }
    if (instr_fmt == INSTR_FMT_SEG_ADDR) {
      ret.set_cs_addr = true;
      CHECK(stream_read(stream, &ret.cs_addr, true),
          "Error: failed to read CS segment address\n");
    }
  }

  assert(ops_size < 3 && "More than 2 arguments to the operation");
  assert(!d || ops_size > 1 && "d flag is set but not enough operands");

  if (d && ops_size > 1) {
    SWAP(ret.ops[0], ret.ops[1]);
  }

  return ret;
}

void print_ea(const struct ea *ea, bool set_cs_addr) {
  if (set_cs_addr) {
    printf("%u", ea->direct_addr);
    return;
  }

  if (ea->seg) {
    printf("%s:", ea->seg);
  }
  if (ea->type == EA_DIRECT) {
    printf("[%u]", ea->direct_addr);
    return;
  }

  printf("[");
  if (ea->compute_addr.base_reg) {
    printf("%s", ea->compute_addr.base_reg);
  }
  if (ea->compute_addr.index_reg) {
    printf(" + %s", ea->compute_addr.index_reg);
  }
  if (ea->type == EA_COMPUTE && ea->compute_addr.displ != 0) {
    // print displacement with space after arithmetic operation character
    const char *op = ea->compute_addr.displ < 0 ? "-" : "+";
    u16 udispl = ea->compute_addr.displ < 0
      ? ~ea->compute_addr.displ + 1
      : ea->compute_addr.displ;
    printf(" %s %u", op, udispl);
  }
  printf("]");
}

void print_operand(const struct operand *op, bool set_cs_addr) {
  switch (op->type) {
    case OP_NONE:
      break;
    case OP_REG:
      printf("%s", op->reg);
      break;
    case OP_EA:
      print_ea(&op->ea, set_cs_addr);
      break;
    case OP_DATA:
      printf("%u", op->data);
      break;
    case OP_IP_INC: {
      printf("$%+d", op->ip_inc + 2);
    } break;
  }
}

void print_prefixes(const struct prefixes *prefixes) {
  if (prefixes->types & PREFIX_LOCK) {
    printf("lock ");
  }
  if (prefixes->types & PREFIX_REP) {
    printf("rep ");
  }
  if (prefixes->types & PREFIX_REPNE) {
    printf("repne ");
  }
}

void print_instr(const struct instr *instr) {
  const struct operand *op0 = &instr->ops[0];
  const struct operand *op1 = &instr->ops[1];

  if (instr->prefixes.types != PREFIX_NONE) {
    print_prefixes(&instr->prefixes);
  }

  printf("%s", instr->str);
  if (op0->type != OP_NONE) {
    printf(" ");
    if (instr->set_cs_addr) {
      printf("%u: ", instr->cs_addr);
    }
    // (byte|word) [memory]
    // No needed to print width prefix it if register was specified as
    // op1 explicitly.
    // For now don't distinguish between explicit and implicit registers
    // and always print width prefix
    if (op0->type == OP_EA) {
      printf("%s ", instr->w ? "word" : "byte");
    }
    print_operand(op0, instr->set_cs_addr);
  }
  if (op1->type != OP_NONE) {
    printf(", ");
    print_operand(op1, instr->set_cs_addr);
  }
  printf("\n");
}

int decode(struct stream *s) {
  build_index();

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
