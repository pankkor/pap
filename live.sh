#!/bin/sh

help="""
USAGE
  live.sh <test> [test_arguments]

Simple live coding evnironment.Runs selected test on every binary modification.
Requirements:     'entr' utility installed.
Build directory:  './build'

Example
  live.sh --test sum
Would watch ./build/sum for modifications using 'entr' and rerun sum test when
it's modified.

OPTIONS
  -h, --help

TESTS
  sim86_decode
    Test 8086 instructions decode listings (see test.sh)

  sim86_simulate
    Test 8086 instructions simulate listings (see test.sh)

  sum
    Simple array sum benchmark.

  estimate_cpu_timer_freq [time_to_run_ms]
    Estimate cpu timer frequency.

  gen_harvestine <seed> <coord_pair_count>
    Generate harvestine distance json.

  harvestine <input_json>
    Calculate avg. of harvestine distances from coordinate pairs in input json.

  microbenchmark [benchmark]
    Performance benchmarks.

  pf_counter
    Test of Page Fault performance

  read_overhead <input_file>
    Repetiton test of file read performance

  cp_rect
    Interview Questions from 1994: rectangular copy

  str_copy
    Interview Questions from 1994: copy string

  has_color
    Interview Questions from 1994: find 2-bit pixel color in a byte

  draw_circle
    Interview Questions from 1994: draw circle outline (Bresenham's circle)
"""

case "$1" in
  -h | --help)
    echo "$help"
    ;;
  sim86_decode | sim86_simulate)
    cmd="./test.sh '$1'"
    echo 'build/sim86' | entr -cs "echo 'Running: $cmd'; $cmd"
    ;;
  estimate_cpu_timer_freq)
    time_to_run_ms="${2:-1000}"
    cmd="time ./build/$1 '$time_to_run_ms'"
    echo "build/$1" | entr -cs "echo 'Running: $cmd'; $cmd"
    ;;
  gen_harvestine)
    filename="build/harvestine_live"
    cmd="./build/$1 '$2' '$3' '$filename' && cat '$filename.json'"
    echo "build/$1" | entr -cs "echo 'Running: $cmd'; $cmd"
    ;;
  harvestine)
    filename=${2:-'build/harvestine_live'}
    cmd="time ./build/harvestine '$filename.json' '$filename.out.avg'"
    echo "build/$1" | entr -cs "echo 'Running: $cmd'; $cmd \
      && echo 'Average:  ' && cat '$filename.out.avg' \
      && echo 'Expected: ' && cat '$filename.avg' \
      && echo 'Average comparison: ' \
      && cmp '$filename.out.avg' '$filename.avg'"
    ;;
  read_overhead)
    filename=${2:-'build/harvestine_live.json'}
    cmd="./build/$1 '$filename'"
    echo "build/$1" | entr -cs "echo 'Running: $cmd'; $cmd"
    ;;
  # Simply forward arguments to test app
  # NOTE: Beware of non escaped args
  sum|cp_rect|str_cpy|has_color|draw_circle|microbenchmarks|pf_counter|\
    ptr_anatomy)
    cmd="./build/$@"
    echo "build/$1" | entr -cs "echo 'Running: $cmd'; $cmd"
    ;;
  *)
    echo "Error: unsupported test '$1'" >&2
    echo "$help" >&2
    exit 1
    ;;
esac
