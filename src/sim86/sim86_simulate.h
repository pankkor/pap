#pragma once

#include "sim86_types.h"
#include "sim86_instr.h"

enum flags_bit {
  FLAGS_BIT_CARRY,
  FLAGS_BIT_PARITY,
  FLAGS_BIT_AUXILIARY_CARRY,
  FLAGS_BIT_ZERO,
  FLAGS_BIT_SIGN,
  FLAGS_BIT_OVERFLOW,
  FLAGS_BIT_INTERRUPT_ENABLE,
  FLAGS_BIT_DIRECTION,
  FLAGS_BIT_TRAP,
  FLAGS_BIT_COUNT,
};

struct flags_reg {
  u8 bits[FLAGS_BIT_COUNT];
};

struct state {
  struct flags_reg flags_reg;
  u16 regs[REG_COUNT];
};

struct state state_simulate_instr(const struct state *state, const struct instr *instr);
