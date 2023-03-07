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
        echo src/sum.c | entr -cs './build.sh && ./build/sum'
        ;;
      decode)
        echo src/decode.c | entr -cs './build.sh && ./test_decode.sh'
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
