#include "sim86_simulate.h"

#include <assert.h>   // assert
#include <stdio.h>    // fprintf

struct mem {
  u16 *ptr;
  u8 offset;
  u8 size;
};

static FORCE_INLINE u16 calc_sign_mask(u8 bits) {
  return 1 << (bits - 1);
}

static FORCE_INLINE u16 calc_bits_mask(u8 bits) {
  return (1 << bits) - 1;
}

static FORCE_INLINE bool calc_parity(u8 v) {
  return __builtin_popcount(v) % 2 == 0;
}

void calc_flags_reg(struct flags_reg *out_fr, u32 res_masked, u32 res_unmasked,
    u32 sign_mask) {
  out_fr->bits[FLAGS_BIT_CARRY]   = ((res_unmasked & (sign_mask << 1)) != 0);
  out_fr->bits[FLAGS_BIT_PARITY]  = calc_parity(res_masked);
  out_fr->bits[FLAGS_BIT_ZERO]    = res_masked == 0;
  out_fr->bits[FLAGS_BIT_SIGN]    = (res_masked & sign_mask) != 0;
}

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

  struct state new_state = *state;

  switch(instr->type) {
    case INSTR_MOV: {
      struct mem dst = op_to_dst(instr->ops[0], &new_state);
      u16 v0 = load_op(instr->ops[1], &new_state);
      store_mem(dst, v0);
    } break;

    case INSTR_ADD: {
      struct mem dst = op_to_dst(instr->ops[0], &new_state);
      u16 v0 = load_op(instr->ops[0], &new_state);
      u16 v1 = load_op(instr->ops[1], &new_state);

      u16 sign_mask = calc_sign_mask(instr->w ? 16 : 8);
      u16 mask = calc_bits_mask(instr->w ? 16 : 8);
      u16 res = (v0 & mask) + (v1 & mask);
      u32 res_unmasked = (u32)v0 + v1;

      calc_flags_reg(&new_state.flags_reg, res, res_unmasked, sign_mask);
      new_state.flags_reg.bits[FLAGS_BIT_AUXILIARY_CARRY] =
        (((v0 & 0x0F) + (v1 & 0x0F)) & 0x10) != 0;
      new_state.flags_reg.bits[FLAGS_BIT_OVERFLOW] =
        (~(v0 ^ v1) & (v0 ^ res) & sign_mask) != 0;

      store_mem(dst, res);
    } break;

    case INSTR_SUB: {
      struct mem dst = op_to_dst(instr->ops[0], &new_state);
      u16 v0 = load_op(instr->ops[0], &new_state);
      u16 v1 = load_op(instr->ops[1], &new_state);

      u16 sign_mask = calc_sign_mask(instr->w ? 16 : 8);
      u16 mask = calc_bits_mask(instr->w ? 16 : 8);
      u16 res = (v0 & mask) - (v1 & mask);
      u32 res_unmasked = (u32)v0 - v1;

      calc_flags_reg(&new_state.flags_reg, res, res_unmasked, sign_mask);
      new_state.flags_reg.bits[FLAGS_BIT_AUXILIARY_CARRY] =
        (((v0 & 0x0F) - (v1 & 0x0F)) & 0x10) != 0;
      new_state.flags_reg.bits[FLAGS_BIT_OVERFLOW] =
        ((v0 ^ v1) & (v0 ^ res) & sign_mask) != 0;

      store_mem(dst, res);
    } break;

    case INSTR_CMP: {
      u16 v0 = load_op(instr->ops[0], &new_state);
      u16 v1 = load_op(instr->ops[1], &new_state);

      u16 sign_mask = calc_sign_mask(instr->w ? 16 : 8);
      u16 mask = calc_bits_mask(instr->w ? 16 : 8);
      u16 res = (v0 & mask) - (v1 & mask);
      u32 res_unmasked = (u32)v0 - v1;

      calc_flags_reg(&new_state.flags_reg, res, res_unmasked, sign_mask);
      new_state.flags_reg.bits[FLAGS_BIT_AUXILIARY_CARRY] =
        (((v0 & 0x0F) - (v1 & 0x0F)) & 0x10) != 0;
      new_state.flags_reg.bits[FLAGS_BIT_OVERFLOW] =
        ((v0 ^ v1) & (v0 ^ res) & sign_mask) != 0;
    } break;

    default:
      fprintf(stderr,
          "Simulation error: unhandled instruction '%s' of type %d\n",
          instr->str, instr->type);

  }

  return new_state;
}
