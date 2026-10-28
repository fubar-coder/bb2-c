# blitzbasic2-c – notes for Claude

C translation of Blitz Basic 2: compiler `blitz2` (v1.95, `blitz2.s`),
editor `ted` (TED 1.1, `ted.s`) and the libraries `acidlibs`/`deflibs`,
built for AmigaOS (68000) with m68k-amigaos-gcc. Structure, building and
the checks in the build: `README.md`. Open items: `TODO.md`.

**This repository is the reference** for the code (decision of
2026-10-27, takeover from bb2 `7b5df74`); bb2 no longer builds the
authoritative state. Here are the build, the reference builds (`ted_ref`,
`blitz2_ref`), the original sources (`archive/`), the TED oracle and the
host tests of TED. Still in `../bb2`: corpus, TED A/B and library tests in
the emulator, analysis tools. The notes on the translation are in `docs/`
(from bb2 `ee6174b`, unchanged, paths in the layout of bb2) – that is also
where the reasons for code comments like `BUG(orig)`, `DIFF`, `ted.s:NNN`
or `blitz2.s:NNN` are. State of the takeover: `TODO.md`.

## Building and checking

```
cmake --preset amiga && cmake --build --preset amiga   # result: build/amiga/blitz2/
ctest --preset host                                    # tests without emulator (seconds)
ctest --preset emulator                                # tests in Amiberry
```

* Toolchain via `PATH` (`m68k-amigaos-gcc`, gcc ≥ 15 because of
  `#embed`), otherwise `AMIGA_TOOLCHAIN_PREFIX`. Ninja.
* The shell is **fish**: wrap loops, `$(...)`, heredocs in `bash -c '…'`
  or `bash <<'EOF'`.
* A clean rebuild has **no compiler warnings**; the only expected ones are
  the 73 warnings of `blitzlibcheck` (references into the error check
  area, as with the originals).
* CMake generates `.clangd` on the first configure (not in git).
* Emulator: the Kickstart ROM and hardfiles are local in
  `amiga-emulation/` (not in git, see the README there); default
  configuration `tools/lib/amiga_emulation/default.uae`. Hardfiles are
  read-only: tests write only to `TEST:` or `RAM:` (system programs like
  `ShowConfig` also write to `SYS:` and then hang).
* **Host side** (`host/`, ExternalProject, native compiler): tools and
  host tests. Host tests live with their component
  (`programs/ted/tests/host/`), are snitch programs (C++20, C headers in
  `extern "C"`), **one program per test file** (the TED core has global
  state). For checking, also build with `-DHOST_SANITIZE=ON`.
* Emulator runs only via `tools/lib/amiga_emulation` or `amiga_run`
  (isolated, can run in parallel); control socket: arguments separated by
  tabs. Tests with the emulator get the ctest label `emulator` and the
  return value 77 (skipped) if Amiberry or the data are missing.

## Rules

* **Acceptance for every rework:** `blitz2`, `ted`, `acidlibs`, `deflibs`
  and all library files byte-identical to the state before (build of the
  previous commit of this repository). If a deviation is intended (e.g. a
  code rework), check functionally – with the tests here
  (`ctest --preset all`) and, as long as they are still in bb2, with the
  bb2 tools against `build/amiga/blitz2/` of this repository (corpus,
  snitch tests, TED A/B) – and name the deviation in the commit.
* **`archive/`** contains unchanged originals (sources like
  `sources/ted-1.1/`, reference data) – never edit; a new state is a new
  directory with a version number, the origin is in `archive/README.md`.
  Anything derived from it is created in the build directory or as
  generated test data under `tests/data/`. Material without a clear
  license (CD) is not checked in but loaded with
  `cmake/fetch-originals.cmake` (`archive/cd/`, `archive/aminet/`,
  ignored); tests that need it are skipped without it.
* **Do not fix original bugs**, only document them (`BUG(orig)` in the
  code, list of deviations in `docs/*/deviations.md`); `DIFF` only where
  technically necessary. Applies only to the port: once it is complete
  (including the comparison with blitz2 2.10), original bugs may be fixed
  deliberately (one issue per bug, marked as an intended deviation).
* **Only C/C++ and CMake.** No Python, no shell scripts in the build.
  Whatever compiler and linker can do themselves, without a tool
  (`#embed` + `_Static_assert`, `strip`); text and control as CMake;
  binary formats and logic as a C tool in `tools/` (logic in
  `tools/lib/`, tool = `main.c`, tests in `tools/tests/` with inputs from
  memory).
* **Checks in the build:** in the same build step as the result (another
  `COMMAND` of the custom command or `POST_BUILD` on the target). If the
  check fails, the result counts as not built, and nothing that depends on
  it is built. No temporary files, no stamp files. Every check needs a
  test with deliberately broken input.
* **Two compilation worlds:** libraries with the Blitz conventions
  (`cmake/BlitzLibrary.cmake`: `-ffixed-a3/a4/a5`, freestanding,
  `-fno-toplevel-reorder`, one hunk via `libs/blitzlib.ld`); blitz2 and
  TED as programs with `-fbaserel` (a4 = base of the globals). Code for
  both sides (`shared/`) is compiled by each side itself, with its own
  environment header (`filereq_env.h`).
* `libs/acidlibs` is joined in the order of the original file
  (`ACIDLIBS_ORDER`); `prebuilt/` contains originals that are not ported
  yet (`dhandlerlib`).

## License

GPL-3.0-or-later with `RUNTIME-LIBRARY-EXCEPTION` (README, section
"License"); license situation of the originals in `archive/README.md`.
New code is under the same license; do not take over code with an unclear
or GPL-incompatible license.

## Git

* One topic per commit; never combine separable work.
  All texts in the repository (documentation, comments, commit messages,
  messages of tools) and GitHub issues are English.
* Before a commit, check `git status`: stage only the files of the topic
  (otherwise staged `git mv` slip in).
* Remote `origin` (GitHub `fubar-coder/bb2-c`): no force push, do not
  rewrite pushed history.
