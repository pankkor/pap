## Performance-Aware-Programming Course Playground
https://www.computerenhance.com/p/table-of-contents

https://github.com/cmuratori/computer_enhance

### Supported platforms
- macOS (clang)
- Linux (gcc)

### Supported architectures
- AArch64
- x86_64

### Build
```
build.sh [asm]
```
Creates `build` directory and build all the targets from `src` directory. One
target per source file. With `asm` argument the output is an assembly file.

### Files
#### Prologue
- `src/sum/sum.c`            - sum of array with and w/o SIMD

#### Part 1: Simple 8086 simulator
- `src/sim86/*.(h|c)` - 8086 instruction simulator
- `src/sim86/listings/*.asm` - 8086 decode and simulation test listings
- `test.sh` - script to test sim86 with test listings

#### Part 2: Harvestine distance calculation
- `src/harvestine/calc_harvestine.h` - func to calculate harvestine distance
- `src/harvestine/estimate_cpu_timer_freq.c` - util to estimate timer frequency
- `src/harvestine/gen_harvestine.c` - generate json with pairs of coordinates
- `src/harvestine/harvestine.c` - parse json and calculate harvestine distances
- `src/harvestine/profiler.(h|c)` - simple instrumentation profiler
- `src/harvestine/timer.(h|c)` - time stamp counter platform abstraction
- `src/harvestine/types.h` - types and macroses

### Misc
- `.lvimrc`                  - vim local config. Ignore if you don't use vim
- `build.sh`                 - build script
- `compile_flags.txt`        - list of compilation flags used by clangd and build.sh
- `live.sh`                  - live coding environment (requires 'entr' utility)
