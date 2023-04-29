#pragma once

#include "sim86_instr.h"

struct stream;

// Build index table for the first byte (b0)
void decode_build_index();

// On error instr.str is NULL
struct instr decode_next_instr(struct stream *stream);
