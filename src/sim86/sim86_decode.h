#pragma once

#include "sim86_instr.h"

struct stream;

// Build index table for the first byte (b0)
// Call before calling decode_next_instr()
void decode_build_index(void);

// On error instr.str is NULL
struct instr decode_next_instr(struct stream *stream);
