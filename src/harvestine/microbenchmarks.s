# TODO
.ifdef __aarch64__

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
. ret
.elifdef __x86_64__

.global test_asm_2
.type test_asm_2, "function"
.p2align 4
.intel_syntax noprefix

// Args:
// rdi - u8 *data
// rsi - u64 bytes
test_asm_2:
  test    rsi, rsi
  je      .loop_end
  xor     eax, eax
.loop:
  mov     byte ptr [rdi + rax], al
  inc     rax
  cmp     rsi, rax
  jne     .loop
.loop_end:
  ret

.endif
