# Tasks

## 1. Spike

- [ ] 1.1 A Blitz library compiled with `--coverage` (`gcov_info` table behind a marker): are the pointers inside the gcov data relocated when blitz2 copies the library into a program and when `libs_tests` loads it? Write the result into design.md; if not, the libraries use the compilers' function bitmap instead (D2)

## 2. TED

- [ ] 2.1 TED with gcov in the coverage configuration: `--coverage`, flat `.gcda` names, written from its quit path through `__gcov_info_to_gcda()` and dos.library; baserel kept if it fits, else not in the coverage configuration; `ted_ab` and the TED bug-orig tests collect the `.gcda` (D1). Verify: the normal `ted` is byte-identical; a `ted_ab` test's tracefile names TED functions

## 3. Blitz libraries

- [ ] 3.1 The Blitz libraries with gcov (or the bitmap, after 1.1); `debuglib.c` excluded; `blitzlibcheck` still passes; `libs_tests` writes the data of the loaded libraries (D2). Verify: the normal library files are byte-identical
- [ ] 3.2 A coverage library whose finit routine writes the libraries' data in a compiled program; a compiler option of the coverage configuration that links it into every program (D2). Verify: a corpus program's tracefile names the library functions it ran

## 4. Collection and selection

- [ ] 4.1 Several `.gcda` sets of one test merged with `m68k-amigaos-gcov-tool merge`; `covmap` reads them with the coverage build's `.gcno` (D3). Verify: a test with two runs counts both
- [ ] 4.2 `tools/covsel/rules.txt`: `libs/` and `programs/ted/` select by coverage (D4); `test_covsel` checks it with the real rules
- [ ] 4.3 Check and docs: a full `ctest --preset coverage` and map; the scenarios of the spec; `tests/README.md` (what is measured, excluded libraries), `TODO.md`; a clean normal build, `ctest --preset all` and `ctest --preset bug-orig` with the known verdicts
