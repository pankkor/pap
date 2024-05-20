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
  cbz x1, 2f
  mov x9, #0
1: // loop
  strb w9, [x0, x9]
  add x9, x9, #1
  cmp x1, x9
  b.ne 1b
2: // loop_end
  ret

// Move all byte counter into all bytes of array
// Version 2
// Args:
// x0 - u8 *data
// x1 - u64 bytes
.p2align 4
_test_nop_all_bytes:
  cbz x1, 2f
  mov x9, #0
1: // loop
  nop
  add x9, x9, #1
  cmp x1, x9
  b.ne 1b
2: // loop_end
  ret

#else
#error Not implemented
#endif // #if __APPLE__

#elif defined(__x86_64__)
.intel_syntax noprefix

.global test_mov_all_bytes
.type test_mov_all_bytes, "function"

#if __linux__

// Args:
// rdi - u8 *data
// rsi - u64 bytes
// Move all byte counter into all bytes of array
// No version 2 for x86_64 provided
.p2align 4
test_mov_all_bytes:
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

#else
#error Not implemented
#endif // #if __linux__

#endif // #if defined(__aarch64__)

.end
