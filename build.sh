#!/bin/sh

# machine type
os=
case "$(uname -s)" in
    Linux*)   os=linux;;
    Darwin*)  os=darwin;;
    *)        echo "Error: unknown OS '$(uname -s)'" >&2; exit 1;;
esac

#default compiler
default_cc=clang
case "$os" in
    linux)  default_cc=gcc;;
esac

cc="${CC:-"$default_cc"}"
cc_flags="$(cat compile_flags.txt)"
cc_asm_flags="-S -fno-asynchronous-unwind-tables -fverbose-asm $CC_ASM_FLAGS"

# Compiler flags

# Optimisations
# x86 and ARM have different behaviour for optimisation flags
# x86:
# -march - minimal architecture to run on, overrides -mtune and -mcpu
# -mtune - tune for mictoarchitecture but don't change ABI
# -mcpu  - deprecated alias for -mtune
#
# ARM:
# -march - minimal architecture to run on, doesn't override -mtune
# -mtune - tune for mictoarchitecture but don't change ABI
# -mcpu  - optimise for both cpu architecture and mictoarchitecture
#          (combines -march and -mtune)
#
# Note: clang doesn't supporch -march=native
arch="$(uname -m)"
case "$arch" in
  x86_64)   cc_flags="$cc_flags -march=native";;
  arm64)    cc_flags="$cc_flags -mcpu=native";;
  *)        echo 'Error: unknown architecture' >&2; exit 1;;
esac

# Warnings
case "$cc" in
  clang)    cc_flags="$cc_flags -Wno-gnu-statement-expression";;
  gcc)      cc_flags="$cc_flags -Wno-missing-braces";;
esac

# Linker flags
ld_flags=
case "$os" in
  linux)   ld_flags=-lm;;
esac

# Build process
[ -d build ] || mkdir build

# srcs='
# src/harvestine/estimate_cpu_timer_freq.c
# src/harvestine/gen_harvestine.c
# src/harvestine/harvestine.c
# src/harvestine/microbenchmarks.c src/harvestine/microbenchmarks.s
# src/harvestine/pf_counter.c
# src/harvestine/ptr_anatomy.c
# src/harvestine/read_overhead.c
# src/sim86/sim86.c
# src/sum/sum.c
# src/interview1994/cp_rect.c
# src/interview1994/draw_circle.c
# src/interview1994/has_color.c
# src/interview1994/str_cpy.c
# '
srcs='
src/harvestine/microbenchmarks.c src/harvestine/microbenchmarks.S
'

while IFS=$'\n' read -r src; do
  if [ "$src" ]; then
    basename="${src##*/}"
    basename_wo_ext="${basename%.*}"

    case "$1" in
      asm)
        $cc $cc_flags $cc_asm_flags $src -o "build/$basename_wo_ext.S" \
          $ld_flags || exit $?
        ;;
      *)
        $cc $cc_flags $src -o "build/$basename_wo_ext" $ld_flags || exit $?
        ;;
    esac
  fi
done << EOF
$srcs
EOF
