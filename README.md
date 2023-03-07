## Performance-Aware-Programming Course Playground
https://www.computerenhance.com/p/table-of-contents

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
- `src/sum.c`           - sum of array with and w/o SIMD

#### Part 1:
- `src/decode.c`        - 8086 instructions decode
- `listings/*.asm`      - 8086 decode listings
- `test_decode.sh`      - script to test decode.c with listings

### Misc
- `.lvimrc`             - vim local config. Ignore if you don't use vim
- `build.sh`            - build script
- `compile_flags.txt`   - list of compilation flags used by clangd and build.sh
