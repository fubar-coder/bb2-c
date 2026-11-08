# blitzbasic2-c – notes for Claude

C translation of Blitz Basic 2: compiler `blitz2` (v2.10: `blitz2.s` 1.95
with the changes of the CD's compiler, `docs/v2.10/`),
editor `ted` (TED 1.1, `ted.s`) and the libraries `acidlibs`/`deflibs`,
built for AmigaOS (68000) with m68k-amigaos-gcc. Overview and
building: `README.md`; structure: `docs/structure.md`; the checks in the
build: `docs/development.md`; the tests: `tests/README.md`. Open items:
`TODO.md`, as task lists: `- [ ] **Title** …` open, `- [x] **Title** done (date): …` done or
decided (also for numbered steps, `1. [x] …`); no strikethrough.

**This repository is the reference** for the code (decision of
2026-10-27, takeover from bb2 `7b5df74`); bb2 no longer builds the
authoritative state. Here are the build, the reference builds (`ted_ref`,
`blitz2_ref`), the original sources (`archive/`), all tests (host tests,
TED oracle, corpus, TED A/B, library tests in the emulator, tests of
reproduced original bugs) and the notes on the translation in `docs/`
(from bb2 `ee6174b`, translated to English) – that is also where the
reasons for code comments like `BUG(orig)`, `DIFF`, `ted.s:NNN` or
`blitz2.s:NNN` are. Only bb2's analysis tools (Python) were not taken
over. State of the takeover: `TODO.md`.

## Building and checking

```
cmake --preset amiga && cmake --build --preset amiga   # result: build/amiga/blitz2/
ctest --preset host                                    # tests without emulator (seconds)
ctest --preset emulator                                # tests in Amiberry
ctest --preset oracle                                  # AmiBlitz3's compiler checks tests/ab3's expectations;
                                                       # run it before merging changes of tests/ab3
```

* Toolchain via `PATH` (`m68k-amigaos-gcc`, gcc ≥ 15 because of
  `#embed`, ≥ 16.2-rc21 because of `__saveds`, checked by configure),
  otherwise `AMIGA_TOOLCHAIN_PREFIX`. Ninja.
* The shell is **fish**: wrap loops, `$(...)`, heredocs in `bash -c '…'`
  or `bash <<'EOF'`.
* A clean rebuild has **no warnings**. A reference from normal code into
  the error check area is a `blitzlibcheck` error (the ones the
  originals have are fixed, 2026-10-31, TODO.md).
* CMake generates `.clangd` on the first configure (not in git).
* Emulator: the Kickstart ROM and hardfiles are local in
  `amiga-emulation/` (not in git, see `AMIGA-EMULATION-SETUP.md`); default
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

* **Acceptance for every rework** (rule of 2026-11-01): the *results*
  stay byte-identical to the state before (the `prev` build, the build of
  the previous commit of this repository: `cmake --preset amiga
  -DBLITZ2_PREV=ON`, `tests/README.md` "The prev build") unless a change
  of them is intended: the executables the compilers generate, the files
  TED writes, and `acidlibs`, `deflibs` and all library files.
  `ctest --preset all` compares them with the `prev` build
  (`prev_results`, the corpus configuration `prev`, the `ted_ab`
  combinations `P5`/`Pc`; without a prev build only `prev_results` is
  skipped and the others run without that part), and an intended change
  is named by `-DPREV_INTENDED`. The programs themselves
  (`blitz2`, `blitz2-ab3`, `ted`, the tools) may change with a code
  rework; then check functionally with the tests here
  (`ctest --preset all` and `ctest --preset bug-orig`, needs Amiberry
  and the originals) and name the rework in the commit. A rework that
  is meant to change no code is still best checked by comparing the
  programs byte by byte. An intended change of a result (e.g. a fixed
  original bug) is named in the commit, with tests.
* **`archive/`** contains unchanged originals (sources like
  `sources/ted-1.1/`, reference data) – never edit; a new state is a new
  directory with a version number, the origin is in `archive/README.md`.
  Anything derived from it is created in the build directory or as
  generated test data under `tests/data/`. Material without a clear
  license (CD) is not checked in but loaded with
  `cmake/fetch-originals.cmake` (`archive/cd/`, `archive/aminet/`,
  `archive/bernd/`, ignored); tests that need it are skipped without it.
* **Original bugs are fixed deliberately** (bug-fix phase: the port is
  complete; user decisions of 2026-10-30 and 2026-11-07). The goal is BB2
  compatibility with the bugs fixed, in the default variant; there is no
  bug-preserving variant (whoever needs the original behaviour uses the
  original compiler and libraries). Tests check the expected behaviour,
  not equality with the original; the comparison with the previous commit
  of this repository stays (Acceptance). A bug not fixed yet is
  documented: `BUG(orig) (#N)` in the code, a bug-orig test that comes out
  REPRODUCED (`tests/README.md`, "bug-orig tests"); `DIFF` only where
  technically necessary. **A fix** (spec `testing/bug-fix-workflow`):
  1. one issue per bug;
  2. in the fix commit: the code change; the test of the expected
     behaviour - the bug-orig test gets the header `; @fixed #N` (or the
     equivalent of its suite) and must come out FIXED in
     `ctest --preset all`; the comment `BUG(orig)` becomes `Fixed (#N)`;
     the entry in `docs/*/deviations.md` as an intended deviation (kind
     F); the changed results named in the commit message, and a line
     `Fixes #N` in it (GitHub closes the issue on the push);
  3. the entry on the website page of fixed original bugs
     (`site/content/docs/compared-with-original/fixed-bugs.md`);
  4. the label `fixed-in-port` on the issue (every fix);
  5. after the user's push: check that the issue was closed (close it
     with a comment naming the commit if not).
  Steps 3-5 are the coordinator's; an agent names them in its report.
* **Only C/C++ and CMake.** No Python, no shell scripts in the build.
  Whatever compiler and linker can do themselves, without a tool
  (`#embed` + `static_assert`, `strip`); text and control as CMake;
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
  (`ACIDLIBS_ORDER`); every library is a C file (no prebuilt originals
  any more; the originals for comparison come from the CD).

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
