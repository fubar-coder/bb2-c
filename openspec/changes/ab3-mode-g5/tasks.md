# Tasks

One commit per task group step that changes files (code, tests, table and
docs of that step together). After every group the table
`tests/data/ab3-examples.tsv` is updated and its diff is checked: it may
only improve (a source goes from failing to compiling, or its first
message moves later), and only for the sources the group is about.
BB2-mode results stay byte-identical to the `prev` build; only group 4
passes `-DPREV_INTENDED`. Issue texts are drafted into files for the
coordinator.

## 1. The test ab3_examples

- [ ] 1.1 `cmake/fetch-originals.cmake`: fetch AmiBlitz3 `58a11658`'s `Sourcecodes/Examples`, `Templates`, `Amiblitz3/Tests`, `Includes`, `Data` and `BlitzLibs/Residents` (one archive with a SHA-256 and extraction by pattern, or per-file hashes if the archive is too big; decide by measuring the size). Verify: a clean fetch fills `archive/amiblitz3/github-58a11658/` with 221 examples, 8 templates, 69 tests, 139 top-level includes; a second run touches nothing; update the header comment of the script and `archive/README.md`.
- [ ] 1.2 Read `tests/corpus/runner/cmd_cd.cpp` and `tests/ab3/cli.cmake`; implement the pass: the `Blitz3:` tree of the survey (four BigLibs, residents, empty `UserLibs`, `Includes`, `Data`), wrapper generation for the includes (`variant.md:963-968`), `-r` where the header says `RuntimeDebug = 0`, `_` for spaces, the result per source ("ok" or the first error line). Verify: a run over the fetched tree gives 437 results and `blitz2-ab3` is the only compiler used.
- [ ] 1.3 The table `tests/data/ab3-examples.tsv` from the survey (`analyse-ab3-state-results.tsv`, both columns), the comparison as a tool function with a host test (changed row, extra row, missing row, unknown source), the test `ab3_examples` (label `emulator`, in `all`, skipped 77 without data or Amiberry), the target `ab3-examples-update`. Verify: the first run equals the survey (examples 139 of 221, templates 4 of 8, tests 50 of 69, includes 42 of 139); the host test fails on each broken table; time `ctest --preset all -R ab3_examples` and state the measured time in the commit (above 10 minutes: move it out of `all` and tell the coordinator).
- [ ] 1.4 Optional target of the label `originals` that records AmiBlitz3's column through its own compiler. Verify: it rewrites only that column; skipped if it needs more than the existing `ab3_cli_ref` setup, then say so in the report.
- [ ] 1.5 Docs: `docs/testing.md` and `tests/README.md` (the test, the table, the update target, the fetch), `docs/ab3/variant.md` "Measured share" points to the table; `TODO.md` task 3 of the AmiBlitz3 item names `ab3-mode-g5`. Verify: `openspec validate ab3-mode-g5 --strict` and a clean build without warnings.

## 2. The type .d without the FPU bit

- [ ] 2.1 Probes against AmiBlitz3 (`ab3_cli_ref` cases, compile and run, `OPTIMIZE 4`): which 4 of the 8 bytes hold the value; `.d` as the 7th to 10th parameter and its slot size; `.d` in a `.res` and in a NewType (`sdl.res.ab3:174`); `SizeOf` of NewTypes and arrays with `.d`; a library argument of type 8; conversions to and from `.l`, `.f`, `.s`; `Function.d`. Write the results into `design.md` D2 and the spec scenarios. Verify: every row of the D2 table has a measured answer; a contradiction of "same as `.f`" is named.
- [ ] 2.2 The type entry `.d` in AB3 mode (size 8, FFP arithmetic like `.f`, the measured byte placement), variables, arrays, NewTypes, pointers, `SizeOf`. Verify: cases against AmiBlitz3 in `ab3_cli_ref` pass; BB2 mode still gives "Type Not Found" (`ab3_cli` case, `ab3_bb2_equivalence`).
- [ ] 2.3 Parameters (also 7th to 10th), `Function.d`, conversions, library arguments of type 8. Verify: cases against AmiBlitz3; the G4 cases still pass.
- [ ] 2.4 Residents: the `.res` loader accepts `.d` in NewTypes and macros. Verify: a case loading a `.res` with a `.d` field (the `sdl.res.ab3` case) and the NewType's `SizeOf`.
- [ ] 2.5 With the FPU bit `.d` stays refused. Verify: the `OPTIMIZE 7` and `.d` case keeps "Not supported: OPTIMIZE 2 (FPU)" at the `OPTIMIZE` line.
- [ ] 2.6 Table and docs: update the table (expect `useful.include` and about 40 includes to move on; programs that set the FPU bit move to the FPU message), `docs/ab3/variant.md` (types, "Differences"), site pages of AB3 types, the directives spec text. Verify: the table diff is as expected and the existing `ab3_cli`, `ab3_cli_ref` cases are unchanged.

## 3. 68020 instructions in the inline assembler

- [ ] 3.1 Read AmiBlitz3's assembler tables at `58a11658` and probe it: the list of 68020 integer instructions and modes it accepts, the opcode and extension words, and whether `OPTIMIZE` bit 1 or anything else gates them (cases with `OPTIMIZE 4`, `5`, none, in AB3 mode and under `CNIF`). Close the list in `design.md` D3 and the spec. Verify: each instruction has a probe that prints its assembled words, from AmiBlitz3.
- [ ] 3.2 `asmlibs.c` tables: the instructions and modes of 3.1, only in AB3 mode; no change to code generation. Verify: the probe cases of 3.1 give the same words with `blitz2-ab3`; BB2 mode still gives "Illegal Assembler Instruction Size" (a case); `ab3_bb2_equivalence` and the `prev` comparison show no BB2 change.
- [ ] 3.3 Cases `DIVU.l d2,d3` and `MULU.l` as in `convert.include.ab3:609-611` and an include case for `convert.include`. Verify: they pass in `ab3_cli` and `ab3_cli_ref`; the executables need a 68020 (stated in the case comment).
- [ ] 3.4 Table and docs: update the table (expect `convert.include`, `dos.include` and the examples through them to move on), `docs/ab3/variant.md`, the site page on assembler (`not-supported.md`, 68020 section) with the instruction set and the 68020 requirement. Verify: the table diff names the sources moved.

## 4. The #940 relaxation (both modes)

- [ ] 4.1 Read `doend3`, `stateret`, `funcret` and the epilogue of `End Function`; record what a numeric Function returns at `End Function`; probe AmiBlitz3 with `End Statement` closing numeric and string Functions. Verify: the result is in `design.md` D4.
- [ ] 4.2 Code: `End Statement` closing a numeric Function compiles as `End Function` and gives the warning `procend`; the string and NewType Function stay "Illegal End Procedure"; the other mismatches unchanged. Verify: a bug-orig-style test pair (numeric accepted with warning and byte-identical to the `End Function` source; string refused); `blitz2` and `blitz2-ab3` agree in BB2 mode (`ab3_bb2_equivalence`).
- [ ] 4.3 Tests of the old fix: change `compiler-procend-kind*` expectations for the numeric case; add the warning case to `ab3_cli` (console line, return code 5) and a `-werr`-style case. Verify: `ctest --preset bug-orig` and the affected corpus tests as expected; the changed results are named in the commit and in `-DPREV_INTENDED`.
- [ ] 4.4 `corpus_cd`: update `tests/data/blitz2-cd.tsv` (`DARTS_POLY.BB`, `DARTS_WIRE.BB`, and `BALLBLITZ.BB2` if it moves) with `blitz2-cd-update`, removing their #940 notes. Verify: `corpus_cd` and `corpus_cd_ab3` pass with the CD; the diff of the table is exactly those rows.
- [ ] 4.5 Docs: `docs/blitz2/deviations.md` row #940 (the narrowed rule), `docs/ab3/variant.md`, `TODO.md`; draft the issue comment (the fix of #940 is narrowed; labels and the closed state are the coordinator's) into a file; draft the text for the site pages `compared-with-original/new-errors.md` (End of procedure) and `fixed-bugs/compiler.md` (they need an update: `gendart` compiles again with a warning, only the string case stays an error) into a file for the coordinator. Verify: `fix-scope` (`cmake --build --preset amiga --target fix-scope`) is run and its result named in the report.
- [ ] 4.6 Table: update; expect `3d`, `darts_poly`, `darts_wire` and the `image.include` family to move on. Verify: the table diff.

## 5. The small G5 items

- [ ] 5.1 Probe `USELASTPATH` and `USEPATH2` against AmiBlitz3 (`ab3_cli_ref`: an include relative to the main source and to the last path), record the rule in the spec, implement in `ab3dir.c`/`ab3inc.c` (AB3 mode only). Verify: the cases pass in both compilers; `image.include.ab3` passes the word; BB2 mode unchanged.
- [ ] 5.2 Probe and implement `Dim ... KEEP` (the example that uses it as the first case). Verify: a run case with the same output as AmiBlitz3; the example compiles.
- [ ] 5.3 Probe and implement constants in `Data` lines (`Data.w #c`, the test program that fails today). Verify: the data bytes equal AmiBlitz3's; the test program compiles.
- [ ] 5.4 Probe and implement `Object` (the include that uses it). Verify: the wrapper of that include compiles past the word; AmiBlitz3's behaviour is recorded in the spec.
- [ ] 5.5 Probe the casts `(.type)` and list the forms AmiBlitz3 accepts. If the probe shows a large feature, split it into an own change and remove its requirement from this one, saying so in the report; otherwise implement. Verify: the cast cases against AmiBlitz3; the programs that use them compile.
- [ ] 5.6 Classify the unclassified differences of the survey: `window_SetPointer.ab3:104` (`NOT (value ROL 8)`), `Triton_toolmanager1.ab3:201` (`AddTags !ListSSCN{...}`), `Shared_Library.ab3:109` ("Label not Found" in `!lib_Init`), `#rawkey_# = $2A`, `_private_.b[#SIZEOF_ENSEMBLE]`, `sft\symbol[#@synth_Deinit]`, the "Constant Not Found" at the wrapper's `End` of `prefixtree` and `zlib`. Each gets a probe against AmiBlitz3 and a draft issue text (file), or is shown to be a probe artefact. No fix in this change. Verify: a file with one entry per item.
- [ ] 5.7 Table and docs: update the table, `docs/ab3/variant.md` (G5 section, "Differences"), `TODO.md` (item 3 and item 11), site `not-supported.md` and the pages of the new features. Verify: the table diff names only the sources of these items.

## 6. Site fix and closing

- [ ] 6.1 `site/content/docs/ab3-compared/not-supported.md`: remove `Null` from "Smaller additions" (it compiles as a library call, `ab3-mode-g4` proposal line 71) and update the list to what groups 2 to 5 left open. Verify: the site builds with hugo via podman and the page matches the compiler (a one-line `Null` case compiles).
- [ ] 6.2 Final check: clean build without warnings; `ctest --preset all`, `ctest --preset bug-orig`, `ctest --preset oracle`; the `prev` comparison names only group 4's results as changed; `openspec validate ab3-mode-g5 --strict`. Verify: the report lists the results and the table's before/after counts (examples, templates, tests, includes).

## Workflow follow-up

- Archive the change with the `openspec-archive-change` skill. This creates
  `testing/ab3-examples`, `ab3-compat/types`, `ab3-compat/assembler` and
  `compiler/procedure-end`, and adds requirements to
  `ab3-compat/directives`, `ab3-compat/syntax-rules` and
  `compiler/warnings`.
