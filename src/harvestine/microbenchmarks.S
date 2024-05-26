# NOTE: Compile with 'gcc' driver (not with 'as'!) to get platform defines

#if defined(__aarch64__)

#if __APPLE__
.text

.global _test_mov_all_bytes
.global _test_nop_all_bytes

// Move all byte counter into all bytes of array
// Args:
// x0 - u8 *data
// x1 - u64 bytes
.p2align 4 // 16-byte align (8-byte required by macOS ABI)
_test_mov_all_bytes:
  cbz   x1, 2f
  mov   x9, #0
1: // loop
  strb  w9, [x0, x9]
  add   x9, x9, #1
  cmp   x1, x9
  b.ne  1b
2: // loop_end
  ret

// Nop over all bytes
// Args:
// x0 - u8 *data
// x1 - u64 bytes
.p2align 4
_test_nop_all_bytes:
  cbz   x1, 2f
  mov   x9, #0
1: // loop
  nop
  add   x9, x9, #1
  cmp   x1, x9
  b.ne  1b
2: // loop_end
  ret

#else
#error Not implemented
#endif // #if __APPLE__

#elif defined(__x86_64__)
.section .text
.intel_syntax noprefix

.global test_mov_all_bytes
.global test_nop_all_bytes

.type test_mov_all_bytes, "function"
.type test_nop_all_bytes, "function"

#if __linux__

// Move all byte counter into all bytes of array
// Args:
// rdi - u8 *data
// rsi - u64 bytes
// Move all byte counter into all bytes of array
.p2align 4
test_mov_all_bytes:
  test  rsi, rsi
  je    2f
  xor   eax, eax
1: // loop
  mov   byte ptr [rdi + rax], al
  inc   rax
  cmp   rsi, rax
  jne   1b
2: // loop_end
  ret

// Nop over all bytes
// Args:
// rdi - u8 *data
// rsi - u64 bytes
// Move all byte counter into all bytes of array
.p2align 4
test_nop_all_bytes:
  test  rsi, rsi
  je    2f
  xor   eax, eax
1: // loop
  .byte 0x0f, 0x1f, 0x00 // Byte sequence for a 3-byte NOP
  inc   rax
  cmp   rsi, rax
  jne   1b
2: // loop_end
  ret

#else
#error Not implemented
#endif // #if __linux__

#endif // #if defined(__aarch64__)

.end
