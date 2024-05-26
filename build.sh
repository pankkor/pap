#!/bin/sh

# Newline separated build sources. One target per line.
srcs='
src/harvestine/estimate_cpu_timer_freq.c
src/harvestine/gen_harvestine.c
src/harvestine/harvestine.c
src/harvestine/microbenchmarks.c src/harvestine/microbenchmarks.S
src/harvestine/pf_counter.c
src/harvestine/ptr_anatomy.c
src/harvestine/read_overhead.c
src/sim86/sim86.c
src/sum/sum.c
src/interview1994/cp_rect.c
src/interview1994/draw_circle.c
src/interview1994/has_color.c
src/interview1994/str_cpy.c
'

# Help text generateion
help="
Build script.

Usage
  build.sh [options] [target=all]

Creates 'build' directory and builds specified 'target' with every command from
the comma separated list of '[commands]'.

Example:
  build.sh build,asm,disasm all

Options
  --help,-h         This help.
  --build-disasm    Build target to 'build/[target]' and then disassemble binary
                    to 'build/[target].s'.

Targets
  all                             Build all targets. Default.
"

while read -r src; do
  if [ "$src" ]; then
    basename="${src##*/}"
    basename_wo_ext="${basename%.*}"
    help="$help  $basename_wo_ext
"
  fi
done << EOF
$srcs
EOF

# Options
target="all"

cmd_build=1
cmd_disasm=0

for opt in "$@"; do
  case "$opt" in
    --help | -h)
      echo "$help"
      exit 0
      ;;
    --build-disasm)
      cmd_disasm=1
      ;;
    -*)
      echo "Error: unknown option '$opt'" >&2;
      echo "$help" >&2;
      exit 1
      ;;
    *)
      target="$opt"
      ;;
  esac
done

# Platform options
# Machine type
os=
case "$(uname -s)" in
    Linux*)   os=linux;;
    Darwin*)  os=darwin;;
    *)        echo "Error: unknown OS '$(uname -s)'" >&2; exit 1;;
esac

# Default compiler
default_cc=clang
case "$os" in
    linux)  default_cc=gcc;;
esac

cc="${CC:-"$default_cc"}"
cc_flags="$(cat compile_flags.txt)"
cc_asm_flags="-S -g0 -fno-asynchronous-unwind-tables -fverbose-asm $CC_ASM_FLAGS"

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
case "$os"  in
  linux)    ld_flags=-lm;;
esac

# Disassembler util
# Alternatively on 'otool -vt' could be used on Darwin
disasm='objdump -S'

# Build directory
[ -d build ] || mkdir build

echo "Building target '$target'"

# Build
build_done=0
while IFS='
' read -r src; do
  if [ "$src" ]; then
    # Skip not matching $target
    if [ "$target" != 'all' ]; then
      case "$src" in
        *$target*)  ;;
        *)          continue;;
      esac
    fi

    basename="${src##*/}"
    basename_wo_ext="${basename%.*}"

    echo "
Building '$src'"
    if [ $cmd_build -eq 1 ]; then
      echo " * build  '$src' -> 'build/$basename_wo_ext'"
      $cc $cc_flags $src -o "build/$basename_wo_ext" $ld_flags || exit $?
      build_done=1
    fi

    if [ $cmd_disasm -eq 1 ]; then
      echo \
        " * disasm 'build/$basename_wo_ext' -> 'build/$basename_wo_ext.s'"
      $disasm "build/$basename_wo_ext" > "build/$basename_wo_ext.s" || exit $?
      build_done=1
    fi
  fi
done << EOF
$srcs
EOF

if [ "$build_done" -eq 0 ]; then
  echo "Error: target not found '$target'." >&2
fi
