#include "sim86_clocks.h"
#include "sim86_instr.h"

#include "stdio.h"  // fprintf

FORCE_INLINE bool is_all_op_of(const struct instr *instr, enum operand_type op_type) {
  return instr->ops[0].type == op_type && instr->ops[1].type == op_type;
}

FORCE_INLINE bool is_any_op_of(const struct instr *instr, enum operand_type op_type) {
  return instr->ops[0].type == op_type || instr->ops[1].type == op_type;
}

FORCE_INLINE bool is_any_op_ax(const struct instr *instr) {
  for (int i = 0; i < 2; ++i) {
    if (instr->ops[i].type == OP_REG && instr->ops[i].reg.type == REG_A) {
      return true;
    }
  }
  return false;
}

struct clocks calc_clocks(const struct instr *instr, bool is_8088) {
  // TODO
  (void)is_8088;

  switch(instr->type) {
    case INSTR_MOV:
      if (is_all_op_of(instr, OP_REG)) {
        return (struct clocks){2, 0, 0};
      }
      // TODO decide which way is better
      // if (is_any_op_ax(instr) && is_any_op_of(instr, OP_EA)) {
      //   return 10;
      // }

      if (instr->ops[0].type == OP_REG && instr->ops[1].type == OP_EA) {
        return instr->ops[0].reg.type == REG_A
          ? (struct clocks){10, 0, 0}
          : (struct clocks){8, 0/*ea*/, 0};// TODO ea
      }
      if (instr->ops[1].type == OP_REG && instr->ops[0].type == OP_EA) {
        return instr->ops[1].reg.type == REG_A
          ? (struct clocks){10, 0, 0}
          : (struct clocks){9, 0/*ea*/, 0};// TODO ea
      }
      if (instr->ops[0].type == OP_REG && instr->ops[1].type == OP_DATA) {
        return (struct clocks){4, 0, 0};
      }
      if (instr->ops[0].type == OP_EA && instr->ops[1].type == OP_DATA) {
        return (struct clocks){10, 0/*ea*/, 0}; // TODO
      }
      // TODO SEG REGs
      // TODO calc ea
      return (struct clocks){4, 0, 0};

    case INSTR_ADD:     return (struct clocks){4, 0, 0};
    case INSTR_SUB:     return (struct clocks){4, 0, 0};
    case INSTR_CMP:     return (struct clocks){4, 0, 0};
    case INSTR_JBE:     return (struct clocks){4, 0, 0};
    case INSTR_JNBE:    return (struct clocks){4, 0, 0};
    case INSTR_JB:      return (struct clocks){4, 0, 0};
    case INSTR_JNB:     return (struct clocks){4, 0, 0};
    case INSTR_JE:      return (struct clocks){4, 0, 0};
    case INSTR_JNE:     return (struct clocks){4, 0, 0};
    case INSTR_JG:      return (struct clocks){4, 0, 0};
    case INSTR_JNG:     return (struct clocks){4, 0, 0};
    case INSTR_JL:      return (struct clocks){4, 0, 0};
    case INSTR_JNL:     return (struct clocks){4, 0, 0};
    case INSTR_JO:      return (struct clocks){4, 0, 0};
    case INSTR_JNO:     return (struct clocks){4, 0, 0};
    case INSTR_JP:      return (struct clocks){4, 0, 0};
    case INSTR_JNP:     return (struct clocks){4, 0, 0};
    case INSTR_JS:      return (struct clocks){4, 0, 0};
    case INSTR_JNS:     return (struct clocks){4, 0, 0};
    case INSTR_JCXZ:    return (struct clocks){4, 0, 0};
    case INSTR_LOOP:    return (struct clocks){4, 0, 0};
    case INSTR_LOOPZ:   return (struct clocks){4, 0, 0};
    case INSTR_LOOPNZ:  return (struct clocks){4, 0, 0};
    default:
      fprintf(stderr,
          "Calculat clocks error: unhandled instruction '%s' of type %d\n",
          instr->str, instr->type);
  }
  return (struct clocks){0, 0, 0};
}
