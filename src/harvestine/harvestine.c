// Performance-Aware-Programming Course
// https://www.computerenhance.com/p/table-of-contents
//
// Part 2
// Harvestine distance input file generator

// TODO:
// Performance roadmap

// * Baseline
//
// Stats
// <bench_stat>
//
// * Optimisation 1: eliminate buf deref checks
// At the moment I always check if we are still within [buf->data, buf->end)
// bounds:
//    while (w->cur < w->buf.end && is_whitespace(*w->cur)) ...
//
// after the check we deref current symbol.
//
// Allocate some extra space at the end of the buffer and fill it in with EOF.
// They we can deref past file content and just fail char or string comparison.
//
// Stats
// <bench_stat>
//
// * Optimization 2:
// Rewrite is_whitespace
// #define OPT_IS_WHITESPACE
//
// Stats
// <bench_stat>
//
// * Optimization 2:
// Rewrite is_pairs
// #define OPT_IS_PAIRS
//
// * Optimization 4:
// Rewrite strtod(). strtod() uses isspace() and acceses locale
//
// Stats
// <bench_stat>
//
// * Optimization 5:
// Filter out all the whitespaces
//
// Stats
// <bench_stat>

// Begin unity build
#include "timer.c"
#include "profiler.c"
// End unity build

#include "types.h"
#include "timer.h"
#include "calc_harvestine.h"

#include <stdio.h>      // printf fprintf fopen fread fseek ftell
#include <stdlib.h>     // malloc free strtod
#include <string.h>     // strncmp

struct buf_u8 {
  u8 *data;
  u8 *end;
};

// String view
struct sv {
  char *data;
  u32 size;
};

enum {COORDS_SIZE_MAX = 10 * 4 * 1024 * 1024};

struct coords {
  f64 data[COORDS_SIZE_MAX];
  u64 size;
};

// Predictive parser helper data
struct walk {
  struct buf_u8 buf;
  u8 *cur;
};

static struct coords s_coords;

// --------------------------------------
// File IO
// --------------------------------------

static struct buf_u8 alloc_buf_file_read(const char *filepath) {
  PROFILE_FUNC();

  struct buf_u8 ret = {0};
  u8 *buf = 0;
  u64 file_size = 0;

  FILE *f = fopen(filepath, "rb");
  if (!f) {
    perror("Error: fopen() failed");
    goto file_read_failed;
  }

  if (fseek(f, 0, SEEK_END)) {
    perror("Error: fseek() failed");
    goto file_read_failed;
  }

  file_size = ftell(f);
  if (fseek(f, 0, SEEK_SET)) {
    perror("Error: fseek() failed");
    goto file_read_failed;
  }

  buf = malloc(file_size);
  if (!buf) {
    perror("Error: malloc failed");
    goto file_read_failed;
  }

  if (fread(buf, 1, file_size, f) != file_size) {
    perror("Error: fread() failed");
    goto file_read_failed;
  }

  ret.data = buf;
  ret.end = buf + file_size;

file_read_cleanup:
  if (f) {
    fclose(f);
  }

  return ret;

file_read_failed:
  free(buf);
  goto file_read_cleanup;
}

// --------------------------------------
// Predictive Parser
// --------------------------------------

b32 is_whitespace(i32 c) {
#ifndef OPT_IS_WHITESPACE
  switch(c) {
    case ' ':
    case '\t':
    case '\r':
    case '\n':
    case '\v':
    case '\f':
    return 1;
  }
  return 0;
#else
  i32 m0 = c == '\n';
  i32 m1 = c == '\r';
  i32 m2 = c == ' ';
  i32 m3 = c == '\t';
  return m0 | m1 | m2 | m3;
#endif // #ifndef OPT_IS_WHITESPACE
}

void skip_whitespace(struct walk *w) {
  while (w->cur < w->buf.end && is_whitespace(*w->cur)) {
    ++w->cur;
  }
}

b32 accept_char(struct walk * restrict w, i32 c) {
  skip_whitespace(w);

  if (w->cur >= w->buf.end) {
    return 0;
  }

  if (*w->cur == c) {
    ++w->cur;
    return 1;
  }
  return 0;
}

b32 accept_sv(struct walk * restrict w, struct sv * restrict out_key) {
  skip_whitespace(w);

  if (!accept_char(w, '"')) {
    return 0;
  }

  u8 *cur = w->cur;
  u8 *end = w->buf.end;
  while (cur < end && *cur++ != '"') {
  }

  *out_key = (struct sv){(char *)w->cur, cur - w->cur - 1};
  if (w->cur != cur) {
    w->cur = cur;
    return 1;
  }
  return 0;
}

b32 accept_f64(struct walk * restrict w, f64 * restrict out_d) {
  skip_whitespace(w);

  u8 *end;
  f64 d = strtod((char *)w->cur, (char **)&end);
  if (end != w->cur) {
      w->cur = end;
      *out_d = d;
      return 1;
  }
  return 0;
}

b32 accept_any_to_char(struct walk * restrict w, i32 c) {
  skip_whitespace(w);

  u8 *cur = w->cur;
  u8 *end = w->buf.end;
  while (cur < end && *cur++ != c) {
  }

  if (w->cur != cur) {
    w->cur = cur;
    return 1;
  }
  return 0;
}

b32 expect_char(struct walk * restrict w, i32 c) {
  if (accept_char(w, c)) {
    return 1;
  }
  fprintf(stderr, "Perser error: expected '%c' at position %lu, got '%c'\n",
      c, w->cur - w->buf.data, *w->cur);
  return 0;
}

b32 expect_f64(struct walk * restrict w, f64 *d) {
  if (accept_f64(w, d)) {
    return 1;
  }
  fprintf(stderr, "Perser error: expected f64 at position %lu\n",
      w->cur - w->buf.data);
  return 0;
}

i32 key_to_coord_index(struct sv key) {
  // `k + c` should remain negative if either `k` or `c` is not initialized
  // in switch cases below
  i32 k = -32;
  i32 c = -32;
  if (key.size == 2) {
    switch (key.data[0]) {
      case 'x': c = 0; break;
      case 'y': c = 1; break;
    }
    switch (key.data[1]) {
      case '0': k = 0; break;
      case '1': k = 2; break;
    }
  }
  return k + c;
}

void print_error_unexpected_key_error(const struct walk *w, struct sv key) {
  i32 pos = w->cur - w->buf.data - key.size;
  fprintf(stderr, "Perser error: unexpected key \"%.*s\" at position %d.\n\n",
      key.size, key.data, pos);

  // print surrounding text
  u8 *w_l = CLAMP(w->cur - key.size - 32, w->buf.data, w->buf.end);
  u8 *w_r = CLAMP(w->cur - key.size + 48, w->buf.data, w->buf.end);

  // print only surrounding characters that don't mess with a carret position
  u8 *l = w->cur - key.size;
  u8 *r = w_r;
  while (l - 1 >= w_l) {
    i32 m0 = *l == '\t';
    i32 m1 = *l == '\r';
    i32 m2 = *l == '\n';
    i32 m3 = *l == '\v';
    if (m0 | m1 | m2 | m3) {
      goto l_loop_finished;
    }
    --l;
  }
l_loop_finished:

  fprintf(stderr, "    %*c\n", (int)(w->cur - key.size - l), '|');
  fprintf(stderr, "    %*c\n", (int)(w->cur - key.size - l), 'v');
  fprintf(stderr, "    %.*s\n\n", (int)(r - l) , l);
}

b32 is_pairs(struct sv sv) {
#ifndef OPT_IS_PAIRS
  return strncmp("pairs", (char *)sv.data, sv.size) == 0;
#else
  if (sv.size != 7) {
    return 0;
  }
  i32 m0 = sv.data[0] == '"';
  i32 m1 = sv.data[1] == 'p';
  i32 m2 = sv.data[2] == 'a';
  i32 m3 = sv.data[3] == 'i';
  i32 m4 = sv.data[4] == 'r';
  i32 m5 = sv.data[5] == 's';
  i32 m6 = sv.data[6] == '"';
  return m0 | m1 | m2 | m3 | m4 | m5 | m6;
#endif // #ifndef OPT_IS_PAIRS
}

// JSON predictive parser
b32 parse_coords_json(struct buf_u8 json_buf, struct coords *out_coords) {
  PROFILE_FUNC();

  u64 ret_coords_size = 0;
  struct sv key = {0};
  struct walk w = {json_buf, json_buf.data};

  out_coords->size = ret_coords_size;
  expect_char(&w, '{');

  accept_sv(&w, &key);
  if (strncmp("pairs", (char *)key.data, key.size) == 0) {
    expect_char(&w, ':');
    expect_char(&w, '[');

    while (!accept_char(&w, ']')) {

      expect_char(&w, '{');

      f64 coords[4] = {0};
      while (!accept_char(&w, '}')) {
        key.data = 0;
        key.size = 0;
        accept_sv(&w, &key);

        i32 coord_index = key_to_coord_index(key);
        if (coord_index < 0) {
          print_error_unexpected_key_error(&w, key);
          fprintf(stderr, "Expected keys \"x0\", \"y0\", \"x1\" or \"y1\"\n");
          return 0;
        } else {
          expect_char(&w, ':');
          expect_f64(&w, &coords[coord_index]);
          accept_char(&w, ',');
        }
      }
      accept_char(&w, ',');

      if (s_coords.size + 4 < COORDS_SIZE_MAX) {
        s_coords.data[s_coords.size + 0] = coords[0];
        s_coords.data[s_coords.size + 1] = coords[1];
        s_coords.data[s_coords.size + 2] = coords[2];
        s_coords.data[s_coords.size + 3] = coords[3];
        s_coords.size += 4;
      } else {
        fprintf(stderr, "Error: not enough memory to store coordinates\n");
        return 0;
      }
    }

  } else {
    print_error_unexpected_key_error(&w, key);
    fprintf(stderr, "Expected keys: \"pairs\"");
    return 0;
  }

  expect_char(&w, '}');
  return 1;
}

// --------------------------------------
// Sum
// --------------------------------------

f64 sum_harvestine_distances(const struct coords *coords) {
  PROFILE_FUNC();

  f64 ret = 0.0;

  for (u64 i = 0; i < coords->size - 3; i += 4) {
    ret += calc_harvestine(
        coords->data[i + 0],
        coords->data[i + 1],
        coords->data[i + 2],
        coords->data[i + 3],
        EARTH_RAD);
  }

  return ret;
}

// --------------------------------------
// Main
// --------------------------------------
static void print_usage(void) {
  fprintf(stderr, "Usage:\nharvestine <input_file>\n");
}

int main(int argc, char **argv) {
  if (argc < 2) {
    print_usage();
    return 1;
  }

  profile_begin();

  const char *filepath = argv[1];

  struct buf_u8 json_buf = alloc_buf_file_read(filepath);
  if (!json_buf.data) {
    fprintf(stderr, "Error: failed to read '%s'.\n", filepath);
    return 1;
  }


  b32 parsed = parse_coords_json(json_buf, &s_coords);
  free(json_buf.data);

  if (!parsed) {
    fprintf(stderr, "Error: failed to parse json file '%s'.\n", filepath);
    return 1;
  }

  f64 sum = sum_harvestine_distances(&s_coords);

  profile_end();
  profile_print_stats(get_or_estimate_cpu_timer_freq(300));

  printf("%.17f\n", sum);
  return 0;
}
