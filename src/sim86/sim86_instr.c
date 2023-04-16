// Performance-Aware-Programming Course
// https://www.computerenhance.com/p/table-of-contents
//
// Part 1
// 8086 simple simulator
//
// Run 'test_decode.sh' that tests decoding on all the listings in listings dir

#include "sim86_instr.h"

static const struct reg s_regs[][REG_MODE_COUNT] = {
  {{REG_A,  REG_MODE_L}, {REG_A,  REG_MODE_H}, {REG_A,  REG_MODE_X}},
  {{REG_C,  REG_MODE_L}, {REG_C,  REG_MODE_H}, {REG_C,  REG_MODE_X}},
  {{REG_D,  REG_MODE_L}, {REG_D,  REG_MODE_H}, {REG_D,  REG_MODE_X}},
  {{REG_B,  REG_MODE_L}, {REG_B,  REG_MODE_H}, {REG_B,  REG_MODE_X}},
  {{REG_SP, REG_MODE_X}, {REG_SP, REG_MODE_X}, {REG_SP, REG_MODE_X}},
  {{REG_BP, REG_MODE_X}, {REG_BP, REG_MODE_X}, {REG_BP, REG_MODE_X}},
  {{REG_SI, REG_MODE_X}, {REG_SI, REG_MODE_X}, {REG_SI, REG_MODE_X}},
  {{REG_DI, REG_MODE_X}, {REG_DI, REG_MODE_X}, {REG_DI, REG_MODE_X}},
  {{REG_ES, REG_MODE_X}, {REG_ES, REG_MODE_X}, {REG_ES, REG_MODE_X}},
  {{REG_CS, REG_MODE_X}, {REG_CS, REG_MODE_X}, {REG_CS, REG_MODE_X}},
  {{REG_SS, REG_MODE_X}, {REG_SS, REG_MODE_X}, {REG_SS, REG_MODE_X}},
  {{REG_DS, REG_MODE_X}, {REG_DS, REG_MODE_X}, {REG_DS, REG_MODE_X}},
};

static const char s_reg_strs[][REG_MODE_COUNT][3] = {
  {"al", "ah", "ax"},
  {"cl", "ch", "cx"},
  {"dl", "dh", "dx"},
  {"bl", "bh", "bx"},
  {"sp", "sp", "sp"},
  {"bp", "bp", "bp"},
  {"si", "si", "si"},
  {"di", "di", "di"},
  {"es", "es", "es"},
  {"cs", "cs", "cs"},
  {"ss", "ss", "ss"},
  {"ds", "ds", "ds"},
};

_Static_assert(ARRAY_COUNT(s_regs) == ARRAY_COUNT(s_reg_strs),
    "size mismatch");

const struct reg *reg_ptr(enum reg_type reg_type,
    enum reg_mode desired_mode) {
  return &s_regs[reg_type][desired_mode & 0x3];
}

const char *str_reg(struct reg reg) {
  return s_reg_strs[reg.type][reg.mode];
}
