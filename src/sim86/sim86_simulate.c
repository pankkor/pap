#include "sim86_simulate.h"

#include <assert.h>   // assert
#include <stdio.h>    // fprintf

struct mem {
  u16 *ptr;
  u8 offset;
  u8 size;
};

u16 load_mem(struct mem mem) {
  u16 v = *mem.ptr;
  v >>= mem.offset;
  u16 mask = (0x1 << mem.size) - 1;
  return v & mask;
}

void store_mem(struct mem mem, u16 value) {
  u16 old = *mem.ptr;

  u16 mask = (0x1 << mem.size) - 1;
  mask = mask << mem.offset;

  u16 new = (old & ~mask) | ((value << mem.offset) & mask);
  *mem.ptr = new;
}

struct mem reg_mode_to_offset_size(enum reg_mode m) {
  assert(m != REG_MODE_COUNT);
  switch(m) {
    case REG_MODE_L:      return (struct mem){0, 0, 8};
    case REG_MODE_H:      return (struct mem){0, 8, 8};
    case REG_MODE_X:      return (struct mem){0, 0, 16};
    case REG_MODE_COUNT:  return (struct mem){0};
  }
}

u16 load_op(struct operand op, struct state *state) {
  switch(op.type) {
    case OP_NONE: return 0;

    case OP_REG: {
      struct reg reg = op.reg;
      struct mem mem = reg_mode_to_offset_size(reg.mode);
      mem.ptr = &state->regs[reg.type];
      return load_mem(mem);
    }

    case OP_DATA:
      return op.data;

    default: assert("unhandled type" || op.type);
  }
  return 0;
}

struct mem op_to_dst(struct operand op, struct state *state) {
  switch(op.type) {
    case OP_NONE: return (struct mem){0};

    case OP_REG: {
      struct reg reg = op.reg;
      struct mem mem = reg_mode_to_offset_size(reg.mode);
      mem.ptr = &state->regs[reg.type];
      return mem;
    }

    case OP_DATA:
      assert(0 && "Can't convert OP_DATA to memory");
    default: assert("unhandled type" || op.type);
  }
  return (struct mem){0};
}

struct state state_simulate_instr(const struct state *state,
    const struct instr *instr) {
  assert(instr->type != INSTR_UNKNOWN);

  struct state state_new = *state;

  switch(instr->type) {
    case INSTR_MOV: {
      struct mem dst = op_to_dst(instr->ops[0], &state_new);
      u16 v = load_op(instr->ops[1], &state_new);
      store_mem(dst, v);
    } break;

    default:
      fprintf(stderr, "Sim error: unhandled instruction '%s' of type %d\n",
          instr->str, instr->type);

  }

  return state_new;
}
