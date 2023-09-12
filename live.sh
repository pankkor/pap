#!/bin/sh

help="""
Simple live coding evnironment.Runs selected test on every binary modification.
Requirements:     'entr' utility installed.
Build directory:  './build'

Example:
  live.sh --test sum
Would watch ./build/sum for modifications using 'entr' and rerun sum test when
it's modified.

OPTIONS
  -h, --help
  --test <test> [test] [test_options]

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

  read_overhead <input_file>
    Test of file read performance

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
  --test)
    case "$2" in
      sim86_decode | sim86_simulate)
        cmd="./test.sh $2"
        echo 'build/sim86' | entr -cs "echo 'Running: $cmd'; $cmd"
        ;;
      estimate_cpu_timer_freq)
        time_to_run_ms="${3:-1000}"
        cmd="time ./build/$2 $time_to_run_ms"
        echo "build/$2" | entr -cs "echo 'Running: $cmd'; $cmd"
        ;;
      gen_harvestine)
        filename=build/harvestine_live
        cmd="./build/$2 $3 $4 $filename && cat $filename.json"
        echo "build/$2" | entr -cs "echo 'Running: $cmd'; $cmd"
        ;;
      harvestine)
        filename=${3:-'build/harvestine_live'}
        cmd="time ./build/harvestine $filename.json $filename.out.avg"
        echo "build/$2" | entr -cs "echo 'Running: $cmd'; $cmd \
          && echo 'Average:  ' && cat $filename.out.avg \
          && echo 'Expected: ' && cat $filename.avg \
          && echo 'Average comparison: ' \
          && cmp build/harvestine_live.out.avg build/harvestine_live.avg"
        ;;
      read_overhead)
        filename=${3:-'build/harvestine_live.json'}
        cmd="./build/$2 $filename"
        echo "build/$2" | entr -cs "echo 'Running: $cmd'; $cmd"
        ;;
      sum|cp_rect|str_cpy|has_color|draw_circle)
        cmd="./build/$2"
        echo "build/$2" | entr -cs "echo 'Running: $cmd'; $cmd"
        ;;
      *)
        echo "Error: unsupported target '$2'" >&2
        echo "$help" >&2
        exit 1
        ;;
    esac
    ;;
  *)
    echo "Error: unsupported command '$1'" >&2
    echo "$help" >&2
    exit 1
    ;;
esac
