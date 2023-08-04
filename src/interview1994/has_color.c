// Performance-Aware-Programming Course
// https://www.computerenhance.com/p/table-of-contents
//
// Microsoft interview questions from 1994
// Question 3: in CGA mode pixel color can be any of 4 pallete colors (2-bits).
// Therefore a byte can contain 4 colors.
// Given a byte of 4 2-bit colors, check if any of them contains a given 2-bit
// color.

#include <stdio.h>
#include <assert.h>

typedef unsigned char u8;
typedef int b32;

// Inverted 2-bit color patterns
// Index: 0, 2-bit color: 00, pattern: 00000000, inverted pattern: 11111111
// Index: 1, 2-bit color: 01, pattern: 01010101, inverted pattern: 10101010
// Index: 2, 2-bit color: 10, pattern: 10101010, inverted pattern: 01010101
// Index: 3, 2-bit color: 11, pattern: 11111111, inverted pattern: 00000000
u8 s_not_cols[4] = {0xFF, 0xAA, 0x55, 0x00};

// Given a byte of 4 2-bit colors check if any of them contain a 2-bit color.
b32 has_color(u8 col4, u8 col) {
  u8 p4 = s_not_cols[col & 3];  // repeat inverted col in every 2-bit pixel

  u8 v0 = col4 ^ p4;       // 2-bit colors that match inverted pattern are 11
  u8 v1 = v0 & 0xAA;       // check every high bit of 2-bit color of v0
  u8 v2 = v0 & (v1 >> 1);  // AND check for every low bit of 2-bit color of v0
  return v2 != 0;
}

int main(void) {
  {
    u8 col4 = 0xE4;
    for (u8 col = 0; col < 4; ++col) {
      b32 r = has_color(col4, col);
      printf("Colors: '%02X' contains '%X' color: %s\n",
          col4, col, r ? "true" : "false");
      assert(r == 1);
    }
  }
  {
    u8 col4 = 0x02;
    u8 col = 0x02;
    b32 r = has_color(col4, col);
    printf("Colors: '%02X' contains '%X' color: %s\n",
        col4, col, r ? "true" : "false");
    assert(r == 1);
  }
  {
    u8 col4 = 0x08;
    u8 col = 0x02;
    b32 r = has_color(col4, col);
    printf("Colors: '%02X' contains '%X' color: %s\n",
        col4, col, r ? "true" : "false");
    assert(r == 1);
  }
  {
    u8 col4 = 0x20;
    u8 col = 0x02;
    b32 r = has_color(col4, col);
    printf("Colors: '%02X' contains '%X' color: %s\n",
        col4, col, r ? "true" : "false");
    assert(r == 1);
  }
  {
    u8 col4 = 0x80;
    u8 col = 0x02;
    b32 r = has_color(col4, col);
    printf("Colors: '%02X' contains '%X' color: %s\n",
        col4, col, r ? "true" : "false");
    assert(r == 1);
  }
  {
    u8 col4 = 0x0F;
    u8 col = 0x02;
    b32 r = has_color(col4, col);
    printf("Colors: '%02X' contains '%X' color: %s\n",
        col4, col, r ? "true" : "false");
    assert(r == 0);
  }
  {
    u8 col4 = 0xF0;
    u8 col = 0x02;
    b32 r = has_color(col4, col);
    printf("Colors: '%02X' contains '%X' color: %s\n",
        col4, col, r ? "true" : "false");
    assert(r == 0);
  }
  {
    u8 col4 = 0xFF;
    u8 col = 0x02;
    b32 r = has_color(col4, col);
    printf("Colors: '%02X' contains '%X' color: %s\n",
        col4, col, r ? "true" : "false");
    assert(r == 0);
  }
  {
    u8 col4 = 0x06;
    u8 col = 0x02;
    b32 r = has_color(col4, col);
    printf("Colors: '%02X' contains '%X' color: %s\n",
        col4, col, r ? "true" : "false");
    assert(r == 1);
  }
  {
    u8 col4 = 0x60;
    u8 col = 0x02;
    b32 r = has_color(col4, col);
    printf("Colors: '%02X' contains '%X' color: %s\n",
        col4, col, r ? "true" : "false");
    assert(r == 1);
  }
  {
    u8 col4 = 0x66;
    u8 col = 0x02;
    b32 r = has_color(col4, col);
    printf("Colors: '%02X' contains '%X' color: %s\n",
        col4, col, r ? "true" : "false");
    assert(r == 1);
  }
  {
    u8 col4 = 0x05;
    u8 col = 0x02;
    b32 r = has_color(col4, col);
    printf("Colors: '%02X' contains '%X' color: %s\n",
        col4, col, r ? "true" : "false");
    assert(r == 0);
  }
  {
    u8 col4 = 0x50;
    u8 col = 0x02;
    b32 r = has_color(col4, col);
    printf("Colors: '%02X' contains '%X' color: %s\n",
        col4, col, r ? "true" : "false");
    assert(r == 0);
  }
  {
    u8 col4 = 0x55;
    u8 col = 0x02;
    b32 r = has_color(col4, col);
    printf("Colors: '%02X' contains '%X' color: %s\n",
        col4, col, r ? "true" : "false");
    assert(r == 0);
  }
  {
    u8 col4 = 0x03;
    u8 col = 0x03;
    b32 r = has_color(col4, col);
    printf("Colors: '%02X' contains '%X' color: %s\n",
        col4, col, r ? "true" : "false");
    assert(r == 1);
  }
  {
    u8 col4 = 0x0C;
    u8 col = 0x03;
    b32 r = has_color(col4, col);
    printf("Colors: '%02X' contains '%X' color: %s\n",
        col4, col, r ? "true" : "false");
    assert(r == 1);
  }
  {
    u8 col4 = 0x30;
    u8 col = 0x03;
    b32 r = has_color(col4, col);
    printf("Colors: '%02X' contains '%X' color: %s\n",
        col4, col, r ? "true" : "false");
    assert(r == 1);
  }
  {
    u8 col4 = 0xC0;
    u8 col = 0x03;
    b32 r = has_color(col4, col);
    printf("Colors: '%02X' contains '%X' color: %s\n",
        col4, col, r ? "true" : "false");
    assert(r == 1);
  }
  {
    u8 col4 = 0xFF;
    u8 col = 0x03;
    b32 r = has_color(col4, col);
    printf("Colors: '%02X' contains '%X' color: %s\n",
        col4, col, r ? "true" : "false");
    assert(r == 1);
  }
  {
    u8 col4 = 0x00;
    u8 col = 0x03;
    b32 r = has_color(col4, col);
    printf("Colors: '%02X' contains '%X' color: %s\n",
        col4, col, r ? "true" : "false");
    assert(r == 0);
  }
  {
    u8 col4 = 0x55;
    u8 col = 0x03;
    b32 r = has_color(col4, col);
    printf("Colors: '%02X' contains '%X' color: %s\n",
        col4, col, r ? "true" : "false");
    assert(r == 0);
  }
  {
    u8 col4 = 0xFF;
    u8 col = 0x02;
    b32 r = has_color(col4, col);
    printf("Colors: '%02X' contains '%X' color: %s\n",
        col4, col, r ? "true" : "false");
    assert(r == 0);
  }
  {
    u8 col4 = 0xFF;
    u8 col = 0x00;
    b32 r = has_color(col4, col);
    printf("Colors: '%02X' contains '%X' color: %s\n",
        col4, col, r ? "true" : "false");
    assert(r == 0);
  }
  return 0;
}
