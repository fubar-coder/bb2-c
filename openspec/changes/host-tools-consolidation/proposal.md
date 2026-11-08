# Proposal

## Why

The repository has 31 host programs: 25 in `tools/` (each a `main.c` of
40-500 lines around a library in `tools/lib/NAME/`), `corpusrun` and
`cdrun` (`tests/corpus/runner/`), `tedab` and `teddump`
(`tests/ted-ab/runner/`), `tedtok` and `teddetok`
(`programs/ted/tools/`). Each has its own CMake variable
(`${BLITZTOKENS}`, `${TEDTOK}`, ... `CMakeLists.txt:70-97`), its own entry
in `BUILD_BYPRODUCTS` (`CMakeLists.txt:109`), its own `help_*` test
(`host/CMakeLists.txt:50-54`) and its own argument style: `biglib join`,
`vasmlisting LISTING map`, `hunkcheck --hunks`, `ab3tok -cmp`,
`blitzres --dump`, `bb22 --info`. None of the tool directories has a
README; what a tool does is in its `-h` text and in scattered mentions in
`docs/structure.md`, `tests/README.md` and `docs/development.md`. It is
hard to know which tool does what, and every new tool adds another
program, variable and test.

## What Changes

- Group the tools thematically into a few programs with subcommands
  (`tool SUB [options] args`). The logic stays in `tools/lib/`; only the
  `main.c` files are merged: a group's `main.c` is a dispatch table, each
  former `main.c` becomes the body of one subcommand. 31 programs become
  14 (grouping and reasons: design D1).
- One syntax for all of them: subcommand first, `-h` for the program and
  for every subcommand, usage errors exit 2, one error message form
  (design D2, D4). Tools that have several modes today but select them
  with a flag or by argument position (`hunkcheck`, `vasmlisting`,
  `blitzres`, `ab3tok`, `bb22`, `biglib check`) get subcommands.
- A shared dispatch library `tools/lib/subcmd` (design D3).
- Every program, merged or standalone, gets a `README.md` in its
  directory: purpose, subcommands and options (matching `-h`), inputs and
  outputs, callers, an example. A host test checks that the README
  contains every synopsis line of `-h` (design D6). `docs/structure.md`,
  `tests/README.md` and `docs/development.md` link to the READMEs where
  they describe tools inline today.
- The `help_*` tests cover every subcommand (design D5).
- The old program names are removed, without aliases; every call site is
  switched in the commit of its group (design D8, D9).
- **BREAKING** for anyone who calls the host tools by hand or from their
  own scripts: program and option names change. Nothing outside the
  repository depends on them: they are build products, not installed
  (`install()` covers only `blitz2`, `ted` and the library files,
  `CMakeLists.txt:189-191`).
- No change of any result: the executables the compilers generate, the
  files TED writes, `acidlibs`, `deflibs` and all library files stay
  byte-identical (acceptance rule, `CLAUDE.md`).

**Timing:** implementation starts after compiler fix round R3 is merged.
R3 changes code generation and library results; a rework of the tools in
the same period would mix the byte comparison with the previous commit
(`-DBLITZ2_PREV=ON`) with intended result changes, and the R3 tests and
fixes touch the tool call sites (`tests/bug-orig/CMakeLists.txt`,
`libs/userlibs/rwe/*.cmake`). The first task checks that R3 is merged;
nothing else of this change is implemented before.

## Capabilities

### New Capabilities

- `tooling/host-tools`: how the host programs are organised and invoked:
  the grouping rule, the subcommand syntax, `-h` for every subcommand,
  exit codes and messages, a README per program, and that regrouping
  changes no behaviour or build result.

### Modified Capabilities

None. No spec in `openspec/specs/` (`ab3-compat/*`,
`compiler/debug-constant`) describes the host tools. The in-flight change
`expected-results-testing` names `tools/prevcmp`, `checkdata` and
`asmcmp` in its design and tasks (`design.md:377`, `tasks.md:21-23`) as
rationale and history of that change, not as a tool interface; those
texts are not edited here.

## Impact

- `tools/` (25 programs become 11, 6 of them groups;
  directories `tools/NAME/` are renamed or merged; per-tool test scripts
  such as `tools/bbdetok/tooltest.cmake` move with them), `tools/lib/`
  (new `subcmd`; the other libraries are unchanged), `tools/tests/`
  (a test for `subcmd`; the library tests stay).
- `programs/ted/tools/`, `tests/corpus/runner/`, `tests/ted-ab/runner/`
  (the TED, corpus and TED A/B programs).
- `CMakeLists.txt` (variables, `BUILD_BYPRODUCTS`), `host/CMakeLists.txt`
  (the `help_*` loop), `cmake/*.cmake` (`coverage-map.cmake:29`,
  `coverage-select.cmake:30`, `setup-amiga-forever.cmake:12,47` name tool
  paths), every CMake file that calls a tool (call sites per group:
  design), `tools/covsel/rules.txt:52-62` (path patterns of the tools for
  the coverage selection).
- `docs/structure.md`, `docs/development.md`, `tests/README.md`,
  `TODO.md` where they name tools.
- Not affected: the program code of `blitz2` and `ted`, the libraries
  (`libs/`), the site.
