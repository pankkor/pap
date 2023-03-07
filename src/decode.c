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

struct instr_table_row {
  const char *str;
  u8 opcode;              // masked first instr byte instruction should match opcode
  u8 with_acc;
  u8 d;                   // exchange operands
  enum displ_fmt displ_fmt;
  enum instr_fmt fmt;
};

// Instruction tables

// Some instructions have the same opcode in Byte0, and the rest of opcode is
// encoded in the following Byte1.
const char s_alu0[]  = "<alu0>"; // add, sub, etc
const char s_alu1[]  = "<alu1>"; // mul, div, etc
const char s_alu2[]  = "<alu2>"; // inc, dec, call, jmp
const char s_shft[]  = "<shft>"; // shifts and rotations

struct instr_table_row s_E7_instrs[] = {
  {"push",  0x06, 0, 0, DISPL_FMT_NONE,   INSTR_FMT_NO_DATA}, // 000s r110    - push SEG
  {"pop",   0x07, 0, 0, DISPL_FMT_NONE,   INSTR_FMT_NO_DATA}  // 000s r111    - pop SEG
};

struct instr_table_row s_F0_instrs[] = {
  {"mov",   0xB0, 0, 0, DISPL_FMT_NONE,   INSTR_FMT_DATAW},   // 1011 wreg    - mov IMM to REG
};

struct instr_table_row s_F8_instrs[] = {
  {"push",  0x50, 0, 0, DISPL_FMT_NONE,   INSTR_FMT_NO_DATA}, // 0101 0reg    - push REG
  {"pop",   0x58, 0, 0, DISPL_FMT_NONE,   INSTR_FMT_NO_DATA}, // 0101 1reg    - pop REG
  {"inc",   0x40, 0, 0, DISPL_FMT_NONE,   INSTR_FMT_NO_DATA}, // 0100 0reg    - inc REG
  {"dec",   0x48, 0, 0, DISPL_FMT_NONE,   INSTR_FMT_NO_DATA}, // 0100 1reg    - dec REG
  {"xchg",  0x90, 1, 0, DISPL_FMT_NONE,   INSTR_FMT_NO_DATA}, // 1001 0reg    - xchg ACC & REG
  // TODO esc is not handled
  // {"esc",   0xD8, 0, 0, DISPL_FMT_REG_RM,   INSTR_FMT_NO_DATA}, // 1101 1xxx    - escape to ext device
};

struct instr_table_row s_FC_instrs[] = {
  {"mov",   0x88, 0, 0, DISPL_FMT_REG_RM, INSTR_FMT_NO_DATA}, // 1000 10dw    - mov R/M to/f REG
  {"mov",   0xA0, 1, 0, DISPL_FMT_NONE,   INSTR_FMT_ADDR},    // 1010 00dw    - mov MEM to/f ACC
  {"add",   0x00, 0, 0, DISPL_FMT_REG_RM, INSTR_FMT_NO_DATA}, // 0000 00dw    - add R/M + REG
  {"or",    0x08, 0, 0, DISPL_FMT_REG_RM, INSTR_FMT_NO_DATA}, // 0000 10dw    - or  R/M & REG
  {"adc",   0x10, 0, 0, DISPL_FMT_REG_RM, INSTR_FMT_NO_DATA}, // 0001 00dw    - adc R/M + REG
  {"and",   0x20, 0, 0, DISPL_FMT_REG_RM, INSTR_FMT_NO_DATA}, // 0010 00dw    - and R/M & REG
  {"sub",   0x28, 0, 0, DISPL_FMT_REG_RM, INSTR_FMT_NO_DATA}, // 0010 10dw    - sub R/M + REG
  {"sbb",   0x18, 0, 0, DISPL_FMT_REG_RM, INSTR_FMT_NO_DATA}, // 0001 10dw    - sbb R/M + REG
  {"xor",   0x30, 0, 0, DISPL_FMT_REG_RM, INSTR_FMT_NO_DATA}, // 0011 00dw    - xor R/M ^ REG
  {"cmp",   0x38, 0, 0, DISPL_FMT_REG_RM, INSTR_FMT_NO_DATA}, // 0011 10dw    - cmp R/M + REG
  {s_alu0,  0x80, 0, 0, DISPL_FMT_RM,     INSTR_FMT_DATASW},  // 1000 00sw    - (add|sub|..) R/M IMM
  {s_shft,  0xD0, 0, 0, DISPL_FMT_RM,     INSTR_FMT_NO_DATA}, // 1101 00vw    - (shl|shr|..) R/M
};

struct instr_table_row s_FE_instrs[] = {
  {"mov",   0xC6, 0, 0, DISPL_FMT_RM,     INSTR_FMT_DATAW},   // 1100 011w    - mov IMM to R/M
  {"xchg",  0x86, 0, 1, DISPL_FMT_REG_RM, INSTR_FMT_NO_DATA}, // 1000 011w    - xchg R/M w REG
  {"add",   0x04, 1, 0, DISPL_FMT_NONE,   INSTR_FMT_DATAW},   // 0000 010w    - add ACC + IMM
  {"or",    0x0C, 1, 0, DISPL_FMT_NONE,   INSTR_FMT_DATAW},   // 0000 110w    - or  ACC | IMM
  {"adc",   0x14, 1, 0, DISPL_FMT_NONE,   INSTR_FMT_DATAW},   // 1000 000w    - adc ACC + IMM
  {"sub",   0x2C, 1, 0, DISPL_FMT_NONE,   INSTR_FMT_DATAW},   // 0010 110w    - sub ACC - IMM
  {"and",   0x24, 1, 0, DISPL_FMT_NONE,   INSTR_FMT_DATAW},   // 0010 010w    - and ACC & IMM
  {"sbb",   0x1C, 1, 0, DISPL_FMT_NONE,   INSTR_FMT_DATAW},   // 0001 110w    - sbb ACC - IMM
  {s_alu2,  0xFE, 0, 0, DISPL_FMT_RM,     INSTR_FMT_NO_DATA}, // 1111 111w    - (inc|dec|..) R/M
  {"xor",   0x34, 1, 0, DISPL_FMT_NONE,   INSTR_FMT_DATAW},   // 0011 010w    - xor ACC ^ IMM
  {"cmp",   0x3C, 1, 0, DISPL_FMT_NONE,   INSTR_FMT_DATAW},   // 0011 110w    - cmp ACC & IMM
  {"in",    0xE4, 1, 0, DISPL_FMT_NONE,   INSTR_FMT_DATA8},   // 1110 010w    - in  ACC IMM8
  {"out",   0xE6, 1, 1, DISPL_FMT_NONE,   INSTR_FMT_DATA8},   // 1110 011w    - out ACC IMM8
  {s_alu1,  0xF6, 0, 0, DISPL_FMT_RM,     INSTR_FMT_NO_DATA}, // 1111 011w    - (mul|div|..) R/M
  {"test",  0x84, 0, 0, DISPL_FMT_REG_RM, INSTR_FMT_NO_DATA}, // 1000 010w    - test R/M & REG
  {"test",  0xA8, 1, 0, DISPL_FMT_NONE,   INSTR_FMT_DATAW},   // 1010 100w    - test ACC & IMM
};

// instructions with implicit dx register
struct instr_table_row s_FE_io_instrs[] = {
  {"in",    0xEC, 1, 1, DISPL_FMT_NONE,   INSTR_FMT_NO_DATA}, // 1110 110w    - in  ACC dx
  {"out",   0xEE, 1, 0, DISPL_FMT_NONE,   INSTR_FMT_NO_DATA}, // 1110 111w    - out ACC dx
};

struct instr_table_row s_FF_instrs[] = {
  {"mov",   0x8C, 0, 0, DISPL_FMT_SR_RM,  INSTR_FMT_NO_DATA}, // 1000 1100    - mov SEG to R/M
  {"mov",   0x8E, 0, 0, DISPL_FMT_SR_RM,  INSTR_FMT_NO_DATA}, // 1000 1110    - mov R/M to SEG
  {"pop",   0x8F, 0, 0, DISPL_FMT_RM,     INSTR_FMT_NO_DATA}, // 1000 1111    - pop R/M
  {s_alu2,  0xFF, 0, 0, DISPL_FMT_RM,     INSTR_FMT_NO_DATA}, // 1111 1111    - push R/M
  {"je",    0x74, 0, 0, DISPL_FMT_NONE,   INSTR_FMT_IP_INC8}, // 0111 0100    - je /jz   IP-INC8
  {"jl",    0x7C, 0, 0, DISPL_FMT_NONE,   INSTR_FMT_IP_INC8}, // 0111 1100    - jl /jnge IP-INC8
  {"jle",   0x7E, 0, 0, DISPL_FMT_NONE,   INSTR_FMT_IP_INC8}, // 0111 1110    - jle/jng  IP-INC8
  {"jb",    0x72, 0, 0, DISPL_FMT_NONE,   INSTR_FMT_IP_INC8}, // 0111 0010    - jb /jnae IP-INC8
  {"jbe",   0x76, 0, 0, DISPL_FMT_NONE,   INSTR_FMT_IP_INC8}, // 0111 0110    - jbe/jna  IP-INC8
  {"jp",    0x7A, 0, 0, DISPL_FMT_NONE,   INSTR_FMT_IP_INC8}, // 0111 1010    - jp /jpe  IP-INC8
  {"jo",    0x70, 0, 0, DISPL_FMT_NONE,   INSTR_FMT_IP_INC8}, // 0111 0000    - jo       IP-INC8
  {"js",    0x78, 0, 0, DISPL_FMT_NONE,   INSTR_FMT_IP_INC8}, // 0111 1000    - js       IP-INC8
  {"jne",   0x75, 0, 0, DISPL_FMT_NONE,   INSTR_FMT_IP_INC8}, // 0111 0101    - jne/jnz  IP-INC8
  {"jnl",   0x7D, 0, 0, DISPL_FMT_NONE,   INSTR_FMT_IP_INC8}, // 0111 1101    - jnl/jge  IP-INC8
  {"jg",    0x7F, 0, 0, DISPL_FMT_NONE,   INSTR_FMT_IP_INC8}, // 0111 1111    - jg /jnle IP-INC8
  {"jnb",   0x73, 0, 0, DISPL_FMT_NONE,   INSTR_FMT_IP_INC8}, // 0111 0011    - jnb/jae  IP-INC8
  {"jnbe",  0x77, 0, 0, DISPL_FMT_NONE,   INSTR_FMT_IP_INC8}, // 0111 0111    - jnb/jae  IP-INC8
  {"jnp",   0x7B, 0, 0, DISPL_FMT_NONE,   INSTR_FMT_IP_INC8}, // 0111 1011    - jnp/jpo  IP-INC8
  {"jno",   0x71, 0, 0, DISPL_FMT_NONE,   INSTR_FMT_IP_INC8}, // 0111 0001    - jno      IP-INC8
  {"jns",   0x79, 0, 0, DISPL_FMT_NONE,   INSTR_FMT_IP_INC8}, // 0111 1001    - jno      IP-INC8
  {"jcxz",  0xE3, 0, 0, DISPL_FMT_NONE,   INSTR_FMT_IP_INC8}, // 1110 0011    - jsxz     IP-INC8
  {"loop",  0xE2, 0, 0, DISPL_FMT_NONE,   INSTR_FMT_IP_INC8}, // 1110 0010    - loop CX times
  {"loopz", 0xE1, 0, 0, DISPL_FMT_NONE,   INSTR_FMT_IP_INC8}, // 1110 0001    - loopz/loope
  {"loopnz",0xE0, 0, 0, DISPL_FMT_NONE,   INSTR_FMT_IP_INC8}, // 1110 0000    - loopnz/loopne
  {"jmp",   0xEB, 0, 0, DISPL_FMT_NONE,   INSTR_FMT_IP_INC8}, // 1110 1001    - jmp in SEG short
  {"call",  0xE8, 0, 0, DISPL_FMT_NONE,   INSTR_FMT_IP_INC16},// 1110 1000    - call direct in SEG
  {"jmp",   0xE9, 0, 0, DISPL_FMT_NONE,   INSTR_FMT_IP_INC16},// 1110 1001    - jmp direct in SEG
  {"ret",   0xC2, 0, 0, DISPL_FMT_NONE,   INSTR_FMT_DATA16},  // 1100 0010    - ret SP + IMM16 in SEG
  {"ret",   0xCA, 0, 0, DISPL_FMT_NONE,   INSTR_FMT_DATA16},  // 1100 1010    - ret SP + IMM16 inter SEG
  {"ret",   0xC3, 0, 0, DISPL_FMT_NONE,   INSTR_FMT_NO_DATA}, // 1100 0011    - ret in SEG
  {"ret",   0xCB, 0, 0, DISPL_FMT_NONE,   INSTR_FMT_NO_DATA}, // 1100 1011    - ret inter SEG
  {"xlat",  0xD7, 0, 0, DISPL_FMT_NONE,   INSTR_FMT_NO_DATA}, // 1101 0111    - byte to AL
  {"lea",   0x8D, 0, 1, DISPL_FMT_REG_RM, INSTR_FMT_NO_DATA}, // 1000 1101    - lea R/M to REG
  {"lds",   0xC5, 0, 1, DISPL_FMT_REG_RM, INSTR_FMT_NO_DATA}, // 1100 0101    - load ptr to DS
  {"les",   0xC4, 0, 1, DISPL_FMT_REG_RM, INSTR_FMT_NO_DATA}, // 1100 0100    - load ptr to ES
  {"lahf",  0x9F, 0, 0, DISPL_FMT_NONE,   INSTR_FMT_NO_DATA}, // 1001 1111    - load FLAGS to AH
  {"sahf",  0x9E, 0, 0, DISPL_FMT_NONE,   INSTR_FMT_NO_DATA}, // 1001 1110    - store AH to FLAGS
  {"call",  0x9A, 0, 0, DISPL_FMT_NONE,   INSTR_FMT_SEG_ADDR},// 1001 1010    - call direct intra SEG
  {"jmp",   0xEA, 0, 0, DISPL_FMT_NONE,   INSTR_FMT_SEG_ADDR},// 1110 1010    - jmp direct intra SEG
  {"pushf", 0x9C, 0, 0, DISPL_FMT_NONE,   INSTR_FMT_NO_DATA}, // 1001 1110    - push FLAGS
  {"popf",  0x9D, 0, 0, DISPL_FMT_NONE,   INSTR_FMT_NO_DATA}, // 1001 1110    - pop FLAGS
  {"aaa",   0x37, 0, 0, DISPL_FMT_NONE,   INSTR_FMT_NO_DATA}, // 0011 0111    - ASCII adjust for add
  {"daa",   0x27, 0, 0, DISPL_FMT_NONE,   INSTR_FMT_NO_DATA}, // 0010 0111    - Dec adjust for add
  {"aas",   0x3F, 0, 0, DISPL_FMT_NONE,   INSTR_FMT_NO_DATA}, // 0011 1111    - ASCII adjust for sub
  {"das",   0x2F, 0, 0, DISPL_FMT_NONE,   INSTR_FMT_NO_DATA}, // 0010 1111    - Dec adjust for sub
  {"aam",   0xD4, 0, 0, DISPL_FMT_NONE,   INSTR_FMT_SWALLOW8},// 1101 0100    - ASCII adjust for mul
  {"aad",   0xD5, 0, 0, DISPL_FMT_NONE,   INSTR_FMT_SWALLOW8},// 1101 0101    - ASCII adjust for div
  {"cbw",   0x98, 0, 0, DISPL_FMT_NONE,   INSTR_FMT_NO_DATA}, // 1001 0100    - byte -> word
  {"cwd",   0x99, 0, 0, DISPL_FMT_NONE,   INSTR_FMT_NO_DATA}, // 1001 0101    - word -> dword
  {"movsb", 0xA4, 0, 0, DISPL_FMT_NONE,   INSTR_FMT_NO_DATA}, // 1010 0100    - movs byte SI,DI
  {"movsw", 0xA5, 0, 0, DISPL_FMT_NONE,   INSTR_FMT_NO_DATA}, // 1010 0101    - movs word SI,DI
  {"cmpsb", 0xA6, 0, 0, DISPL_FMT_NONE,   INSTR_FMT_NO_DATA}, // 1010 0100    - cmps byte SI,DI
  {"cmpsw", 0xA7, 0, 0, DISPL_FMT_NONE,   INSTR_FMT_NO_DATA}, // 1010 0101    - cmps word SI,DI
  {"scasb", 0xAE, 0, 0, DISPL_FMT_NONE,   INSTR_FMT_NO_DATA}, // 1010 1110    - scas byte SI,DI
  {"scasw", 0xAF, 0, 0, DISPL_FMT_NONE,   INSTR_FMT_NO_DATA}, // 1010 1111    - scas word SI,DI
  {"lodsb", 0xAC, 0, 0, DISPL_FMT_NONE,   INSTR_FMT_NO_DATA}, // 1010 1100    - lods byte SI,DI
  {"lodsw", 0xAD, 0, 0, DISPL_FMT_NONE,   INSTR_FMT_NO_DATA}, // 1010 1101    - lods word SI,DI
  {"stosb", 0xAA, 0, 0, DISPL_FMT_NONE,   INSTR_FMT_NO_DATA}, // 1010 1010    - stos byte SI,DI
  {"stosw", 0xAB, 0, 0, DISPL_FMT_NONE,   INSTR_FMT_NO_DATA}, // 1010 1011    - stos word SI,DI
  {"int3",  0xCC, 0, 0, DISPL_FMT_NONE,   INSTR_FMT_NO_DATA}, // 1100 1100    - int3
  {"int",   0xCD, 0, 0, DISPL_FMT_NONE,   INSTR_FMT_DATA8},   // 1100 1101    - int IMM8
  {"into",  0xCE, 0, 0, DISPL_FMT_NONE,   INSTR_FMT_NO_DATA}, // 1100 1110    - int on overflow
  {"iret",  0xCF, 0, 0, DISPL_FMT_NONE,   INSTR_FMT_NO_DATA}, // 1100 1111    - return from int
  {"cmc",   0xF5, 0, 0, DISPL_FMT_NONE,   INSTR_FMT_NO_DATA}, // 1111 0101    - complement carry
  {"clc",   0xF8, 0, 0, DISPL_FMT_NONE,   INSTR_FMT_NO_DATA}, // 1111 1000    - clear carry
  {"stc",   0xF9, 0, 0, DISPL_FMT_NONE,   INSTR_FMT_NO_DATA}, // 1111 0101    - set carry
  {"cld",   0xFC, 0, 0, DISPL_FMT_NONE,   INSTR_FMT_NO_DATA}, // 1111 1100    - clear direction
  {"std",   0xFD, 0, 0, DISPL_FMT_NONE,   INSTR_FMT_NO_DATA}, // 1111 1101    - set direction
  {"cli",   0xFA, 0, 0, DISPL_FMT_NONE,   INSTR_FMT_NO_DATA}, // 1111 1010    - clear interrupt
  {"sti",   0xFB, 0, 0, DISPL_FMT_NONE,   INSTR_FMT_NO_DATA}, // 1111 1011    - set interrupt
  {"hlt",   0xF4, 0, 0, DISPL_FMT_NONE,   INSTR_FMT_NO_DATA}, // 1111 0100    - halt (wait for ext int)
  {"wait",  0x9B, 0, 0, DISPL_FMT_NONE,   INSTR_FMT_NO_DATA}, // 1001 1011    - wait
};

// Instruction mask determines location of flags (d, w, etc) in the first byte
enum instr_mask {
  INSTR_MASK_E7,    // 111X X111
  INSTR_MASK_F0,    // 1111 XXXX
  INSTR_MASK_F8,    // 1111 1XXX
  INSTR_MASK_FC,    // 1111 11XX
  INSTR_MASK_FE,    // 1111 111X
  INSTR_MASK_FE_IO, // 1111 111X - same mask but different logic
  INSTR_MASK_FF,    // 1111 1111
  INSTR_MASK_COUNT
};

u8 s_instr_masks[] = {0xE7, 0xF0, 0xF8, 0xFC, 0xFE, 0xFE, 0xFF};

struct instrs_table {
  struct instr_table_row *rows;
  u64 size;
};

struct instrs_table s_instr_tables[] = {
  {s_E7_instrs,     ARRAY_COUNT(s_E7_instrs)},
  {s_F0_instrs,     ARRAY_COUNT(s_F0_instrs)},
  {s_F8_instrs,     ARRAY_COUNT(s_F8_instrs)},
  {s_FC_instrs,     ARRAY_COUNT(s_FC_instrs)},
  {s_FE_instrs,     ARRAY_COUNT(s_FE_instrs)},
  {s_FE_io_instrs,  ARRAY_COUNT(s_FE_io_instrs)},
  {s_FF_instrs,     ARRAY_COUNT(s_FF_instrs)},
};

_Static_assert(ARRAY_COUNT(s_instr_masks) == INSTR_MASK_COUNT, "Bad masks");
_Static_assert(ARRAY_COUNT(s_instr_tables) == INSTR_MASK_COUNT, "Bad tables");

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
  struct instr ret = {0};
  u8 b0;

  // decode prefixies
  enum prefix_type prefix_type;
  do {
    CHECK(stream_read_b(stream, &b0), "Error: no instruction or prefix");
    prefix_type = decode_prefix(b0, &ret.prefixes);
  } while (prefix_type != PREFIX_NONE);

  // decode instruction byte 0
  const struct instr_table_row *row = NULL;
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
  ret.str = row->str;

  // byte0 flags
  u8 w = 1;
  // s & d share the same position in byte0
  u8 d = row->d;  // d - instruction has displacement w/o data
  u8 s = 0;       // s - instruction has displacement with immediate
  u8 v = 0;       // v = 1, rotate count in CL register

  u8 ops_size = 0;

  switch (instr_mask_type) {
    // XXXs rXXX  - sr 2-bit segment register
    case INSTR_MASK_E7:
      ret.ops[ops_size++] = (struct operand){
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
      ret.ops[ops_size++] = (struct operand){
        .reg = w ? str_reg_word(reg) : str_reg_byte(reg),
        .type = OP_REG
      };
    } break;

    // XXXX XXdw
    case INSTR_MASK_FC:
      w = b0 & 0x01;
      if (row->fmt == INSTR_FMT_DATASW) {
        s = (b0 & 0x02) >> 1;
      }
      if (row->displ_fmt == DISPL_FMT_REG_RM || row->fmt == INSTR_FMT_ADDR) {
        d = (b0 & 0x02) >> 1;
      }
      v = (b0 & 0x02) >> 1;
      break;

    // XXXX XXXw - io instructions with implicit dx
    case INSTR_MASK_FE_IO:
      ret.ops[ops_size++] = (struct operand){.reg = "dx", .type = OP_REG};
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

  ret.w = w;

  if (row->with_acc) {
    ret.ops[ops_size++] = (struct operand){
      .reg = w ? "ax" : "al",
      .type = OP_REG
    };
  }

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
    ret.ops[ops_size++] = v ? (struct operand){.reg = "cl", .type = OP_REG}
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
