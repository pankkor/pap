// Performance-Aware-Programming Course
// https://www.computerenhance.com/p/table-of-contents
//
// Microsoft interview questions from 1994
// Question 4: Draw circle outline

#include <stdio.h>      // printf
#include <unistd.h>     // usleep

typedef unsigned char u8;
typedef unsigned int u32;
typedef int i32;

// Helper function to put a "pixel" on the screen
// In this implementation we just put a symbol to stdout
void plot(i32 x, i32 y, u32 rgb) {
  u8 r = (rgb & 0xFF0000) >> 16;
  u8 g = (rgb & 0x00FF00) >> 8;
  u8 b = (rgb & 0x0000FF) >> 0;

  // x axis scale: 2
  printf("\x1b[%d;%dH", y, x * 2); // move cursor
  printf("\x1b[48;2;%u;%u;%um \x1b[0m", r, g, b) ;
  printf("\x1b[%d;%dH", y, x * 2 + 1); // move cursor
  printf("\x1b[48;2;%u;%u;%um \x1b[0m", r, g, b) ;
}

void draw_circle(i32 cx, i32 cy, i32 r, u32 rgb) {
  // Bresenham’s circle drawing algorithm.
  // Drawing segment from 0 to pi/4, reflect for the other segments.
  // If drawing from 0 to pi/4 every next pixel's y coord increases every step.
  // x coord can either A) decrement x by 1 or B) keep it the same as in
  // previous iteration.
  // To determine wather we need to decrease x or not calculate an error func.
  //   x^2 + y^2 - r^2 = 0 is true for all the points on the circle.
  // Therefore an error function will be:
  //   f(x,y) = |x^2 + y^2 - r^2| is a distance from the circle to {x, y}
  // Calculate f(x,y) for case A and B and compare the result.
  // This would determine if we decrement x or not.
  // After simplification we get
  //   -2x^2 + 2x -2y^2 + C < 0, where C = 2r^2 - 1
  // (For derivation see `draw_circle_derivation.jpeg`)
  // C is a constant that could be precalculated.
  // If this inequality is true then we shall decrease x by 1.
  // However it's too complicated to be efficiently calculated on old CPUs.
  // Let's try to simplifty it by using Digital Differential Analysis.
  // Let
  //   D(x, y)               = -2x^2 + 2x - 2y^2 + C
  //   D(x, y + 1)           = -2x^2 + 2x - 2y^2 - 4y - 2 + C
  //   D(x, y + 1) - D(x, y) = -4y - 2, call it Dy (when y increments)
  // We can apply Digital Differential Analysis one more time to remove
  // multiplication (or a shift operation, which performance on older CPU scales
  // with shift operand.
  //   E(x, y)               = -4y - 2
  //   E(x, y + 1)           = -4y - 6
  //   E(x, y + 1) - E(x, y) = -4, call it DY (y delta of Dy)
  // Do the same for
  //   D(x, y)               = -2x^2 + 2x - 2y^2 + C
  //   D(x - 1, y)           = -2x^2 + 6x - 4 - 2^y^2 + C
  //   D(x - 1, y) - D(x, y) = 4x - 4, let's call it Dx (when x decrements)
  // Apply Digital Differential Analysis one more time for Dx
  //   F(x, y)               = 4x - 4
  //   F(x - 1, y)           = 4x - 8
  //   F(x - 1, y) - F(x, y) = -4, call it DX (x delta of Dy)
  //
  // Algorithm
  // 0. We initialize x, y, d, dy and dx
  // 1. We draw points using symmetry
  // 2. We increase D by DY and we calculate new DY:            dy -= 4
  // 3. If D < 0 we also increase D by DX and calculate new DX: dx -= 4
  // 4. We increment y
  //
  // Initial values x = r, y = 0
  // Therefore initial
  //   D  = 2r - 1
  //   Dy = -2
  //   Dx = 4r - 4
  // Since we already have calculated 2r for D we can use it to calculate 4r
  //   2r = r + r
  //   4r = 2r + 2r

  i32 two_r = r + r;
  i32 x = r;
  i32 y = 0;
  i32 d = two_r - 1;
  i32 dy = -2;
  i32 dx = two_r + two_r - 4;

  plot(cx - x, cy, rgb);
  plot(cx + x, cy, rgb);
  plot(cx,     cy - x, rgb);
  plot(cx,     cy + x, rgb);

  while (y <= x) {
    d += dy;
    dy -= 4;
    ++y;

    if (d < 0) {
      d += dx;
      dx -= 4;
      --x;
    }

    plot(cx + x, cy + y, rgb);
    plot(cx + x, cy - y, rgb);
    plot(cx - x, cy + y, rgb);
    plot(cx - x, cy - y, rgb);
    plot(cx + y, cy + x, rgb);
    plot(cx + y, cy - x, rgb);
    plot(cx - y, cy + x, rgb);
    plot(cx - y, cy - x, rgb);
  }
}

int main(void) {
  // color background
  for (i32 y = 0; y < 255; ++y) {
    for (i32 x = 0; x < 255; ++x) {
      plot(x, y, 0x000000);
    }
  }

  // draw some circles
  i32 cx;
  i32 cy;
  u32 rgb = 0x0000AA;
  u32 n = 0;
  while (1) {
    // move circle center
    cx = 20 + n * 13 % 11;
    cy = 23 + n * 11 % 7;

    // change the color
    rgb |= 0x0000AA;
    rgb <<= n % 24;
    ++n;

    for (i32 r = 2; r < 20; r += 1) {
      // draw circles
      draw_circle(cx, cy, r,      rgb >> 1);
      draw_circle(cx, cy, r >> 1, rgb);
      draw_circle(cx, cy, r >> 2, rgb << 1);

      // move cursos away from the circle
      printf("\x1b[0;0H");

      fflush(stdout);
      usleep(50000);

      // clear circles
      draw_circle(cx, cy, r,      0x0);
      draw_circle(cx, cy, r >> 1, 0x0);
      draw_circle(cx, cy, r >> 2, 0x0);
    }
  }

  // move cursor to the bottom
  printf("\x1b[999;0H");
  return 0;
}
