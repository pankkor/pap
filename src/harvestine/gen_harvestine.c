// Performance-Aware-Programming Course
// https://www.computerenhance.com/p/table-of-contents
//
// Part 2
// Harvestine distance input file generator

#include "types.h"
#include "calc_harvestine.h"

#include <stdio.h>      // printf fprintf fopen fwrite snprintf
#include <stdlib.h>     // atol rand

static void print_usage(void) {
  fprintf(stderr,
      "Generate files with latitude longitude coordinate pairs.\n"
      "    <filename>.json    - JSON file with random coordinates.\n"
      "    <filename>.avg     - verification file with average of distances\n"
      "    <filename>.dists   - verification file with distances per pair\n"
      "\n"
      "Usage:\n"
      "    gen_harvestine <seed> <coord_pair_count> <filename>\n"
      );
}

static f64 rand_range(f64 min, f64 max) {
  u32 x = rand(); // :)
  f64 k = (f64)x / RAND_MAX;
  return k * (max - min) + min;
}

static FILE *file_open(const char *filename, const char *ext) {
    char temp[1024];
    snprintf(temp, sizeof(temp), "%s%s", filename, ext);
    FILE *f = fopen(temp, "wb");
    if (!f) {
      fprintf(stderr, "Error: failed to open file '%s'", temp);
      perror("");
    }
    return f;
}

int main(int argc, char **argv) {
  if (argc < 4) {
    print_usage();
    return 1;
  }

  i64 seed = atol(argv[1]);
  i64 coord_pair_count = atol(argv[2]);
  const char *filename = argv[3];

  FILE *out_json  = file_open(filename, ".json");
  FILE *out_avg   = file_open(filename, ".avg");
  FILE *out_dists = file_open(filename, ".dists");
  if (!out_json || !out_avg || ! out_dists) {
    return 1;
  }

  srand((u32)seed); // TODO: use better rand

  // If we just pick random points on the sphere final harvestine distance
  // average converges to one value that is stable over different seeds.
  // Better randomize final harvestine average by picking points
  // using 2 random clusters.
  enum {CLUSTERS_COUNT = 2};

  f64 x_min = -180.0 / CLUSTERS_COUNT;
  f64 x_max =  180.0 / CLUSTERS_COUNT;
  f64 y_min =  -90.0 / CLUSTERS_COUNT;
  f64 y_max =   90.0 / CLUSTERS_COUNT;

  f64 clusters[CLUSTERS_COUNT][2] = {
    {rand_range(x_min, x_max), rand_range(y_min, y_max)},
    {rand_range(x_min, x_max), rand_range(y_min, y_max)},
  };

  fprintf(out_json, "{\n  \"pairs\": [\n");

  f64 avg = 0.0;
  f64 avg_k = 1.0 / coord_pair_count;
  for (i64 i = 0; i < coord_pair_count; ++i) {
    i32 idx0 = rand() % CLUSTERS_COUNT;
    i32 idx1 = (idx0 + 1) % CLUSTERS_COUNT;
    f64 x0 = rand_range(x_min, x_max) + clusters[idx0][0];
    f64 y0 = rand_range(y_min, y_max) + clusters[idx0][1];
    f64 x1 = rand_range(x_min, x_max) + clusters[idx1][0];
    f64 y1 = rand_range(y_min, y_max) + clusters[idx1][1];
    f64 dist = calc_harvestine(x0, y0, x1, y1, EARTH_RAD);
    avg += dist * avg_k;

    const char *separator = i == coord_pair_count - 1 ? "\n" : ",\n";
    fprintf(out_json, "    {\"x0\": %.17f, \"y0\": %.17f, "
        "\"x1\": %.17f, \"y1\": %.17f}%s", x0, y0, x1, y1, separator);
    fprintf(out_dists, "%f\n", dist);
  }
  fprintf(out_json, "  ]\n}\n");

  fprintf(out_avg, "%.17f\n", avg);

  fclose(out_json);
  fclose(out_dists);
  fclose(out_avg);

  return 0;
}
