#include "sim86_simulate.h"

#include <assert.h>   // assert
#include <stdio.h>    // fprintf

struct mem {
  u8 *ptr;
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
  out_fr->bits[FLAGS_CARRY]   = ((res_unmasked & (sign_mask << 1)) != 0);
  out_fr->bits[FLAGS_PARITY]  = calc_parity(res_masked);
  out_fr->bits[FLAGS_ZERO]    = res_masked == 0;
  out_fr->bits[FLAGS_SIGN]    = (res_masked & sign_mask) != 0;
}

u16 load_mem(struct mem mem) {
  u8 *ptr = mem.ptr + mem.offset;
  switch (mem.size) {
    case 1:
      return *ptr;
    case 2:
      return *(u16 *)ptr;
    default:
      assert(0 && "Bad memory load size");
      return 0;
  }
}

void store_mem(struct mem mem, u16 value) {
  u8 *ptr = mem.ptr + mem.offset;
  *ptr = value;
  switch (mem.size) {
    case 1:
      *ptr = value;
      break;
    case 2:
      *(u16 *)ptr = value;
      break;
    default:
      assert(0 && "Bad memory store size");
  }
}

struct mem reg_mode_to_offset_size(enum reg_mode m) {
  assert(m != REG_MODE_COUNT);
  switch(m) {
    case REG_MODE_L:      return (struct mem){0, 0, 1};
    case REG_MODE_H:      return (struct mem){0, 1, 1};
    case REG_MODE_X:      return (struct mem){0, 0, 2};
    case REG_MODE_COUNT:  return (struct mem){0};
  }
  return (struct mem){0};
}

u16 load_reg(struct reg reg, struct state *state) {
  struct mem mem = reg_mode_to_offset_size(reg.mode);
  mem.ptr = (u8 *)&state->regs[reg.type];
  return load_mem(mem);
}

u16 load_op(struct operand op, struct state *state, bool w) {
  switch(op.type) {
    case OP_NONE:
      return 0;

    case OP_REG: {
      return load_reg(op.reg, state);
    }

    case OP_EA: {
      struct ea ea = op.ea;

      struct mem mem = {0, 0, w ? 2 : 1};

      switch(ea.type) {
        case EA_DIRECT:
          mem.ptr = state->memory->data + ea.direct_addr % MEMORY_SIZE;
          return load_mem(mem);

        case EA_COMPUTE: {
          u16 base = 0;
          u16 index = 0;
          i16 displ = ea.compute_addr.displ;
          if (ea.compute_addr.base_reg) {
            base = load_reg(*ea.compute_addr.base_reg, state);
          }
          if (ea.compute_addr.index_reg) {
            index = load_reg(*ea.compute_addr.index_reg, state);
          }
          assert(base + index + displ < MEMORY_SIZE);
          mem.ptr = state->memory->data + (base + index + displ) % MEMORY_SIZE;
          return load_mem(mem);
        }
      }
      assert(0 && "Unhandled EA type");
      break;
    }

    case OP_DATA:
      return op.data;

    case OP_IP_INC:
      return op.ip_inc;

    default:
      assert(0 && "Unhandled type" && op.type);
      break;
  }
  return 0;
}

struct mem op_to_dst(struct operand op, struct state *state, bool w) {
  switch(op.type) {
    case OP_NONE:
      return (struct mem){0};

    case OP_REG: {
      struct reg reg = op.reg;
      struct mem mem = reg_mode_to_offset_size(reg.mode);
      mem.ptr = (u8 *)&state->regs[reg.type];
      return mem;
    }

    case OP_DATA:
      assert(0 && "Can't convert OP_DATA to memory");
      break;

    case OP_EA: {
      struct ea ea = op.ea;

      struct mem mem = {0, 0, w ? 2 : 1};

      switch(ea.type) {
        case EA_DIRECT:
          mem.ptr = state->memory->data + ea.direct_addr % MEMORY_SIZE;
          return mem;

        case EA_COMPUTE: {
          u16 base = 0;
          u16 index = 0;
          i16 displ = ea.compute_addr.displ;
          if (ea.compute_addr.base_reg) {
            base = load_reg(*ea.compute_addr.base_reg, state);
          }
          if (ea.compute_addr.index_reg) {
            index = load_reg(*ea.compute_addr.index_reg, state);
          }
          mem.ptr = state->memory->data + (base + index + displ) % MEMORY_SIZE;
          return mem;
        }
      }
      assert(0 && "Unhandled EA type");
      break;
    }

    default:
      assert(0 && "Unhandled type" && op.type);
      break;
  }
  return (struct mem){0};
}

void cond_jump(struct state *state, const struct instr *instr, bool cond) {
  if (cond) {
    i16 ip_inc = load_op(instr->ops[0], state, instr->w);
    state->ip += ip_inc;
  }
}

struct state state_simulate_instr(const struct state *state,
    const struct instr *instr) {
  assert(instr->type != INSTR_UNKNOWN);

  struct state new_state = *state;
  new_state.ip += instr->size;

  u8 *flags = new_state.flags_reg.bits; // for readability only

  switch(instr->type) {
    case INSTR_MOV: {
      struct mem dst = op_to_dst(instr->ops[0], &new_state, instr->w);
      u16 v0 = load_op(instr->ops[1], &new_state, instr->w);
      store_mem(dst, v0);
    } break;

    case INSTR_ADD: {
      struct mem dst = op_to_dst(instr->ops[0], &new_state, instr->w);
      u16 v0 = load_op(instr->ops[0], &new_state, instr->w);
      u16 v1 = load_op(instr->ops[1], &new_state, instr->w);

      u16 sign_mask = calc_sign_mask(instr->w ? 16 : 8);
      u16 mask = calc_bits_mask(instr->w ? 16 : 8);
      u16 res = (v0 & mask) + (v1 & mask);
      u32 res_unmasked = (u32)v0 + v1;

      calc_flags_reg(&new_state.flags_reg, res, res_unmasked, sign_mask);
      flags[FLAGS_AUXILIARY_CARRY] = (((v0 & 0x0F) + (v1 & 0x0F)) & 0x10) != 0;
      flags[FLAGS_OVERFLOW] = (~(v0 ^ v1) & (v0 ^ res) & sign_mask) != 0;

      store_mem(dst, res);
    } break;

    case INSTR_SUB: {
      struct mem dst = op_to_dst(instr->ops[0], &new_state, instr->w);
      u16 v0 = load_op(instr->ops[0], &new_state, instr->w);
      u16 v1 = load_op(instr->ops[1], &new_state, instr->w);

      u16 sign_mask = calc_sign_mask(instr->w ? 16 : 8);
      u16 mask = calc_bits_mask(instr->w ? 16 : 8);
      u16 res = (v0 & mask) - (v1 & mask);
      u32 res_unmasked = (u32)v0 - v1;

      calc_flags_reg(&new_state.flags_reg, res, res_unmasked, sign_mask);
      flags[FLAGS_AUXILIARY_CARRY] = (((v0 & 0x0F) - (v1 & 0x0F)) & 0x10) != 0;
      flags[FLAGS_OVERFLOW] = ((v0 ^ v1) & (v0 ^ res) & sign_mask) != 0;

      store_mem(dst, res);
    } break;

    case INSTR_CMP: {
      u16 v0 = load_op(instr->ops[0], &new_state, instr->w);
      u16 v1 = load_op(instr->ops[1], &new_state, instr->w);

      u16 sign_mask = calc_sign_mask(instr->w ? 16 : 8);
      u16 mask = calc_bits_mask(instr->w ? 16 : 8);
      u16 res = (v0 & mask) - (v1 & mask);
      u32 res_unmasked = (u32)v0 - v1;

      calc_flags_reg(&new_state.flags_reg, res, res_unmasked, sign_mask);
      flags[FLAGS_AUXILIARY_CARRY] = (((v0 & 0x0F) - (v1 & 0x0F)) & 0x10) != 0;
      flags[FLAGS_OVERFLOW] = ((v0 ^ v1) & (v0 ^ res) & sign_mask) != 0;
    } break;

    // Jumps
    // above  and below - relationship of two unsinged values
    // grater and less  - relationship of two signed values
    case INSTR_JBE: {
      cond_jump(&new_state, instr, flags[FLAGS_CARRY] && flags[FLAGS_ZERO]);
    } break;

    case INSTR_JNBE: {
      cond_jump(&new_state, instr, !flags[FLAGS_CARRY] && !flags[FLAGS_ZERO]);
    } break;

    case INSTR_JB: {
      cond_jump(&new_state, instr, flags[FLAGS_CARRY]);
    } break;

    case INSTR_JNB: {
      cond_jump(&new_state, instr, !flags[FLAGS_CARRY]);
    } break;

    case INSTR_JE: {
      cond_jump(&new_state, instr, flags[FLAGS_ZERO]);
    } break;

    case INSTR_JNE: {
      cond_jump(&new_state, instr, !flags[FLAGS_ZERO]);
    } break;

    case INSTR_JG: {
      cond_jump(&new_state, instr,
          !flags[FLAGS_ZERO] && flags[FLAGS_SIGN] == flags[FLAGS_OVERFLOW]);
    } break;

    case INSTR_JNG: {
      cond_jump(&new_state, instr,
          flags[FLAGS_ZERO] && flags[FLAGS_SIGN] != flags[FLAGS_OVERFLOW]);
    } break;

    case INSTR_JL: {
      cond_jump(&new_state, instr, flags[FLAGS_SIGN] != flags[FLAGS_OVERFLOW]);
    } break;

    case INSTR_JNL: {
      cond_jump(&new_state, instr, flags[FLAGS_SIGN] == flags[FLAGS_OVERFLOW]);
    } break;

    case INSTR_JO: {
      cond_jump(&new_state, instr, flags[FLAGS_OVERFLOW]);
    } break;

    case INSTR_JNO: {
      cond_jump(&new_state, instr, !flags[FLAGS_OVERFLOW]);
    } break;

    case INSTR_JP: {
      cond_jump(&new_state, instr, flags[FLAGS_PARITY]);
    } break;

    case INSTR_JNP: {
      cond_jump(&new_state, instr, !flags[FLAGS_PARITY]);
    } break;

    case INSTR_JS: {
      cond_jump(&new_state, instr, flags[FLAGS_SIGN]);
    } break;

    case INSTR_JNS: {
      cond_jump(&new_state, instr, !flags[FLAGS_SIGN]);
    } break;

    case INSTR_JCXZ: {
      cond_jump(&new_state, instr, !new_state.regs[REG_C]);
    } break;

    case INSTR_LOOP: {
      --new_state.regs[REG_C];
      cond_jump(&new_state, instr, new_state.regs[REG_C]);
    } break;

    case INSTR_LOOPZ: {
      --new_state.regs[REG_C];
      cond_jump(&new_state, instr, new_state.regs[REG_C] && flags[FLAGS_ZERO]);
    } break;

    case INSTR_LOOPNZ: {
      --new_state.regs[REG_C];
      cond_jump(&new_state, instr, new_state.regs[REG_C] && !flags[FLAGS_ZERO]);
    } break;

    default:
      fprintf(stderr,
          "Simulation error: unhandled instruction '%s' of type %d\n",
          instr->str, instr->type);
  }

  return new_state;
}
