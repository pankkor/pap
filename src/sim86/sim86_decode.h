// Performance-Aware-Programming Course
// https://www.computerenhance.com/p/table-of-contents
//
// Part 1
// 8086 simple simulator
//
// Run 'test_decode.sh' that tests decoding on all the listings in listings dir

#pragma once

#include "sim86_instr.h"

struct stream;

// Build index table for the first byte (b0)
void decode_build_index();

// On error instr.str is NULL
struct instr decode_next_instr(struct stream *stream);
