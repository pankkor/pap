## Performance-Aware-Programming Course Playground
https://www.computerenhance.com/p/table-of-contents

https://github.com/cmuratori/computer_enhance

### Supported platforms
- AArch64

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
- `src/sim86/*.(h|c)`        - 8086 instruction simulator
- `src/sim86/listings/*.asm` - 8086 decode and simulation test listings
- `test.sh`                  - script to test sim86 with test listings

### Misc
- `.lvimrc`                  - vim local config. Ignore if you don't use vim
- `build.sh`                 - build script
- `compile_flags.txt`        - list of compilation flags used by clangd and build.sh
- `live.sh`                  - live coding environment (requires 'entr' utility)
