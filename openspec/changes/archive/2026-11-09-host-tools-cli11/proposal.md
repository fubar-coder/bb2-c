# Proposal

## Why

The host programs of `tools/`, `programs/ted/tools/tedsrc/`,
`tests/corpus/runner/` and `tests/ted-ab/runner/` parse their command
line by hand: `strcmp(argv[i], "--name")` loops in about 23 files
(`tools/*/cmd_*.c`, `tools/amiga_run/main.c`, `tools/hdftool/main.c`,
`tools/xtra/main.c`, `tools/mktestdata/main.c`, `tests/corpus/runner/`,
`tests/ted-ab/runner/`), each with its own handling of missing values,
repeated options, `--` and error messages, and a usage text written as a
C string that the README has to repeat line by line. The dispatcher
`tools/lib/subcmd` (`subcmd.c`, 146 lines) covers only the first
argument. `hdftool` and `xtra` still dispatch by themselves and have no
`SUB -h`. A maintained option parser does all of this once, with the same
behaviour in every program.

## What Changes

- The program and command layer of the host programs (`main.c`,
  `cmd_*.c`) becomes C++20 (`main.cpp`, `cmd_NAME.cpp`) on CLI11
  (BSD-3-Clause, header only), fetched with `FetchContent` at a pinned
  release and SHA256 like snitch (`cmake/Snitch.cmake`). The logic in
  `tools/lib/` and the component libraries stays C, with its tests.
- CLI11 replaces `tools/lib/subcmd` and every hand-written option loop.
  `hdftool` and `xtra` move onto it and get `SUB -h` and the subcommand
  help tests.
- Help and usage-error messages are CLI11's, on purpose. The tests that
  match messages and the checks `cmake/check-help.cmake` and
  `cmake/check-readme.cmake` follow the new format; the README usage
  block is kept equal to the program's help by a test (design D6).
- Exit codes: 0 (success) and 77 (skipped) stay; every argument or usage
  error, CLI11's parse errors included, gives 2. Other documented codes
  may change where nothing depends on them; the documents follow (design
  D7). **BREAKING** for a caller that relies on a message text or on a
  status other than 0, 2 and 77; all callers are in this repository and
  change in the same commit as their program.
- Option names CLI11 cannot express as written (single-dash long names,
  `blitzlib cmdref -group/-name/-last`) become `--name` forms (design D5).
- The prev build reuses the main build's CLI11 copy like snitch's, so it
  needs no network (design D2).
- Results stay byte-identical: the executables the compilers generate, the
  files TED writes, `acidlibs`, `deflibs`, all library files, and what the
  tools write (reports, tables, token lists); checked against the prev
  build (CLAUDE.md, "Acceptance").

## Capabilities

### New Capabilities

None.

### Modified Capabilities

- `tooling/host-tools`: the requirements "Host programs are grouped by
  theme" (the main file declares the subcommands for the option parser;
  no hand-written parsing), "One subcommand syntax" (unknown subcommand),
  "Help for the program and for every subcommand" (CLI11's help format),
  "Every subcommand is covered by a help test", "Exit codes and messages"
  and "A README for every host program" (the usage block equals the help).

## Impact

- Code: `tools/lib/subcmd/` removed; the `main.c`/`cmd_*.c` of the 14
  programs become `.cpp`; about 23 files lose their option loops;
  `tools/tests/fake_subcmd.c` and `tools/tests/test_subcmd.c` are
  replaced by a CLI11 fake program and its test.
- Build: `cmake/CLI11.cmake` (new, next to `cmake/Snitch.cmake`),
  `host/CMakeLists.txt` (C++20 for the tool targets, the include, the
  help and README test functions), `cmake/prev.cmake` and
  `cmake/prev-build.cmake` (pass the CLI11 source on),
  `cmake/check-help.cmake`, `cmake/check-readme.cmake`, the callers of
  renamed options.
- Tests that match messages or exit codes (found by grep):
  `tools/tests/CMakeLists.txt`, `tools/bbsrc/detok-tooltest.cmake`,
  `tools/bbsrc/ab3-tooltest.cmake`, `tools/xtra/tooltest.cmake`,
  `tests/corpus/runner/check-cdrun-source.cmake`,
  `programs/ted/tools/tedsrc/tests/run.cmake`, `tests/bug-orig/want.cmake`.
- Documents: the READMEs of the 14 programs, `docs/development.md`
  ("Host programs"), `docs/structure.md`, `tests/README.md`, `TODO.md`.
- Dependency: CLI11 (BSD-3-Clause), downloaded when the host build is
  configured; its licence is compatible with GPL-3.0-or-later and nothing
  of it reaches the m68k results.
