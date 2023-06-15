#pragma once

#include "sim86_types.h"

struct instr;

// Clocks (cycles) estiamtion for 8086 or 8088
struct clocks {
  u32 base; // basic instruction clocks
  u32 ea;   // effective address calculation clocks
  u32 p;    // ??? clocks TODO
};

// Calculate cycles estiamtion for 8086 or 8088 instruction
// Set 'is_8088' to true to print cycles for 8088, otherwise cycles for 8086
struct clocks calc_clocks(const struct instr *instr, bool is_8088);
