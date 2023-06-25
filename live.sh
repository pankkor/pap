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
    Calculate sum of harvestine distances from coordinate pairs in input json.
"""

./build.sh

case "$1" in
  -h | --help)
    echo "$help"
    ;;
  --test)
    case "$2" in
      sim86_decode | sim86_simulate)
        echo build/sim86 | entr -cs "./test.sh $2"
        ;;
      sum)
        echo build/sum | entr -cs './build/sum'
        ;;
      estimate_cpu_timer_freq)
        time_to_run_ms="${3:-1000}"
        echo build/estimate_cpu_timer_freq \
          | entr -cs "time ./build/estimate_cpu_timer_freq $time_to_run_ms"
        ;;
      gen_harvestine)
        echo build/gen_harvestine \
          | entr -cs "./build/gen_harvestine $3 $4 > build/harvestine_live.json\
          2> build/harvestine_live.sum && cat build/harvestine_live.json"
        ;;
      harvestine)
        echo build/harvestine \
          | entr -cs "./build/harvestine build/harvestine_live.json > \
          build/harvestine_live.out.sum \
          && echo 'Sum:      ' && cat build/harvestine_live.out.sum \
          && echo 'Expected: ' && cat build/harvestine_live.sum \
          && echo 'Sums comparison: ' \
          && cmp build/harvestine_live.out.sum build/harvestine_live.sum"
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
