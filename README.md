# cpp-reflection-stuff

Single-file C++26 experiment exploring compile-time codegen via reflection
(`std::meta`, `#embed`, consteval expansion statements, `define_aggregate`).
A `.dt` data file is baked in at compile time and turned into synthesized C++ aggregate types.

## Requirements

- g++ 16.0 or later

```sh
g++-16 -std=c++26 -freflection main.cpp -o a.out
```

or, with the included Makefile:

```sh
make        # build a.out
make run    # build + run
```

## The `.dt` format

A `.dt` file contains one or more bracketed blocks. Each non-empty line inside a
block is `name: type`:

```
[
    first: i32
    second: f32
]

[
    gino: i32
    pino: bool
    pierino: f32
]
```

### Supported types

| Name | C++ type |
|------|----------|
| `i8` `i16` `i32` `i64` | `signed char`, `short`, `int`, `long long` |
| `u8` `u16` `u32` `u64` | `unsigned char`, `unsigned short`, `unsigned int`, `unsigned long long` |
| `f32` | `float` |
| `f64` | `double` |
| `bool` | `bool` |

## How it works

1. `NewType.dt` is embedded at compile time via `#embed`.
2. Each `[ ... ]` block becomes a synthesized C++ aggregate type (one field per
   `name: type` line, in order).
3. Block 0 is instantiated as `T1` with `T1 t1{2, 2.0}` in `main`; every block
   is x-rayed to print its field names.

## Notes

- A block caps at 16 fields (`MAX_BLOCK_FIELDS`); more causes a compile-time
  `std::out_of_range`.
- Editing `NewType.dt` requires recompiling

