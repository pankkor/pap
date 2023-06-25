#pragma once

#include "types.h"

#include <math.h>       // sin cos asin sqrt

#define EARTH_RAD 6372.8

static FORCE_INLINE f64 sqr(f64 v) {
  return v * v;
}

static FORCE_INLINE f64 deg2rad(f64 deg) {
  return 0.01745329251994329577f * deg;
}

static inline f64 calc_harvestine(f64 x0, f64 y0, f64 x1, f64 y1,
    f64 earth_rad) {
  f64 lat0 = y0;
  f64 lat1 = y1;
  f64 lon0 = x0;
  f64 lon1 = x1;

  f64 dlat = deg2rad(lat1 - lat0);
  f64 dlon = deg2rad(lon1 - lon0);
  lat0 = deg2rad(lat0);
  lat1 = deg2rad(lat1);

  f64 a = sqr(sin(dlat / 2.0)) + cos(lat0) * cos(lat1) * sqr(sin(dlon / 2.0));
  f64 c = 2.0 * asin(sqrt(a));
  return earth_rad * c;
}
