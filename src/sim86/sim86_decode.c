#include "sim86_decode.h"
#include "sim86_stream.h"

#include <assert.h>     // assert
#include <stddef.h>     // NULL
#include <stdio.h>      // fprintf, stderr

// Generic 8086 instruction encoding: 1-6 bytes
//
// |    Byte 0     | |    Byte 1     |
// |7 6 5 4 3 2|1 0| |7 6|5 4 3|2 1 0| ...
// | OPCODE    |D|W| |MOD| REG | R/M |
//
// Byte 2-5 encode optionasl displacement or immediate.
// Special case instructions might miss some flags and have them at different
// positions.

static const struct reg s_map_regs[][2] = {
  {{REG_A,   REG_MODE_L}, {REG_A,   REG_MODE_X}},
  {{REG_C,   REG_MODE_L}, {REG_C,   REG_MODE_X}},
  {{REG_D,   REG_MODE_L}, {REG_D,   REG_MODE_X}},
  {{REG_B,   REG_MODE_L}, {REG_B,   REG_MODE_X}},
  {{REG_A,   REG_MODE_H}, {REG_SP,  REG_MODE_X}},
  {{REG_C,   REG_MODE_H}, {REG_BP,  REG_MODE_X}},
  {{REG_D,   REG_MODE_H}, {REG_SI,  REG_MODE_X}},
  {{REG_B,   REG_MODE_H}, {REG_DI,  REG_MODE_X}},
};

static const struct reg s_map_seg_regs[] = {
  {REG_ES,  REG_MODE_X},
  {REG_CS,  REG_MODE_X},
  {REG_SS,  REG_MODE_X},
  {REG_DS,  REG_MODE_X},
};

static FORCE_INLINE struct reg reg_from_bits(u8 bits, bool is_wide) {
  return s_map_regs[bits & 0x7][is_wide ? 1 : 0];
}

static FORCE_INLINE struct reg seg_reg_from_bits(u8 bits) {
  return s_map_seg_regs[bits & 0x3];
}

static bool decode_effective_address(struct stream *s, u8 rm, u8 mod,
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
      case 0: out->compute_addr = (struct compute_addr){reg_ptr(REG_B,  REG_MODE_X), reg_ptr(REG_SI, REG_MODE_X), 0}; break;
      case 1: out->compute_addr = (struct compute_addr){reg_ptr(REG_B,  REG_MODE_X), reg_ptr(REG_DI, REG_MODE_X), 0}; break;
      case 2: out->compute_addr = (struct compute_addr){reg_ptr(REG_BP, REG_MODE_X), reg_ptr(REG_SI, REG_MODE_X), 0}; break;
      case 3: out->compute_addr = (struct compute_addr){reg_ptr(REG_BP, REG_MODE_X), reg_ptr(REG_DI, REG_MODE_X), 0}; break;
      case 4: out->compute_addr = (struct compute_addr){NULL,                        reg_ptr(REG_SI, REG_MODE_X), 0}; break;
      case 5: out->compute_addr = (struct compute_addr){NULL,                        reg_ptr(REG_DI, REG_MODE_X), 0}; break;
      case 6: out->compute_addr = (struct compute_addr){reg_ptr(REG_BP, REG_MODE_X), NULL,                        0}; break;
      case 7: out->compute_addr = (struct compute_addr){reg_ptr(REG_B,  REG_MODE_X), NULL,                        0}; break;
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
  const char *str;
  enum instr_type type;
  u8 mask;
  u8 opcode;                    // masked first instr byte instruction
                                // should match opcode
  i8 b1_rows_idx;               // index into s_inst_b1_rows table for
                                // instructions with opcode in byte1
  struct instr_flag flags[3];
  enum displ_fmt displ_fmt;
  enum instr_fmt fmt;
};

struct instr_table_b1_row {
  const char *str;
  enum instr_type type;
  enum instr_fmt fmt;
  bool is_far;
};

// Some instructions have the same opcode in Byte0, and the rest of opcode is
// encoded in the following Byte1.
static const struct instr_table_b1_row s_instr_b1_rows[4][8] = {
  // 1000 00sw
  {
    {"add",     INSTR_ADD,      INSTR_FMT_DATASW,   0},
    {"or",      INSTR_OR,       INSTR_FMT_DATASW,   0},
    {"adc",     INSTR_ADC,      INSTR_FMT_DATASW,   0},
    {"sbb",     INSTR_SBB,      INSTR_FMT_DATASW,   0},
    {"and",     INSTR_AND,      INSTR_FMT_DATASW,   0},
    {"sub",     INSTR_SUB,      INSTR_FMT_DATASW,   0},
    {"xor",     INSTR_XOR,      INSTR_FMT_DATASW,   0},
    {"cmp",     INSTR_CMP,      INSTR_FMT_DATASW,   0},
  },

  // 1111 011w
  {
    {"test",    INSTR_TEST,     INSTR_FMT_DATAW,    0},
    {"<error>", INSTR_UNKNOWN,  INSTR_FMT_NO_DATA,  0},
    {"not",     INSTR_NOT,      INSTR_FMT_NO_DATA,  0},
    {"neg",     INSTR_NEG,      INSTR_FMT_NO_DATA,  0},
    {"mul",     INSTR_MUL,      INSTR_FMT_NO_DATA,  0},
    {"imul",    INSTR_IMUL,     INSTR_FMT_NO_DATA,  0},
    {"div",     INSTR_DIV,      INSTR_FMT_NO_DATA,  0},
    {"idiv",    INSTR_IDIV,     INSTR_FMT_NO_DATA,  0},
  },

  // 1111 111w
  {
    {"inc",     INSTR_INC,      INSTR_FMT_NO_DATA,  0},
    {"dec",     INSTR_DEC,      INSTR_FMT_NO_DATA,  0},
    {"call",    INSTR_CALL,     INSTR_FMT_NO_DATA,  0},
    {"call",    INSTR_CALL,     INSTR_FMT_NO_DATA,  1},
    {"jmp",     INSTR_JMP,      INSTR_FMT_NO_DATA,  0},
    {"jmp",     INSTR_JMP,      INSTR_FMT_NO_DATA,  1},
    {"push",    INSTR_PUSH,     INSTR_FMT_NO_DATA,  0},
    {"<error>", INSTR_UNKNOWN,  INSTR_FMT_NO_DATA,  0},
  },

  // 1101 00vw
  {
    {"rol",     INSTR_ROL,      INSTR_FMT_NO_DATA,  0},
    {"ror",     INSTR_ROR,      INSTR_FMT_NO_DATA,  0},
    {"rcl",     INSTR_RCL,      INSTR_FMT_NO_DATA,  0},
    {"rcr",     INSTR_RCR,      INSTR_FMT_NO_DATA,  0},
    {"shl",     INSTR_SHL,      INSTR_FMT_NO_DATA,  0},
    {"shr",     INSTR_SHR,      INSTR_FMT_NO_DATA,  0},
    {"<error>", INSTR_UNKNOWN,  INSTR_FMT_NO_DATA,  0},
    {"sar",     INSTR_SAR,      INSTR_FMT_NO_DATA,  0},
  },
};

// Instruction tables

// Byte 0 flags
#define F_SEG_REG     {FLAG_SEG_REG,  0, .pos = 3,     .size_bits = 2}
#define F_REG         {FLAG_REG,      0, .pos = 0,     .size_bits = 3}
#define F_W           {FLAG_W,        0, .pos = 0,     .size_bits = 1}
#define F_W_ALT       {FLAG_W,        0, .pos = 3,     .size_bits = 1}
#define F_D           {FLAG_D,        0, .pos = 1,     .size_bits = 1}
#define F_S           {FLAG_S,        0, .pos = 1,     .size_bits = 1}
#define F_V           {FLAG_V,        0, .pos = 1,     .size_bits = 1}

#define F_IMPL_REG_A  {FLAG_REG,      1, .value = REG_A, .is_always_wide = 0}
#define F_IMPL_REG_D  {FLAG_REG,      1, .value = REG_D, .is_always_wide = 1}
#define F_IMPL_D      {FLAG_D,        1, .value = 1}

static const struct instr_table_row s_instr_rows[] = {
  {"push",  INSTR_PUSH,   0xE7, 0x06, -1, {F_SEG_REG},                        DISPL_FMT_NONE,   INSTR_FMT_NO_DATA}, // 000s r110    - push SEG
  {"pop",   INSTR_POP,    0xE7, 0x07, -1, {F_SEG_REG},                        DISPL_FMT_NONE,   INSTR_FMT_NO_DATA}, // 000s r111    - pop SEG

  {"mov",   INSTR_MOV,    0xF0, 0xB0, -1, {F_W_ALT, F_REG},                   DISPL_FMT_NONE,   INSTR_FMT_DATAW},   // 1011 wreg    - mov IMM to REG

  {"esc",   INSTR_ESC,    0xF8, 0xD8, -1, {0},                                DISPL_FMT_REG_RM, INSTR_FMT_NO_DATA}, // 1101 1xxx    - escape to ext device
  {"push",  INSTR_PUSH,   0xF8, 0x50, -1, {F_REG},                            DISPL_FMT_NONE,   INSTR_FMT_NO_DATA}, // 0101 0reg    - push REG
  {"pop",   INSTR_POP,    0xF8, 0x58, -1, {F_REG},                            DISPL_FMT_NONE,   INSTR_FMT_NO_DATA}, // 0101 1reg    - pop REG
  {"inc",   INSTR_INC,    0xF8, 0x40, -1, {F_REG},                            DISPL_FMT_NONE,   INSTR_FMT_NO_DATA}, // 0100 0reg    - inc REG
  {"dec",   INSTR_DEC,    0xF8, 0x48, -1, {F_REG},                            DISPL_FMT_NONE,   INSTR_FMT_NO_DATA}, // 0100 1reg    - dec REG
  {"xchg",  INSTR_XCHG,   0xF8, 0x90, -1, {F_REG, F_IMPL_REG_A},              DISPL_FMT_NONE,   INSTR_FMT_NO_DATA}, // 1001 0reg    - xchg ACC & REG

  {"mov",   INSTR_MOV,    0xFC,  0x88, -1, {F_D, F_W},                        DISPL_FMT_REG_RM, INSTR_FMT_NO_DATA}, // 1000 10dw    - mov R/M to/f REG
  {"mov",   INSTR_MOV,    0xFC,  0xA0, -1, {F_D, F_W, F_IMPL_REG_A},          DISPL_FMT_NONE,   INSTR_FMT_ADDR},    // 1010 00dw    - mov MEM to/f ACC
  {"add",   INSTR_ADD,    0xFC,  0x00, -1, {F_D, F_W},                        DISPL_FMT_REG_RM, INSTR_FMT_NO_DATA}, // 0000 00dw    - add R/M + REG
  {"or",    INSTR_OR,     0xFC,  0x08, -1, {F_D, F_W},                        DISPL_FMT_REG_RM, INSTR_FMT_NO_DATA}, // 0000 10dw    - or  R/M & REG
  {"adc",   INSTR_ADC,    0xFC,  0x10, -1, {F_D, F_W},                        DISPL_FMT_REG_RM, INSTR_FMT_NO_DATA}, // 0001 00dw    - adc R/M + REG
  {"and",   INSTR_AND,    0xFC,  0x20, -1, {F_D, F_W},                        DISPL_FMT_REG_RM, INSTR_FMT_NO_DATA}, // 0010 00dw    - and R/M & REG
  {"sub",   INSTR_SUB,    0xFC,  0x28, -1, {F_D, F_W},                        DISPL_FMT_REG_RM, INSTR_FMT_NO_DATA}, // 0010 10dw    - sub R/M + REG
  {"sbb",   INSTR_SBB,    0xFC,  0x18, -1, {F_D, F_W},                        DISPL_FMT_REG_RM, INSTR_FMT_NO_DATA}, // 0001 10dw    - sbb R/M + REG
  {"xor",   INSTR_XOR,    0xFC,  0x30, -1, {F_D, F_W},                        DISPL_FMT_REG_RM, INSTR_FMT_NO_DATA}, // 0011 00dw    - xor R/M ^ REG
  {"cmp",   INSTR_CMP,    0xFC,  0x38, -1, {F_D, F_W},                        DISPL_FMT_REG_RM, INSTR_FMT_NO_DATA}, // 0011 10dw    - cmp R/M + REG
  {"<0>",   INSTR_UNKNOWN,0xFC,  0x80,  0, {F_S, F_W},                        DISPL_FMT_RM,     INSTR_FMT_DATASW},  // 1000 00sw    - (add|sub|..) R/M IMM
  {"<3>",   INSTR_UNKNOWN,0xFC,  0xD0,  3, {F_V, F_W},                        DISPL_FMT_RM,     INSTR_FMT_NO_DATA}, // 1101 00vw    - (shl|shr|..) R/M

  {"mov",   INSTR_MOV,    0xFE,  0xC6, -1, {F_W},                             DISPL_FMT_RM,     INSTR_FMT_DATAW},   // 1100 011w    - mov IMM to R/M
  {"xchg",  INSTR_XCHG,   0xFE,  0x86, -1, {F_W, F_IMPL_D},                   DISPL_FMT_REG_RM, INSTR_FMT_NO_DATA}, // 1000 011w    - xchg R/M w REG
  {"add",   INSTR_ADD,    0xFE,  0x04, -1, {F_W, F_IMPL_REG_A},               DISPL_FMT_NONE,   INSTR_FMT_DATAW},   // 0000 010w    - add ACC + IMM
  {"or",    INSTR_OR,     0xFE,  0x0C, -1, {F_W, F_IMPL_REG_A},               DISPL_FMT_NONE,   INSTR_FMT_DATAW},   // 0000 110w    - or  ACC | IMM
  {"adc",   INSTR_ADC,    0xFE,  0x14, -1, {F_W, F_IMPL_REG_A},               DISPL_FMT_NONE,   INSTR_FMT_DATAW},   // 1000 000w    - adc ACC + IMM
  {"sub",   INSTR_SUB,    0xFE,  0x2C, -1, {F_W, F_IMPL_REG_A},               DISPL_FMT_NONE,   INSTR_FMT_DATAW},   // 0010 110w    - sub ACC - IMM
  {"and",   INSTR_AND,    0xFE,  0x24, -1, {F_W, F_IMPL_REG_A},               DISPL_FMT_NONE,   INSTR_FMT_DATAW},   // 0010 010w    - and ACC & IMM
  {"sbb",   INSTR_SBB,    0xFE,  0x1C, -1, {F_W, F_IMPL_REG_A},               DISPL_FMT_NONE,   INSTR_FMT_DATAW},   // 0001 110w    - sbb ACC - IMM
  {"<2>",   INSTR_UNKNOWN,0xFE,  0xFE,  2, {F_W},                             DISPL_FMT_RM,     INSTR_FMT_NO_DATA}, // 1111 111w    - (inc|dec|..) R/M
  {"xor",   INSTR_XOR,    0xFE,  0x34, -1, {F_W, F_IMPL_REG_A},               DISPL_FMT_NONE,   INSTR_FMT_DATAW},   // 0011 010w    - xor ACC ^ IMM
  {"cmp",   INSTR_CMP,    0xFE,  0x3C, -1, {F_W, F_IMPL_REG_A},               DISPL_FMT_NONE,   INSTR_FMT_DATAW},   // 0011 110w    - cmp ACC & IMM
  {"in",    INSTR_IN,     0xFE,  0xE4, -1, {F_W, F_IMPL_REG_A},               DISPL_FMT_NONE,   INSTR_FMT_DATA8},   // 1110 010w    - in  ACC IMM8
  {"out",   INSTR_OUT,    0xFE,  0xE6, -1, {F_W, F_IMPL_REG_A, F_IMPL_D},     DISPL_FMT_NONE,   INSTR_FMT_DATA8},   // 1110 011w    - out ACC IMM8
  {"<1>",   INSTR_UNKNOWN,0xFE,  0xF6,  1, {F_W},                             DISPL_FMT_RM,     INSTR_FMT_NO_DATA}, // 1111 011w    - (mul|div|..) R/M
  {"test",  INSTR_TEST,   0xFE,  0x84, -1, {F_W},                             DISPL_FMT_REG_RM, INSTR_FMT_NO_DATA}, // 1000 010w    - test R/M & REG
  {"test",  INSTR_TEST,   0xFE,  0xA8, -1, {F_W, F_IMPL_REG_A},               DISPL_FMT_NONE,   INSTR_FMT_DATAW},   // 1010 100w    - test ACC & IMM
  {"in",    INSTR_IN,     0xFE,  0xEC, -1, {F_W, F_IMPL_REG_A, F_IMPL_REG_D}, DISPL_FMT_NONE,   INSTR_FMT_NO_DATA}, // 1110 110w    - in  ACC dx
  {"out",   INSTR_OUT,    0xFE,  0xEE, -1, {F_W, F_IMPL_REG_D, F_IMPL_REG_A}, DISPL_FMT_NONE,   INSTR_FMT_NO_DATA}, // 1110 111w    - out ACC dx
  {"mov",   INSTR_MOV,    0xFF,  0x8C, -1, {0},                        DISPL_FMT_SR_RM,  INSTR_FMT_NO_DATA}, // 1000 1100    - mov SEG to R/M
  {"mov",   INSTR_MOV,    0xFF,  0x8E, -1, {F_IMPL_D},                               DISPL_FMT_SR_RM,  INSTR_FMT_NO_DATA}, // 1000 1110    - mov R/M to SEG
  {"pop",   INSTR_POP,    0xFF,  0x8F, -1, {0},                               DISPL_FMT_RM,     INSTR_FMT_NO_DATA}, // 1000 1111    - pop R/M
  {"je",    INSTR_JE,     0xFF,  0x74, -1, {0},                               DISPL_FMT_NONE,   INSTR_FMT_IP_INC8}, // 0111 0100    - je /jz   IP-INC8
  {"jl",    INSTR_JL,     0xFF,  0x7C, -1, {0},                               DISPL_FMT_NONE,   INSTR_FMT_IP_INC8}, // 0111 1100    - jl /jnge IP-INC8
  {"jng",   INSTR_JNG,    0xFF,  0x7E, -1, {0},                               DISPL_FMT_NONE,   INSTR_FMT_IP_INC8}, // 0111 1110    - jng/jle  IP-INC8
  {"jb",    INSTR_JB,     0xFF,  0x72, -1, {0},                               DISPL_FMT_NONE,   INSTR_FMT_IP_INC8}, // 0111 0010    - jb /jnae IP-INC8
  {"jbe",   INSTR_JBE,    0xFF,  0x76, -1, {0},                               DISPL_FMT_NONE,   INSTR_FMT_IP_INC8}, // 0111 0110    - jbe/jna  IP-INC8
  {"jp",    INSTR_JP,     0xFF,  0x7A, -1, {0},                               DISPL_FMT_NONE,   INSTR_FMT_IP_INC8}, // 0111 1010    - jp /jpe  IP-INC8
  {"jo",    INSTR_JO,     0xFF,  0x70, -1, {0},                               DISPL_FMT_NONE,   INSTR_FMT_IP_INC8}, // 0111 0000    - jo       IP-INC8
  {"js",    INSTR_JS,     0xFF,  0x78, -1, {0},                               DISPL_FMT_NONE,   INSTR_FMT_IP_INC8}, // 0111 1000    - js       IP-INC8
  {"jne",   INSTR_JNE,    0xFF,  0x75, -1, {0},                               DISPL_FMT_NONE,   INSTR_FMT_IP_INC8}, // 0111 0101    - jne/jnz  IP-INC8
  {"jnl",   INSTR_JNL,    0xFF,  0x7D, -1, {0},                               DISPL_FMT_NONE,   INSTR_FMT_IP_INC8}, // 0111 1101    - jnl/jge  IP-INC8
  {"jg",    INSTR_JG,     0xFF,  0x7F, -1, {0},                               DISPL_FMT_NONE,   INSTR_FMT_IP_INC8}, // 0111 1111    - jg /jnle IP-INC8
  {"jnb",   INSTR_JNB,    0xFF,  0x73, -1, {0},                               DISPL_FMT_NONE,   INSTR_FMT_IP_INC8}, // 0111 0011    - jnb/jae  IP-INC8
  {"jnbe",  INSTR_JNBE,   0xFF,  0x77, -1, {0},                               DISPL_FMT_NONE,   INSTR_FMT_IP_INC8}, // 0111 0111    - jnb/jae  IP-INC8
  {"jnp",   INSTR_JNP,    0xFF,  0x7B, -1, {0},                               DISPL_FMT_NONE,   INSTR_FMT_IP_INC8}, // 0111 1011    - jnp/jpo  IP-INC8
  {"jno",   INSTR_JNO,    0xFF,  0x71, -1, {0},                               DISPL_FMT_NONE,   INSTR_FMT_IP_INC8}, // 0111 0001    - jno      IP-INC8
  {"jns",   INSTR_JNS,    0xFF,  0x79, -1, {0},                               DISPL_FMT_NONE,   INSTR_FMT_IP_INC8}, // 0111 1001    - jno      IP-INC8
  {"jcxz",  INSTR_JCXZ,   0xFF,  0xE3, -1, {0},                               DISPL_FMT_NONE,   INSTR_FMT_IP_INC8}, // 1110 0011    - jsxz     IP-INC8
  {"loop",  INSTR_LOOP,   0xFF,  0xE2, -1, {0},                               DISPL_FMT_NONE,   INSTR_FMT_IP_INC8}, // 1110 0010    - loop CX times
  {"loopz", INSTR_LOOPZ,  0xFF,  0xE1, -1, {0},                               DISPL_FMT_NONE,   INSTR_FMT_IP_INC8}, // 1110 0001    - loopz/loope
  {"loopnz",INSTR_LOOPNZ, 0xFF,  0xE0, -1, {0},                               DISPL_FMT_NONE,   INSTR_FMT_IP_INC8}, // 1110 0000    - loopnz/loopne
  {"jmp",   INSTR_JMP,    0xFF,  0xEB, -1, {0},                               DISPL_FMT_NONE,   INSTR_FMT_IP_INC8}, // 1110 1001    - jmp in SEG short
  {"call",  INSTR_CALL,   0xFF,  0xE8, -1, {0},                               DISPL_FMT_NONE,   INSTR_FMT_IP_INC16},// 1110 1000    - call direct in SEG
  {"jmp",   INSTR_JMP,    0xFF,  0xE9, -1, {0},                               DISPL_FMT_NONE,   INSTR_FMT_IP_INC16},// 1110 1001    - jmp direct in SEG
  {"ret",   INSTR_RET,    0xFF,  0xC2, -1, {0},                               DISPL_FMT_NONE,   INSTR_FMT_DATA16},  // 1100 0010    - ret SP + IMM16 in SEG
  {"retf",  INSTR_RETF,   0xFF,  0xCA, -1, {0},                               DISPL_FMT_NONE,   INSTR_FMT_DATA16},  // 1100 1010    - ret SP + IMM16 inter SEG
  {"ret",   INSTR_RET,    0xFF,  0xC3, -1, {0},                               DISPL_FMT_NONE,   INSTR_FMT_NO_DATA}, // 1100 0011    - ret in SEG
  {"retf",  INSTR_RETF,   0xFF,  0xCB, -1, {0},                               DISPL_FMT_NONE,   INSTR_FMT_NO_DATA}, // 1100 1011    - ret inter SEG
  {"xlat",  INSTR_XLAT,   0xFF,  0xD7, -1, {0},                               DISPL_FMT_NONE,   INSTR_FMT_NO_DATA}, // 1101 0111    - byte to AL
  {"lea",   INSTR_LEA,    0xFF,  0x8D, -1, {F_IMPL_D},                        DISPL_FMT_REG_RM, INSTR_FMT_NO_DATA}, // 1000 1101    - lea R/M to REG
  {"lds",   INSTR_LDS,    0xFF,  0xC5, -1, {F_IMPL_D},                        DISPL_FMT_REG_RM, INSTR_FMT_NO_DATA}, // 1100 0101    - load ptr to DS
  {"les",   INSTR_LES,    0xFF,  0xC4, -1, {F_IMPL_D},                        DISPL_FMT_REG_RM, INSTR_FMT_NO_DATA}, // 1100 0100    - load ptr to ES
  {"lahf",  INSTR_LAHF,   0xFF,  0x9F, -1, {0},                               DISPL_FMT_NONE,   INSTR_FMT_NO_DATA}, // 1001 1111    - load FLAGS to AH
  {"sahf",  INSTR_SAHF,   0xFF,  0x9E, -1, {0},                               DISPL_FMT_NONE,   INSTR_FMT_NO_DATA}, // 1001 1110    - store AH to FLAGS
  {"call",  INSTR_CALL,   0xFF,  0x9A, -1, {0},                               DISPL_FMT_NONE,   INSTR_FMT_SEG_ADDR},// 1001 1010    - call direct intra SEG
  {"jmp",   INSTR_JMP,    0xFF,  0xEA, -1, {0},                               DISPL_FMT_NONE,   INSTR_FMT_SEG_ADDR},// 1110 1010    - jmp direct intra SEG
  {"pushf", INSTR_PUSHF,  0xFF,  0x9C, -1, {0},                               DISPL_FMT_NONE,   INSTR_FMT_NO_DATA}, // 1001 1110    - push FLAGS
  {"popf",  INSTR_POPF,   0xFF,  0x9D, -1, {0},                               DISPL_FMT_NONE,   INSTR_FMT_NO_DATA}, // 1001 1110    - pop FLAGS
  {"aaa",   INSTR_AAA,    0xFF,  0x37, -1, {0},                               DISPL_FMT_NONE,   INSTR_FMT_NO_DATA}, // 0011 0111    - ASCII adjust for add
  {"daa",   INSTR_DAA,    0xFF,  0x27, -1, {0},                               DISPL_FMT_NONE,   INSTR_FMT_NO_DATA}, // 0010 0111    - Dec adjust for add
  {"aas",   INSTR_AAS,    0xFF,  0x3F, -1, {0},                               DISPL_FMT_NONE,   INSTR_FMT_NO_DATA}, // 0011 1111    - ASCII adjust for sub
  {"das",   INSTR_DAS,    0xFF,  0x2F, -1, {0},                               DISPL_FMT_NONE,   INSTR_FMT_NO_DATA}, // 0010 1111    - Dec adjust for sub
  {"aam",   INSTR_AAM,    0xFF,  0xD4, -1, {0},                               DISPL_FMT_NONE,   INSTR_FMT_SWALLOW8},// 1101 0100    - ASCII adjust for mul
  {"aad",   INSTR_AAD,    0xFF,  0xD5, -1, {0},                               DISPL_FMT_NONE,   INSTR_FMT_SWALLOW8},// 1101 0101    - ASCII adjust for div
  {"cbw",   INSTR_CBW,    0xFF,  0x98, -1, {0},                               DISPL_FMT_NONE,   INSTR_FMT_NO_DATA}, // 1001 0100    - byte -> word
  {"cwd",   INSTR_CWD,    0xFF,  0x99, -1, {0},                               DISPL_FMT_NONE,   INSTR_FMT_NO_DATA}, // 1001 0101    - word -> dword
  {"movsb", INSTR_MOVSB,  0xFF,  0xA4, -1, {0},                               DISPL_FMT_NONE,   INSTR_FMT_NO_DATA}, // 1010 0100    - movs byte SI,DI
  {"movsw", INSTR_MOVSW,  0xFF,  0xA5, -1, {0},                               DISPL_FMT_NONE,   INSTR_FMT_NO_DATA}, // 1010 0101    - movs word SI,DI
  {"cmpsb", INSTR_CMPSB,  0xFF,  0xA6, -1, {0},                               DISPL_FMT_NONE,   INSTR_FMT_NO_DATA}, // 1010 0100    - cmps byte SI,DI
  {"cmpsw", INSTR_CMPSW,  0xFF,  0xA7, -1, {0},                               DISPL_FMT_NONE,   INSTR_FMT_NO_DATA}, // 1010 0101    - cmps word SI,DI
  {"scasb", INSTR_SCASB,  0xFF,  0xAE, -1, {0},                               DISPL_FMT_NONE,   INSTR_FMT_NO_DATA}, // 1010 1110    - scas byte SI,DI
  {"scasw", INSTR_SCASW,  0xFF,  0xAF, -1, {0},                               DISPL_FMT_NONE,   INSTR_FMT_NO_DATA}, // 1010 1111    - scas word SI,DI
  {"lodsb", INSTR_LODSB,  0xFF,  0xAC, -1, {0},                               DISPL_FMT_NONE,   INSTR_FMT_NO_DATA}, // 1010 1100    - lods byte SI,DI
  {"lodsw", INSTR_LODSW,  0xFF,  0xAD, -1, {0},                               DISPL_FMT_NONE,   INSTR_FMT_NO_DATA}, // 1010 1101    - lods word SI,DI
  {"stosb", INSTR_STOSB,  0xFF,  0xAA, -1, {0},                               DISPL_FMT_NONE,   INSTR_FMT_NO_DATA}, // 1010 1010    - stos byte SI,DI
  {"stosw", INSTR_STOSW,  0xFF,  0xAB, -1, {0},                               DISPL_FMT_NONE,   INSTR_FMT_NO_DATA}, // 1010 1011    - stos word SI,DI
  {"int3",  INSTR_INT,    0xFF,  0xCC, -1, {0},                               DISPL_FMT_NONE,   INSTR_FMT_NO_DATA}, // 1100 1100    - int3
  {"int",   INSTR_INT,    0xFF,  0xCD, -1, {0},                               DISPL_FMT_NONE,   INSTR_FMT_DATA8},   // 1100 1101    - int IMM8
  {"into",  INSTR_INTO,   0xFF,  0xCE, -1, {0},                               DISPL_FMT_NONE,   INSTR_FMT_NO_DATA}, // 1100 1110    - int on overflow
  {"iret",  INSTR_IRET,   0xFF,  0xCF, -1, {0},                               DISPL_FMT_NONE,   INSTR_FMT_NO_DATA}, // 1100 1111    - return from int
  {"cmc",   INSTR_CMC,    0xFF,  0xF5, -1, {0},                               DISPL_FMT_NONE,   INSTR_FMT_NO_DATA}, // 1111 0101    - complement carry
  {"clc",   INSTR_CLC,    0xFF,  0xF8, -1, {0},                               DISPL_FMT_NONE,   INSTR_FMT_NO_DATA}, // 1111 1000    - clear carry
  {"stc",   INSTR_STC,    0xFF,  0xF9, -1, {0},                               DISPL_FMT_NONE,   INSTR_FMT_NO_DATA}, // 1111 0101    - set carry
  {"cld",   INSTR_CLD,    0xFF,  0xFC, -1, {0},                               DISPL_FMT_NONE,   INSTR_FMT_NO_DATA}, // 1111 1100    - clear direction
  {"std",   INSTR_STD,    0xFF,  0xFD, -1, {0},                               DISPL_FMT_NONE,   INSTR_FMT_NO_DATA}, // 1111 1101    - set direction
  {"cli",   INSTR_CLI,    0xFF,  0xFA, -1, {0},                               DISPL_FMT_NONE,   INSTR_FMT_NO_DATA}, // 1111 1010    - clear interrupt
  {"sti",   INSTR_STI,    0xFF,  0xFB, -1, {0},                               DISPL_FMT_NONE,   INSTR_FMT_NO_DATA}, // 1111 1011    - set interrupt
  {"hlt",   INSTR_HLT,    0xFF,  0xF4, -1, {0},                               DISPL_FMT_NONE,   INSTR_FMT_NO_DATA}, // 1111 0100    - halt (wait for ext int)
  {"wait",  INSTR_WAIT,   0xFF,  0x9B, -1, {0},                               DISPL_FMT_NONE,   INSTR_FMT_NO_DATA}, // 1001 1011    - wait
};

static const struct instr_table_row *s_b0_to_row[256] = {0};

void decode_build_index(void) {
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

static FORCE_INLINE enum prefix_type decode_prefix(u8 b, struct prefixes *out) {
  enum prefix_type type = PREFIX_NONE;

  if ((b & 0xE7) == 0x26) {
    type = PREFIX_SEG;
    struct reg seg_reg = seg_reg_from_bits((b & 0x18) >> 3);
    out->seg = reg_ptr(seg_reg.type, seg_reg.mode);
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
// find instruction row matching first byte b0 using index
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

  u8 *stream_instr_begin = stream->data;

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
        .reg = reg_from_bits(f, is_wide),
        .type = OP_REG
      };
    } else if (flag.type == FLAG_SEG_REG) {
      ret.ops[ops_size++] = (struct operand){
        .reg = seg_reg_from_bits(f),
        .type = OP_REG
      };
    } else {
      flags[flag.type] = f;
    }
  }

  // opcode 1

  // byte0 flags
  u8 w = flags[FLAG_W] > 0;
  u8 d = flags[FLAG_D] > 0; // d - instruction has displacement w/o data
  u8 s = flags[FLAG_S] > 0; // s - instruction has displacement with immediate
  u8 v = flags[FLAG_V] > 0; // v = 1, rotate count in CL register

  ret.str = row->str;
  ret.type = row->type;
  ret.w = w;

  enum instr_fmt instr_fmt = row->fmt;

  // read displacement
  if (row->displ_fmt != DISPL_FMT_NONE) {
    u8 b1;
    CHECK(stream_read_b(stream, &b1), "Error: failed to read displacement\n");

    u8 mod = (b1 & 0xC0) >> 6;
    u8 reg = (b1 & 0x38) >> 3;
    u8 sr  = (b1 & 0x18) >> 3;
    u8 rm  = (b1 & 0x07);

    // first operand is either encoded in rm or in following displacement bits
    if (mod == 3) {
      // only rm flag matters
      ret.ops[ops_size++] = (struct operand){
        .reg = reg_from_bits(rm, w),
        .type = OP_REG
      };
    } else {
      // read displacement
      struct ea ea = {.seg = ret.prefixes.seg};
      CHECK(decode_effective_address(stream, rm, mod, &ea),
          "Error: mov no displacement\n");
      ret.ops[ops_size++] = (struct operand){.ea = ea, .type = OP_EA};
    }


    if (row->displ_fmt == DISPL_FMT_RM) {
      // treat reg as auxiliary part of the opcode
      u8 opcode_aux = reg;

      // handle the rest of the opcode in Byte1
      if (row->b1_rows_idx >= 0) {
        const struct instr_table_b1_row *b1_rows = s_instr_b1_rows[row->b1_rows_idx];
        ret.str = b1_rows[opcode_aux].str;
        ret.type = b1_rows[opcode_aux].type;
        instr_fmt = b1_rows[opcode_aux].fmt;
        ret.is_far = b1_rows[opcode_aux].is_far;
      }
    } else {
      ret.ops[ops_size++] = (struct operand){
        .reg = row->displ_fmt == DISPL_FMT_REG_RM
          ? reg_from_bits(reg, w)
          : seg_reg_from_bits(sr),
        .type = OP_REG
      };
    }
  }

  if (flags[FLAG_V] >= 0) {
    ret.ops[ops_size++] = v
      ? (struct operand){.reg = (struct reg){REG_C, REG_MODE_L}, .type = OP_REG}
      : (struct operand){.data = 1, .type = OP_DATA};
  }

  // read data
  bool should_read_data = true;
  bool is_sign_extend = false;
  bool is_data_w = w;
  switch (instr_fmt) {
    case INSTR_FMT_NO_DATA:
      should_read_data = false;
      break;
    case INSTR_FMT_DATASW:
      is_sign_extend = s && w;
      is_data_w = is_sign_extend ? false : w;
      break;
    case INSTR_FMT_ADDR:
    case INSTR_FMT_DATAW:
    case INSTR_FMT_SEG_ADDR:
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
  assert((!d || ops_size > 1) && "d flag is set but not enough operands");

  if (d && ops_size > 1) {
    SWAP(ret.ops[0], ret.ops[1]);
  }

  ret.size = stream->data - stream_instr_begin;

  return ret;
}
