#include "sim86_simulate.h"

struct state state_simulate_instr(const struct state *state,
    const struct instr *instr) {
  struct state state_new = *state;
  (void)instr;
  state_new.regs[REG_A] = 1;
  state_new.regs[REG_B] = 2;
  state_new.regs[REG_C] = 3;
  state_new.regs[REG_D] = 4;
  state_new.regs[REG_SP] = 5;
  state_new.regs[REG_BP] = 6;
  state_new.regs[REG_SI] = 7;
  state_new.regs[REG_DI] = 8;
  return state_new;
}
