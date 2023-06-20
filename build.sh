#!/bin/sh

cc_flags="$(cat compile_flags.txt)"
cc_asm_flags="$cc_flags -S -fno-asynchronous-unwind-tables -fverbose-asm"

[ -d build ] || mkdir build


srcs="
src/harvestine/estimate_cpu_timer_freq.c
src/harvestine/gen_harvestine.c
src/harvestine/harvestine.c
src/sim86/sim86.c
src/sum/sum.c
"

for src in ${srcs}; do
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
