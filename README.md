# tamagotchi-p1

A small toolchain for the E0C6S46, the 4-bit CMOS micro controller inside the
Tamagotchi P1: a C-subset compiler that produces ROM binaries, and an
emulator that runs them.

Everything is plain C++17. The windowed emulator needs SDL3 (optional); the
console emulator and the compiler build everywhere with a C++17 compiler and
CMake.

## Repository layout

```
compiler/      C-subset compiler (lexer, parser, code generator, assembler)
emulator/      E0C6S46 emulator (CPU core, console shell, optional SDL window)
examples/      example project (compiled to ROM by the compiler)
CMakeLists.txt single build script for all platforms
```

## The C subset

The target is a 4-bit machine: every value is 0..15, arithmetic is modulo 16,
and each variable occupies one RAM nibble.

### Types and storage

| construct                  | meaning |
|----------------------------|---------|
| `char, int, short, long, signed, unsigned` | one nibble (identical) |
| `void`                     | only for functions |
| `auto/register/static/extern/const/volatile` | accepted, no effect |
| `typedef ... name;`        | type alias (scalar, struct, union) |
| `enum [tag] { A, B = 5 };` | constants 0..15 |
| `struct tag { f1; f2; };`  | global struct variable, access `s.f` |
| `union tag { a; b; };`     | members share the first nibble |
| `sizeof(expr or type)`     | size in nibbles |
| arrays                    | `char a[4];` with constant index `a[2]` |

Not supported on purpose: `float`, `double`, `goto` (no sense on 4 bit).

### Statements and expressions

`if/else`, `while`, `for`, `do`, `switch/case/default`, `break`, `continue`,
`return`, blocks, `;`, `++/--` and the operators
`= + - & | ^ ~ ! == != < > <= >= * / %` are the usual C constructs.
All arithmetic wraps modulo 16. Comparison results are 0 or 1.

### Built-ins

| function | effect |
|----------|--------|
| `print("..")`, `dbg_print("..")` | text on the debug channel |
| `print_hex(e)`, `print_dec(e)` | one value as hex/dec |
| `print_int(arr2)`, `print_int16(arr4)` | 8/16 bit decimal |
| `print_num("..", e)` | text and value on one line |
| `inc16/add16/sub16/cmp16(arr4, ...)` | 16 bit arithmetic |
| `mul8(a2, b2, dst4)` | 8x8 to 16 bit multiply |
| `rand16(arr4)` | 16 bit PRNG in place |
| `p = &v`, `*p` | pointers (12 bit address) |
| `set_text(buf,"..")`, `print_str(buf)` | strings in RAM |
| `strlen/strcpy/strcmp/memset/memcpy` | string and memory helpers |
| `assert(e)` | stop on failure |
| `read_key()` | buttons: bit0=A, bit1=B, bit2=C |
| `draw_text([x,] y, "..")`, `display_digit(e)` | text and digit on the LCD |

Compilation model: a small C file becomes E0C6S46 assembly and then a
big-endian 12-bit ROM binary (see `compiler`). Code is placed on ROM pages
automatically; `main` starts through the reset vector at `0x100`.

## Example project

`examples/c_ext_demo/main.c` uses the extended syntax (typedef, enum,
struct, union, sizeof) together with core language and built-ins. Its output
is sent over the debug channel; run it in the console emulator and the
numbers `3 12 6 4 9 7 2 1 1 3` appear.

It works like this:

1. `main.c` declares globals, an enum, a struct point and a union.
2. `main()` fills values, reads them back and prints sizes.
3. The compiler turns text into LCD/debug stream instructions.
4. The ROM prints the values through the debug channel of the emulator.

How to build and run it is shown below; the ROM file
(`examples/c_ext_demo/main.bin`) is produced by CMake as `example-rom`.

## Build

Requirements: CMake >= 3.16, a C++17 compiler. SDL3 is optional and only
used for the windowed emulator.

```
cmake -S . -B build
cmake --build build
```

Binaries:

- `build/tama_compiler` (or `.exe`) - the C-subset compiler
- `build/tamagotchi-headless` (or `.exe`) - console emulator (no display)
- `build/tamagotchi` - windowed emulator (only if SDL3 was found)
- `build/c_ext_demo.bin` - ROM of the example, built automatically

Platform notes:

- Windows (MinGW/MSYS2, MSVC or Clang): nothing special needed. When SDL3
  is installed via vcpkg/msys2 it is picked up automatically.
- macOS: `brew install cmake sdl3` then run the cmake commands above.
- Linux: `sudo apt install cmake g++ libsdl3-dev` (or your package
  manager) then run the cmake commands above.

Without SDL3 the windowed target is skipped; the console emulator still
works.

## Run

Console emulator with the example ROM:

```
build/tamagotchi-headless build/c_ext_demo.bin
```

The windowed emulator accepts the ROM as its first argument:

```
build/tamagotchi build/c_ext_demo.bin
```

To run the compiler directly:

```
build/tama_compiler examples/c_ext_demo/main.c -o out.bin -s out.s -l out.lst
```

## Notes

- The original Tamagotchi ROM is not part of this repository. The toolchain
  runs every E0C6S46 binary; provide your own ROM if you want to use the
  original firmware.
- The console emulator reads keyboard input from stdin or `--keys`, so it
  fits into scripts and CI.
