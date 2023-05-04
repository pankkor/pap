// Performance-Aware-Programming Course
// https://www.computerenhance.com/p/table-of-contents
//
// Part 1
// 8086 simple simulator
//
// Run 'test_decode.sh' that tests decoding on all the listings in listings dir

#pragma once

#include "sim86_types.h"

// Register
enum reg_type {
  REG_A,
  REG_C,
  REG_D,
  REG_B,
  REG_SP,
  REG_BP,
  REG_SI,
  REG_DI,
  REG_ES,
  REG_CS,
  REG_SS,
  REG_DS,
  REG_COUNT,
};

enum reg_mode {
  REG_MODE_L,
  REG_MODE_H,
  REG_MODE_X,
  REG_MODE_COUNT
};

struct reg {
  enum reg_type type;
  enum reg_mode mode;
};

// Valid register for type and addressing mode
// F.e. reg_ptr(REG_BP, REG_MODE_L) will return
// (struct reg){REG_BP, REG_MODE_X} as BP can only be accessed as full 16-bit
// register
const struct reg *reg_ptr(enum reg_type reg_type, enum reg_mode desired_mode);

// Correct string for register
// F.e. str_reg(struct reg){REG_BP, REG_MODE_L) will return "bp"
// (even though incorrect access mode was specified)
const char *str_reg(struct reg reg);

// Effective address calculation
struct compute_addr {
  const struct reg *base_reg;
  const struct reg *index_reg;
  i16 displ;
};

enum ea_type {EA_COMPUTE, EA_DIRECT};
struct ea {
  const struct reg *seg;
  enum ea_type type;

  union {
    struct compute_addr compute_addr;
    u16 direct_addr;
  };
};

// Instruction Operands
enum operand_type {OP_NONE, OP_REG, OP_EA, OP_DATA, OP_IP_INC};

struct operand {
  union {
    struct reg  reg;
    struct ea   ea;
    u16         data;
    i16         ip_inc;
  };
  enum operand_type type;
};

// Prefixes
enum prefix_type {
  PREFIX_NONE   = 0,
  PREFIX_LOCK   = 1 << 0,
  PREFIX_REP    = 1 << 1,
  PREFIX_REPNE  = 1 << 2,
  PREFIX_SEG    = 1 << 3,
};

struct prefixes {
  const struct reg *seg;
  enum prefix_type types;
};

// Decoded instruction
enum instr_type {
  INSTR_UNKNOWN, // error type
  INSTR_PUSH,
  INSTR_POP,
  INSTR_MOV,
  INSTR_INC,
  INSTR_DEC,
  INSTR_XCHG,
  INSTR_ADD,
  INSTR_OR,
  INSTR_ADC,
  INSTR_AND,
  INSTR_SUB,
  INSTR_SBB,
  INSTR_XOR,
  INSTR_CMP,
  INSTR_IN,
  INSTR_OUT,
  INSTR_TEST,
  INSTR_JE,
  INSTR_JL,
  INSTR_JLE,
  INSTR_JB,
  INSTR_JBE,
  INSTR_JP,
  INSTR_JO,
  INSTR_JS,
  INSTR_JNE,
  INSTR_JNL,
  INSTR_JG,
  INSTR_JNB,
  INSTR_JNBE,
  INSTR_JNP,
  INSTR_JNO,
  INSTR_JNS,
  INSTR_JCXZ,
  INSTR_LOOP,
  INSTR_LOOPZ,
  INSTR_LOOPNZ,
  INSTR_JMP,
  INSTR_CALL,
  INSTR_RET,
  INSTR_RETF,
  INSTR_XLAT,
  INSTR_LEA,
  INSTR_LDS,
  INSTR_LES,
  INSTR_LAHF,
  INSTR_SAHF,
  INSTR_PUSHF,
  INSTR_POPF,
  INSTR_AAA,
  INSTR_DAA,
  INSTR_AAS,
  INSTR_DAS,
  INSTR_AAM,
  INSTR_AAD,
  INSTR_CBW,
  INSTR_CWD,
  INSTR_MOVSB,
  INSTR_MOVSW,
  INSTR_CMPSB,
  INSTR_CMPSW,
  INSTR_SCASB,
  INSTR_SCASW,
  INSTR_LODSB,
  INSTR_LODSW,
  INSTR_STOSB,
  INSTR_STOSW,
  INSTR_INT,
  INSTR_INTO,
  INSTR_IRET,
  INSTR_CMC,
  INSTR_CLC,
  INSTR_STC,
  INSTR_CLD,
  INSTR_STD,
  INSTR_CLI,
  INSTR_STI,
  INSTR_HLT,
  INSTR_WAIT,
  INSTR_NOT,
  INSTR_NEG,
  INSTR_MUL,
  INSTR_IMUL,
  INSTR_DIV,
  INSTR_IDIV,
  INSTR_ROR,
  INSTR_ROL,
  INSTR_RCL,
  INSTR_RCR,
  INSTR_SHL,
  INSTR_SHR,
  INSTR_SAR,
  INSTR_ESC, // TODO: esc is not tested
};

struct instr {
  const char *str;
  enum instr_type type;
  struct operand ops[2];
  struct prefixes prefixes;
  bool set_cs_addr;
  u16 cs_addr;          // set cs to this new value
  u8 size;              // instruction size in bytes
  bool is_far;
  bool w;
};
