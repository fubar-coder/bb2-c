# Tasks

Each task leaves `ctest --preset host` green (and `ctest --preset all`
where it touches the emulator tests). Build with `prev` for the
acceptance (`-DBLITZ2_PREV=ON`). The recording (group 4) needs the CD and
Amiberry; the rest does not.

## 1. The descriptions

- [ ] 1.1 `tests/corpus/data/t110_bounce.8svx.spec` and `t110_hitwall.8svx.spec` (design D2): VHDR + BODY, `bounce` one-shot only, `hitwall` one-shot + repeat, non-zero varying data. Verify: `mktestdata` writes both; `hexdump` shows FORM/8SVX/VHDR/BODY with the intended counts; the length fields are even (pad byte rule).
- [ ] 1.2 `tests/corpus/data/t111_song.mod.spec`: M.K., 3+ samples with data, song length 8, restart 127, highest pattern 1 below position 126, two patterns with notes, sample data behind them (design D2). Verify: size = 1084 + 2048 + sample bytes; byte 950 = 8, 951 = 127, 1080 = `M.K.`.
- [ ] 1.4 Verify the two sound files and the module with the libraries before wiring them in: a throwaway corpus run (`corpusrun pass` with the three programs' new paths, not committed) shows `t110`/`t111` outputs whose Sound fields and checksums match what the descriptions say.
- [ ] 1.3 `tests/corpus/data/t112_song.med.spec`: MMD0 with 4 samples (one null slot), 4 blocks of 4 tracks x 32 lines, song length 6 (`playseq` 0 1 2 3 1 0), tempo2 6, notes on all four tracks in the first twelve lines of every block, one `0C` row (design D2; note layout `medlib.c:1160-1180`). Verify: `modlen` field equals the file size; `blockarr` and `smplarr` entries point at the block and sample headers; a throwaway run of `t112` loads it and prints `loaded id 4D4D4430`, then notes in the `a`, `j`, `v` and `m` phases.

## 2. Build and staging

- [ ] 2.1 `tests/corpus/CMakeLists.txt`: build every `data/*.spec` to the build directory with `${MKTESTDATA}` (the loop of `tests/bug-orig/CMakeLists.txt:397-414`), target `corpus_data` in `ALL`; `corpus_pass()` argument `DATA` writes `file=data/NAME|PATH` lines; pass `DATA` to `corpus` and `corpus_runerr`; remove the `mount=BB2|...` line and its comment. Verify: the pass files in the build directory (`corpus.pass`, `corpus_runerr.pass`, `*_originals.pass`) hold the `file=` lines and no `mount=`; `corpus_req` and the serial passes hold neither; `cmake --build` writes the five files twice byte-identical (build twice, `cmp`).
- [ ] 2.2 `tests/corpus/check-data.cmake` (design D7), included at configure time, with the ctests `corpus_data_broken_source`, `corpus_data_broken_spec` (data under `tests/corpus/selftest/`, `PASS_REGULAR_EXPRESSION` naming the source or description) and `corpus_data_ok`. Verify: each broken tree fails with its message; the real tree passes; a rename of a spec in the working tree fails the configure.

## 3. The programs

- [ ] 3.1 `t110_audio.bas`, `t111_tracker.bas`, `t112_med.bas`: replace the `BB2:` paths by `CORPUS:data/...` (and the comments that say "from the CD", `t110_audio.bas:3`, `t111_tracker.bas:1`, `t112_med.bas:1`); no other change of logic; the second `LoadSound` of `t110` loads `hitwall`. Verify: `git diff` shows only paths and comments; every corpus source tokenises (`corpus_sources`); the `ab3` compile-equivalence tests (`tests/ab3`) still pass (they compile the same sources).

## 4. Recording

- [ ] 4.1 Make the CD available to the build of the worktree (`archive/cd` of the checkout, or `cmake/fetch-originals.cmake`); run `corpus-expect-record` with `CORPUS_RECORD_FILTER=corpus:t110_audio,corpus:t111_tracker,corpus:t112_med,corpus_runerr:t110_audio,corpus_runerr:t111_tracker,corpus_runerr:t112_med`. Verify: no "test not deterministic"; `git status` shows only the six `.out` files changed. If a program is not deterministic: find why (a pointer or timing value in the output) and change the program or the data, do not accept it.
- [ ] 4.2 Review the six files against the requirement "The generated modules give the player programs something to check": `t112_med.out` has notes in the `a`, `j`, `v` and `m` phases, different `pos block` values before and after the jump, the same `decoded` rows as the `d` phase expects from a relocated copy; `t111_tracker.out` has `cks` different from `silenced cks`; `t110_audio.out` shows the one-shot-only and the repeat sound. A weak output means a description to improve (back to group 1), not an accepted file. Verify: the three scenarios of the spec hold on the files.

## 5. Acceptance

- [ ] 5.1 `ctest --preset all` without the CD (move `archive/cd` away or use a build without it): the report of `corpus` and `corpus_runerr` lists no program as left out, `t110`, `t111`, `t112` pass, `prev_results` and the `prev` configuration are byte-identical (no executable, TED file or library changed apart from the three programs' own executables, which compile the same source in `prev` and `clib`). Verify: the pass reports.
- [ ] 5.2 `ctest --preset originals` with the CD: `corpus_originals`, `corpus_runerr_originals` pass for the three programs (`ref`, `port`, `orig-clib` against the new files); `git status` clean afterwards. Verify: all three configurations match.
- [ ] 5.3 `ctest --preset host` (also with `-DHOST_SANITIZE=ON` if tools changed: they do not), `ctest --preset bug-orig` unchanged, `cmake --build --preset amiga` has no warnings.

## 6. Documents

- [ ] 6.1 `tests/README.md` ("Corpus", `:455-456`: the sentence about leaving out the three programs; the corpus description mentions data from `tests/corpus/data`), `docs/testing.md` (the corpus row and the data), `docs/libs/cmdlibs-port.md` (t110 to t112 entries: "lambada.med from the CD, 700 ticks" becomes the generated module), `tests/corpus/runner/README.md` (the `mount=` example "BB2: = the CD, where the tests load sounds and modules from"), `TODO.md` (a done entry with the date; the item 9.3 text is a record of that day and stays). Verify: `rg 'BB2:' tests docs TODO.md` finds no statement that corpus programs load from the CD; text agrees with the specs.
- [ ] 6.2 Sync `specs/testing/expected-results` into `openspec/specs` when archiving; the `testing/original-comparisons` scenario "all without the CD" needs no change.
