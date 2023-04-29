#include "sim86_types.h"
#include "sim86_instr.h"
#include "sim86_simulate.h"

#include <stdio.h>      // printf

enum reg_type s_print_regs[] = {
  REG_A, REG_B, REG_C, REG_D, REG_SP, REG_BP, REG_SI, REG_DI,
};

void print_ea(const struct ea *ea, bool set_cs_addr, bool is_far) {
  if (is_far) {
    printf("far ");
  }

  if (set_cs_addr) {
    printf("%u", ea->direct_addr);
    return;
  }

  if (ea->seg) {
    printf("%s:", str_reg(*ea->seg));
  }
  if (ea->type == EA_DIRECT) {
    printf("[%u]", ea->direct_addr);
    return;
  }

  printf("[");
  const char *separator = "";
  if (ea->compute_addr.base_reg) {
    printf("%s", str_reg(*ea->compute_addr.base_reg));
    separator = " + ";
  }
  if (ea->compute_addr.index_reg) {
    printf("%s%s", separator, str_reg(*ea->compute_addr.index_reg));
  }
  if (ea->type == EA_COMPUTE && ea->compute_addr.displ != 0) {
    // print displacement with space after arithmetic operation character
    const char *op = ea->compute_addr.displ < 0 ? "-" : "+";
    u16 udispl = ea->compute_addr.displ < 0
      ? ~ea->compute_addr.displ + 1
      : ea->compute_addr.displ;
    printf(" %s %u", op, udispl);
  }
  printf("]");
}

void print_operand(const struct operand *op, const struct instr *instr) {
  switch (op->type) {
    case OP_NONE:
      break;
    case OP_REG:
      printf("%s", str_reg(op->reg));
      break;
    case OP_EA:
      print_ea(&op->ea, instr->set_cs_addr, instr->is_far);
      break;
    case OP_DATA:
      printf("%u", op->data);
      break;
    case OP_IP_INC: {
      printf("$%+d", op->ip_inc + instr->size);
    } break;
  }
}

void print_prefixes(const struct prefixes *prefixes) {
  if (prefixes->types & PREFIX_LOCK) {
    printf("lock ");
  }
  if (prefixes->types & PREFIX_REP) {
    printf("rep ");
  }
  if (prefixes->types & PREFIX_REPNE) {
    printf("repne ");
  }
}

void print_instr(const struct instr *instr) {
  const struct operand *op0 = &instr->ops[0];
  const struct operand *op1 = &instr->ops[1];

  if (instr->prefixes.types != PREFIX_NONE) {
    print_prefixes(&instr->prefixes);
  }

  printf("%s", instr->str);
  if (op0->type != OP_NONE) {
    printf(" ");
    if (instr->set_cs_addr) {
      printf("%u: ", instr->cs_addr);
    }
    // (byte|word) [memory]
    // No needed to print width prefix it if register was specified as
    // op1 explicitly.
    // For now don't distinguish between explicit and implicit registers
    // and always print width prefix
    if (op0->type == OP_EA) {
      printf("%s ", instr->w ? "word" : "byte");
    }
    print_operand(op0, instr);
  }
  if (op1->type != OP_NONE) {
    printf(", ");
    print_operand(op1, instr);
  }
}

void print_state_diff(const struct state *state_old,
    const struct state *state_new) {
  // diff not insluding segment registers
  for (u64 i = 0; i < ARRAY_COUNT(s_print_regs); ++i) {
    struct reg reg = {s_print_regs[i], REG_MODE_X};
    if (state_old->regs[i] != state_new->regs[i]) {
      printf("%s:0x%x->0x%x ",
        str_reg(reg),  state_old->regs[reg.type], state_new->regs[reg.type]);
    }
  }
}

void print_state_registers(const struct state *state) {
  // don't print segment registers
  for (u64 i = 0; i < ARRAY_COUNT(s_print_regs); ++i) {
    struct reg reg = {s_print_regs[i], REG_MODE_X};
    printf("      %s: 0x%04x (%u)\n",
        str_reg(reg),  state->regs[reg.type], state->regs[reg.type]);
  }
}
