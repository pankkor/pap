#!/bin/sh

help="""
Simple live coding evnironment.
Requirements: 'entr' utility installed. Build directory: './build'

OPTIONS
  -h, --help
  --build-and-run (sum|decode)
"""

case "$1" in
  -h | --help)
    echo "$help"
    ;;
  --build-and-run)
    case "$2" in
      sum)
        echo src/sum/sum.c | entr -cs './build.sh && ./build/sum'
        ;;
      sim86_decode)
        echo src/sim86/sim86.c | entr -cs './build.sh && ./test_decode.sh'
        ;;
      sim86)
        echo src/sim86/sim86.c | entr -cs './build.sh && ./test_simulate.sh'
        ;;
      *)
        echo "Error: unsupported target '$2'" >&2
        exit 1
        ;;
    esac
    ;;
  *)
    echo "$help" >&2
    exit 1
    ;;
esac
