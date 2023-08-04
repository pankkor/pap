// Performance-Aware-Programming Course
// https://www.computerenhance.com/p/table-of-contents
//
// Microsoft interview questions from 1994
// Question 1: Rectangle copy

#include <stdio.h>  // printf

typedef unsigned char u8;
typedef int i32;
typedef unsigned int u32;

// Returns -1 for negative integers and +1 for positive integers
i32 sign(i32 a) {
  i32 s = a >> 31; // -1 or 0 (signed integers generate shift arithmetic right)
  return 1 | s;
}

i32 min_i32(i32 a, i32 b) {
  return a < b ? a : b;
}

i32 max_i32(i32 a, i32 b) {
  return a > b ? a : b;
}

i32 clamp_i32(i32 x, i32 l, i32 r) {
  return x <= l ? l : x >= r ? r : x;
}

// Perform rectangular copy of 1-D arrays in range
// [src_x0, src_x1], [src_y0, src_y1]
// Copy in direction of src_x0 -> src_x1, src_y0 -> src_y1
//
// Pitch - distance between element in one row and element in the same column,
// on the next row.
void cp_rect(
    u8 *src, i32 src_pitch, u8 *dst, i32 dst_pitch,
    i32 src_x0, i32 src_y0, i32 src_x1, i32 src_y1,
    i32 dst_x0, i32 dst_y0) {
  i32 src_min_x = clamp_i32(min_i32(src_x0, src_x1), 0, src_pitch);
  i32 src_max_x = clamp_i32(max_i32(src_x0, src_x1), 0, src_pitch);
  i32 src_min_y = max_i32(min_i32(src_y0, src_y1), 0);
  i32 src_max_y = max_i32(src_y0, src_y1);

  i32 w = src_max_x - src_min_x + 1;
  i32 h = src_max_y - src_min_y + 1;
  w = clamp_i32(dst_pitch - dst_x0, 0, w);

  i32 src_dx = sign(src_x1 - src_x0);
  i32 src_dy = sign(src_y1 - src_y0);
  i32 src_dpitch = src_dy * src_pitch - w * src_dx;
  i32 dst_dpitch = dst_pitch - w;

  u8 *s = src + src_y0 * src_pitch + src_x0;
  u8 *d = dst + dst_y0 * dst_pitch + dst_x0;
  for (i32 y = 0; y < h; ++y) {
    for (i32 x = 0; x < w; ++x) {
      *d = *s;
      s += src_dx;
      d += 1;
    }
    s += src_dpitch;
    d += dst_dpitch;
  }
}


void init_buf(u8 *buf, i32 w, i32 h) {
  for (i32 y = 0; y < h; ++y) {
    for (i32 x = 0; x < w; ++x) {
      buf[y * w + x] = y * w + x;
    }
  }
}

void init_buf_c(u8 *buf, i32 w, i32 h, u8 c) {
  for (i32 y = 0; y < h; ++y) {
    for (i32 x = 0; x < w; ++x) {
      buf[y * w + x] = c;
    }
  }
}

void print_buf(u8 *buf, u32 w, u32 h) {
  // Header:
  // ····01·02·03·04 ... w
  printf("    ");
  for (u32 i = 0; i < w; ++i) {
    printf("%2X ", i);
  }
  printf("\n----");
  for (u32 i = 0; i < w; ++i) {
    printf("---");
  }
  printf("\n");

  for (u32 y = 0; y < h; ++y) {
    printf("%2X: ", y);
    for (u32 x = 0; x < w; ++x) {
      u8 byte = buf[y * w + x];
      printf("%02X ", byte);
    }
    printf("\n");
  }
  printf("\n");
}

enum { BUF_A_W = 24, BUF_A_H = 24};
enum { BUF_B_W = 32, BUF_B_H = 16};
u8 s_buf_a[BUF_A_W * BUF_A_H];
u8 s_buf_b[BUF_B_W * BUF_B_H];

int main(void) {
  init_buf(s_buf_a, BUF_A_W, BUF_A_H);
  printf("Buffer A [%d x %d]\n", BUF_A_W, BUF_A_H);
  print_buf(s_buf_a, BUF_A_W, BUF_A_H);

  init_buf_c(s_buf_b, BUF_B_W, BUF_B_H, 0xBB);

  printf("Buffer B [%d x %d]\n", BUF_B_W, BUF_B_H);
  cp_rect(s_buf_a, BUF_A_W, s_buf_b, BUF_B_W,
      1, 2, 8, 9,
      4, 6);
  print_buf(s_buf_b, BUF_B_W, BUF_B_H);

  cp_rect(s_buf_a, BUF_A_W, s_buf_b, BUF_B_W,
      BUF_A_W - 1, 5, 0, 0,
      18, 1);

  print_buf(s_buf_b, BUF_B_W, BUF_B_H);

  cp_rect(s_buf_a, BUF_A_W, s_buf_b, BUF_B_W,
      5, 0, 0, 5,
      15, 10);

  print_buf(s_buf_b, BUF_B_W, BUF_B_H);

  cp_rect(s_buf_a, BUF_A_W, s_buf_b, BUF_B_W,
      0, 0, 128, BUF_B_H - 1,
      28, 0);
  print_buf(s_buf_b, BUF_B_W, BUF_B_H);

  return 0;
}
