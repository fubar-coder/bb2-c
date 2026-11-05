# Proposal

## Why

Nobody can tell which statements of a Blitz program ran, or how often.
Users of the compilers have no coverage report for their own programs,
and the repository's test programs (the corpus, `tests/ab3/cli`) are
measured only on the C side (change `test-coverage`), not on the Blitz
side: which Blitz statements and library paths a test program really
exercises. With runtime error checking on, the compiler already writes a
record before every statement (`debugstuff`, `compile.c:1066`) that
points at the statement's source line, so the information about
statements and lines exists inside the compiler; only a counter and a
report are missing. User decisions of 2026-11-05: this is a compiler
feature for users first, and a coverage source for the test selection of
`test-coverage` second; lcov is the single output format after
collection on the host.

## What Changes

- **A coverage mode of `blitz2-ab3`** (option `-coverage`, off by
  default), in BB2 mode and in AB3 mode: the compiled program counts
  how often each statement runs and writes the counts when it ends.
- **A statement map** that the compiler writes next to the executable
  at compile time: for every counted statement its source file and
  line, and for every `Statement`/`Function` procedure its name and
  first line.
- **A host tool `blitzcov`**: turns map and counts (of one or several
  runs) into an lcov `.info` file (`SF`, `FN`, `FNDA`, `FNF`, `FNH`,
  `DA`, `LF`, `LH`).
- **Test integration**: the emulator runners of `tests/ab3/cli` and the
  corpus can compile with the mode and leave the per-test `.info` file
  under `<build>/coverage/lcov/<test>.info`, where `test-coverage`
  collects its inputs.
- Normal compiles do not change: without the option every result is
  byte-identical to the state before.

## Capabilities

### New Capabilities

- `compiler/source-coverage`: the coverage mode of the compilers - how
  it is switched on, what the program and the compiler write, and the
  lcov report derived from it.

### Modified Capabilities

None. The new option is specified in `compiler/source-coverage`; the
requirement "Unsupported options are refused" of `ab3-compat/command-line`
lists other options and is not affected.

## Impact

- `programs/blitz2/src/`: `ab3args`/`ab3main.c` (option and job field),
  `compile.c` (a counting point next to `debugstuff`), `ab3inc.c`
  (include name and line of a statement), a new coverage module for the
  map writer, and a small run-time part of the generated program.
- `tools/blitzcov` with its logic in `tools/lib/blitzcov` and host tests.
- `tests/ab3/cli`, `tests/corpus`: cases and runner options for the mode.
- README, `docs/ab3/compiler.md` (the option), `TODO.md`.
- No change of results without the option: compiled programs, libraries,
  `acidlibs`, `deflibs` and the written files stay byte-identical.
