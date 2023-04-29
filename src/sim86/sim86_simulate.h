#pragma once

#include "sim86_types.h"
#include "sim86_instr.h"

struct state {
  u16 regs[REG_COUNT];
};

struct state state_simulate_instr(const struct state *state, const struct instr *instr);
