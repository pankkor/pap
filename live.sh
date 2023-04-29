#!/bin/sh

help="""
Simple live coding evnironment.
Requirements: 'entr' utility installed. Build directory: './build'

OPTIONS
  -h, --help
  --build-and-test (sum|sim86_decode|sim86_simulate)
"""

case "$1" in
  -h | --help)
    echo "$help"
    ;;
  --build-and-test)
    case "$2" in
      sum)
        echo build/sum | entr -cs './build.sh && ./build/sum'
        ;;
      sim86_decode | sim86_simulate)
        echo build/sim86|  entr -cs "./build.sh && ./test.sh $2"
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
