#pragma once

#include "sim86_types.h"
#include "sim86_instr.h"

enum flags_bit {
  FLAGS_CARRY,
  FLAGS_PARITY,
  FLAGS_AUXILIARY_CARRY,
  FLAGS_ZERO,
  FLAGS_SIGN,
  FLAGS_OVERFLOW,
  FLAGS_INTERRUPT_ENABLE,
  FLAGS_DIRECTION,
  FLAGS_TRAP,
  FLAGS_COUNT,
};

struct flags_reg {
  u8 bits[FLAGS_COUNT];
};

enum {MEMORY_SIZE = 1024 * 1024};

struct state {
  u8 memory[MEMORY_SIZE];
  struct flags_reg flags_reg;
  u16 regs[REG_COUNT];
  u16 ip;
};

struct state state_simulate_instr(const struct state *state, const struct instr *instr);
