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
struct instr {
  const char *str;
  struct operand ops[2];
  struct prefixes prefixes;
  bool set_cs_addr;
  u16 cs_addr;          // set cs to this new value
  u8 size;              // instruction size in bytes
  bool is_far;
  bool w;
};
