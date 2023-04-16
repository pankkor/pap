// Performance-Aware-Programming Course
// https://www.computerenhance.com/p/table-of-contents
//
// Part 1
// 8086 simple simulator
//
// Run 'test_decode.sh' that tests decoding on all the listings in listings dir

#include "sim86_types.h"
#include "sim86_instr.h"

#include <stdio.h>      // printf

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
  printf("\n");
}
