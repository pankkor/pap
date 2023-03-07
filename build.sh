#!/bin/sh

cc_flags="$(cat compile_flags.txt)"
cc_asm_flags="$cc_flags -S -fno-asynchronous-unwind-tables -fverbose-asm"

[ -d build ] || mkdir build

for src in src/*.c; do
  basename="${src##*/}"
  basename_wo_ext="${basename%.*}"

  case "$1" in
    asm)
      clang $cc_asm_flags $CC_ASM_FLAGS "$src" -o "build/$basename_wo_ext".s || exit $?
      ;;
    *)
      clang $cc_flags "$src" -o "build/$basename_wo_ext" || exit $?
      ;;
  esac
done
