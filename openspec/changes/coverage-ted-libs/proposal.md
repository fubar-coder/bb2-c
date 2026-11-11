# Proposal

## Why

The coverage configuration records which functions of `blitz2` and
`blitz2-ab3` each test executes, and the host tests' gcov data cover the
TED core and the shared code. TED on the Amiga and the Blitz libraries
are not measured: a change of `programs/ted/` or `libs/` selects every
emulator test by rule (`tools/covsel/rules.txt`), and nothing shows which
library code no test reaches. The libraries are the largest part of the
open bug-orig issues, so their fix rounds gain most from a selection.

## What Changes

- **TED with gcov** in the coverage configuration: lines and branches,
  the `.gcda` files written from TED's quit path through dos.library.
- **The Blitz libraries with gcov** (or the function bitmap of the
  compilers if a spike shows that the gcov data are not relocated when
  blitz2 copies a library into a program): `libs_tests` writes the data
  of the libraries it loads.
- **A coverage library** whose finit routine writes the data of the
  libraries inside a compiled program, and a compiler option of the
  coverage configuration that links it into every program.
- **Collection:** several `.gcda` sets of one test are merged on the
  host; `covmap` turns them into lcov tracefiles; changes of
  `programs/ted/` and `libs/` then select by coverage instead of by rule.
- The normal configuration and every result stay unchanged.

## Capabilities

### New Capabilities

None.

### Modified Capabilities

- `testing/coverage`: TED and the Blitz libraries are measured in the
  coverage configuration.

## Impact

- `programs/ted/CMakeLists.txt`, its linker script and quit path (coverage
  configuration only).
- `cmake/BlitzLibrary.cmake`, `libs/tests` (the loader of the library
  tests), a new coverage library under `libs/`.
- blitz2's option handling (the coverage configuration's option that
  links the coverage library).
- `tools/lib/covmap`, `tools/covsel/rules.txt`, `cmake/coverage-map.cmake`.
- `tests/README.md`, `TODO.md`.
