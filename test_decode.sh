#!/bin/sh

echo "Tests listings"

for asm_in in listings/*.asm; do
  basename="${asm_in##*/}"
  basename_wo_ext="${basename%.*}"

  bin_in="build/$basename_wo_ext.in"
  bin_out="build/$basename_wo_ext.out"
  asm_out="build/$basename.out.asm"

  echo "• $asm_in"
  (
    nasm "$asm_in" -o "$bin_in" || exit 1
    build/decode "$bin_in" > "$asm_out" || exit 1
    nasm "$asm_out" -o "$bin_out" || exit 1
    cmp "$bin_in" "$bin_out" || exit 1
  ) && echo "  ✅ passed" || {
    echo "\n––– Input  assembly –––"
    cat "$asm_in"
    echo "\n––– Output assembly –––"
    cat "$asm_out"
    echo "\n––– Diff command –––"
    echo "diff '$asm_in' '$asm_out'"
    echo "\n––– Command –––"
    echo "build/decode '$bin_in'"
    echo "  ❌ failed"
    exit 1
  }

done
