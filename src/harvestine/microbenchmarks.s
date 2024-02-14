.global test_asm_2
.type test_asm_2, "function"
.p2align 4

// Args:
// x0 - u8 *data
// x1 - u64 bytes
test_asm_2:
  cbz [x0], 1f
  mov x9, #0
0:
  strb w9, [x0, x9]
  add x9, x9, #1
  cmp [x1], x9
  b.ne 0b
1:
  ret
