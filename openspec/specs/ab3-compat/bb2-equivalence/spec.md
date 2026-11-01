# ab3-compat/bb2-equivalence Specification

## Purpose

Guarantees that adding AB3 mode changes nothing for Blitz Basic 2: the
default `blitz2` stays byte-identical, and `blitz2-ab3` compiles every
BB2 source exactly as `blitz2` does.

## Requirements

### Requirement: The default blitz2 is unchanged
`blitz2`'s results SHALL stay byte-identical to those of the build of the previous commit (the executables it writes, and `ted`'s, `acidlibs`', `deflibs`' and all library files); the AB3 mode SHALL only be part of `blitz2-ab3`. The `blitz2` program file itself may change with a code rework (rule of 2026-11-01).

#### Scenario: Default build
- **WHEN** `blitz2` is built before and after a code rework and both compile the corpus, the bug-orig tests and the CD's sources
- **THEN** every executable written by the two builds is byte-identical, and so are `ted`'s output files, `acidlibs`, `deflibs` and all library files

#### Scenario: No AB3 mode in blitz2
- **WHEN** `blitz2` is started with `-s SOURCE -e EXE`
- **THEN** it behaves as before the AB3 work (the argument is taken as the project name for the editor), with no AB3 mode

### Requirement: BB2 sources give the same executables with blitz2-ab3
For every BB2-mode program (the corpus, the bug-orig tests, the CD's sources), `blitz2-ab3` SHALL write an executable byte-identical to the one `blitz2` writes with the same options and libraries. Excepted are sources whose result depends on the compiler's own memory: the bug-orig tests of #16 (hangs the compiler) and #335 (addresses of the compiler's memory in the executable).

#### Scenario: Corpus compiled with both
- **WHEN** the corpus sources are compiled once with `blitz2` and once with `blitz2-ab3` in the same batch setup
- **THEN** every pair of executables is byte-identical, and the same sources fail to compile with the same error

### Requirement: BB2 mode uses BB2's libraries and tokens
In BB2 mode `blitz2-ab3` SHALL use BB2's `acidlibs`, `deflibs`, `userlibs/` directory, compiler tokens, the math library 192 for `^` and the resident names as given, exactly as `blitz2`.

#### Scenario: Float power in BB2 mode
- **WHEN** a BB2-mode program uses `a.f^10`
- **THEN** the generated code calls library 192, as with `blitz2`
