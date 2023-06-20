// Performance-Aware-Programming Course
// https://www.computerenhance.com/p/table-of-contents
//
// Part 2
// Harvestine distance input file generator


// Begin unity build
#include "calc_harvestine.c"
// End unity build

#include "calc_harvestine.h"

#include "types.h"

#include <stdio.h>      // printf fprintf fopen fwrite
#include <stdlib.h>     // atol rand

static void print_usage(void) {
  fprintf(stderr,
      "Usage:\ngen_harvestine <seed> <coord_pair_count>\n"
      "\n"
      "Prints JSON with coordinates to stdout and expected sum of harvestine\n"
      "distances to stderr.\n"
      );
}

static f64 rand_range(f64 min, f64 max) {
  u32 x = rand(); // :)
  f64 k = (f64)x / RAND_MAX;
  return k * (max - min) + min;
}

int main(int argc, char **argv) {
  if (argc < 3) {
    print_usage();
    return 1;
  }

  i64 seed = atol(argv[1]);
  i64 coord_pair_count = atol(argv[2]);

  srand((u32)seed);

  // If we just pick random points on the sphere final harvestine distance sum
  // converges to some value.
  // Better randomize final harvestine sum by picking points
  // using in 2 random clusters.
  enum {CLUSTERS_COUNT = 2};

  f64 x_min = -180.0 / CLUSTERS_COUNT;
  f64 x_max =  180.0 / CLUSTERS_COUNT;
  f64 y_min =  -90.0 / CLUSTERS_COUNT;
  f64 y_max =   90.0 / CLUSTERS_COUNT;

  f64 clusters[CLUSTERS_COUNT][2] = {
    {rand_range(x_min, x_max), rand_range(y_min, y_max)},
    {rand_range(x_min, x_max), rand_range(y_min, y_max)},
  };

  printf("{\"pairs\":[");

  f64 sum = 0.0;
  if (coord_pair_count > 0) {
    for (i64 i = 0; i < coord_pair_count - 1; ++i) {
      i32 idx0 = rand() % CLUSTERS_COUNT;
      i32 idx1 = (idx0 + 1) % CLUSTERS_COUNT;
      f64 x0 = rand_range(x_min, x_max) + clusters[idx0][0];
      f64 y0 = rand_range(y_min, y_max) + clusters[idx0][1];
      f64 x1 = rand_range(x_min, x_max) + clusters[idx1][0];
      f64 y1 = rand_range(y_min, y_max) + clusters[idx1][1];
      printf("{\"x0\":%f,\"y0\":%f,\"x1\":%f,\"y1\":%f},", x0, y0, x1, y1);

      sum += calc_harvestine(x0, y0, x1, y1, 6372.8);
    }

    {
      i32 idx0 = rand() % CLUSTERS_COUNT;
      i32 idx1 = (idx0 + 1) % CLUSTERS_COUNT;
      f64 x0 = rand_range(x_min, x_max) + clusters[idx0][0];
      f64 y0 = rand_range(y_min, y_max) + clusters[idx0][1];
      f64 x1 = rand_range(x_min, x_max) + clusters[idx1][0];
      f64 y1 = rand_range(y_min, y_max) + clusters[idx1][1];
      printf("{\"x0\":%f,\"y0\":%f,\"x1\":%f,\"y1\":%f}", x0, y0, x1, y1);

      sum += calc_harvestine(x0, y0, x1, y1, 6372.8);
    }
  }
  printf("]}\n");

  // print harvestine distance sum to stderr
  fprintf(stderr, "%f\n", sum);

  return 0;
}
