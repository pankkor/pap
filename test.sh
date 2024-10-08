#!/bin/sh

listings_dir='src/sim86/listings'

help="
Test script

OPTIONS
  -h, --help
  --sim86_decode    test 8086 instructions decode listings
  --sim86_simulate  test 8086 instructions simulate listings
"

sim86_decode=1
sim86_simulate=1

case "$1" in
  -h | --help)
    echo "$help"
    ;;
  sim86_decode)
    sim86_simulate=0
    ;;
  sim86_simulate)
    sim86_decode=0
    ;;
esac

if ! command -v nasm > /dev/null; then
    echo "Error: 'nasm' not found"
    exit 1
fi

if [ $sim86_decode -gt 0 ]; then
  echo ''
  echo '–––––––––––––––––––––––––––––––'
  echo 'sim86 sim86_decode listings'
  echo '–––––––––––––––––––––––––––––––'
  for asm_in in "$listings_dir"/*.asm; do
    basename="${asm_in##*/}"
    basename_wo_ext="${basename%.*}"

    bin_in="build/$basename_wo_ext.in"
    bin_out="build/$basename_wo_ext.out"
    asm_out="build/$basename.out.asm"

    echo "• $asm_in"
    (
      nasm "$asm_in" -o "$bin_in" || exit 1
      build/sim86 decode "$bin_in" > "$asm_out" || exit 1
      nasm "$asm_out" -o "$bin_out" || exit 1
      cmp "$bin_in" "$bin_out" || exit 1
    ) && echo '  ✅ passed' || {
      echo '\n––– Expected assembly –––'
      cat "$asm_in"
      echo '\n––– Produced assembly –––'
      cat "$asm_out"
      echo '\n––– Diff command –––'
      echo "diff '$asm_in' '$asm_out'"
      echo '\n––– Command –––'
      echo "build/sim86 sim86_decode '$bin_in'"
      echo '  ❌ failed'
      exit 1
    }
  done
fi

if [ $sim86_simulate -gt 0 ]; then
  echo ''
  echo '–––––––––––––––––––––––––––––––'
  echo "sim86 sim86_simulate listings"
  echo '–––––––––––––––––––––––––––––––'
  # simulation listings have *.txt with expected output
  for sim_in in "$listings_dir"/*.txt; do
    basename="${sim_in##*/}"
    basename_wo_ext="${basename%.*}"

    asm_in="$listings_dir/$basename_wo_ext.asm"
    bin_in="build/$basename_wo_ext"
    sim_out="build/$basename_wo_ext.out.txt"

    # Casey's listing_0048 onwards print IP register simulation results.
    # For listing before 0048 provide a special argument to sim86 binary
    # in order to skip printing IP register changes
    args=
    clocks=0
    case "$basename" in
      listing_0043_*|listing_0044_*|listing_0045_*|listing_0046_*|listing_0047_*)
        args='--print-no-ip'
        ;;
      listing_0056_*|listing_0057_*|listing_0058_*|listing_0059_*|listing_0060_*\
        |listing_0061_*|listing_0062_*|listing_0063*|listing_0064_*)
        clocks=1
        ;;
    esac

    echo "• $asm_in"
    (
      nasm "$asm_in" -o "$bin_in" || exit 1
      if [ "$clocks" -eq 0 ]; then
        echo "--- test\\$basename_wo_ext execution ---" > "$sim_out" || exit 1
        build/sim86 simulate $args "$bin_in" >> "$sim_out" || exit 1
      else
        (
          echo '**************'
          echo '**** 8086 ****'
          echo '**************'
          echo
          echo 'WARNING: Clocks reported by this utility are strictly from the 8086 manual.
They will be inaccurate, both because the manual clocks are estimates, and because
some of the entries in the manual look highly suspicious and are probably typos.'
          echo
          echo "--- test\\$basename_wo_ext execution ---"
          build/sim86 simulate $args --print-clocks-8086 "$bin_in" || exit 1
          echo
          echo '**************'
          echo '**** 8088 ****'
          echo '**************'
          echo
          echo 'WARNING: Clocks reported by this utility are strictly from the 8086 manual.
They will be inaccurate, both because the manual clocks are estimates, and because
some of the entries in the manual look highly suspicious and are probably typos.'
          echo
          echo "--- test\\$basename_wo_ext execution ---"
          build/sim86 simulate $args --print-clocks-8088 "$bin_in" || exit 1
        ) > "$sim_out" || exit 1
      fi
      diff -w "$sim_in" "$sim_out" > /dev/null || exit 1
    ) && echo '  ✅ passed' || {
      echo '\n––– Diff –––'
      echo "diff '$sim_in' '$sim_out'"
      echo ''
      diff -w -d -y --color=auto -b -W 108 "$sim_in" "$sim_out"
      echo '\n––– Command –––'
      echo "build/sim86 '$bin_in'"
      echo '  ❌ failed'
      exit 1
    }
  done
  echo ''
fi
