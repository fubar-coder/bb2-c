# Proposal

## Why

The run of a Blitz test is described by directives in the header comments
of its source (`; @pass`, `; @configs`, `; @rwe`, `; @machine`,
`; @runerrs`, ...; about 2700 lines in `tests/**/*.bas`), which CMake
parses at configure time and turns into a pass file for `corpusrun`.
`; @configs clib` alone stands in 316 files. Variants of the same test
(with and without runtime error checking) are copies of the file
(`tests/blitz2/intrinsic-frac.bas` / `intrinsic-frac-runerrs.bas`). With
the test library of change `basic-testlib` the programs judge
themselves; what is left to describe is how a test runs and what the run
must show outside the program. User decisions of 2026-11-21 (chat,
`~/.local/share/bb2-c-work/decisions/rounds/tr/decisions.md`).

## What Changes

- **A run configuration in files** (`tests.toml` per directory,
  `ID.test.toml` per test), hierarchical: the search goes from the test
  upwards and stops at `root = true`. It holds the run settings (what the
  header directives hold today) and the assertions.
- **A matrix**: every run setting can be an axis; the axes of a test are
  named explicitly, its cells are their product minus exclusions.
- **Assertions in the style of hurl** (query, predicate, value) for what
  the program cannot state itself: exit code, requesters, compile result,
  printed output, the declared deviations of the original with optional
  validators. Assertions per cell; a more specific one replaces.
- **A generated driver and a preprocessor**: the runner copies the text
  of the test source (`ID.test.bas`) into a program it writes, which calls
  the selected cases; the preprocessor numbers the checks and keeps their
  line and expression text for the report, and replaces a fixed set of
  placeholders.
- **Registration at build time** from sources and configurations, without
  the emulator: one ctest test per case and side.
- **The runner reads the files, CMake does not**; a diagnostic prints the
  effective configuration with the origin of every key.
- The new model and the existing directives live side by side: a test is
  in the new model if its file name says so.

## Capabilities

### New Capabilities

- `testing/basic-test-runner`: the run configuration (files, hierarchy,
  settings, matrix, assertions), the driver, the registration and naming
  of the tests, and how the runner judges the results of the test library.

### Modified Capabilities

- `testing/test-model`: a test that judges itself holds its expectations
  in its source and its run configuration and has no recording; the
  requirements on the difference of output and on `[orig]`/`[fixed #N]`
  pairs are scoped to the tests with output files.

## Impact

- `tests/corpus/runner` (`corpusrun`: a subcommand for tests of the model,
  the driver, the configuration diagnostic), `tools/lib` (parser,
  resolution, matrix, assertions, registration; host tests with broken
  input), `tools/testinfo/rules.txt` (configuration files select tests).
- `tests/bug-orig/CMakeLists.txt`, `tests/blitz2/CMakeLists.txt`: the
  glob and the include of the registration next to the existing tests.
- `tests/README.md`, `docs/testing.md`.
- No result of the compilers or libraries changes.
- Depends on `basic-test-interface` (the contract with the test library),
  which is done first.
