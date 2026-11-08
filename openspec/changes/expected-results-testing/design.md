# Design

## Context

Source: the test strategy inventory of 2026-11-07 (state of main
`bde98b2`, counts from `ctest -N` of `build/amiga`): preset `host` 63 tests
(+ the host subproject as one test `host`), `emulator` 87, `all` 150,
`oracle` 3, `bug-orig` 351 (350 `bug_orig_*` + fixture `rwe_refbuild`).
Issues: `bug-orig` 862 open / 97 closed, `fixed-in-port` 23 (all closed,
kinds K/T), `bug-port` 1 open / 5 closed. Motivation: see proposal.md.

### Evidence

1. **Fixed tests are unguarded.** Every `tests/bug-orig/*/ID.bas` gets the
   label `bug-orig` (`tests/bug-orig/CMakeLists.txt:220-224`); `host`,
   `emulator` and `all` exclude it (`CMakePresets.json`). About 20 tests
   come out FIXED today and run only in `ctest --preset bug-orig`:
   `datelib_day366` #120, `fontlib_lochar_127` #148,
   `arrayslib_newlist_overrun` #59, `multi_68010_vbr` #42, eight
   `*_messages_errarea`/`*_errarea` tests (blitzlibcheck fixes),
   `deflibs_cachedma_flags_d1` #959, `compiler_even_pad_byte*`,
   `compiler_data_pad_byte` #969, and the tests of the 23 `fixed-in-port`
   issues (`cliargs_parpath_cli` #108, `compiler_userlibs_doubleslash` #2,
   `compiler_wb_exit_permit` #337, `windowslib_taglist_ueberlauf` #329).
   Checked: `datelib_day366` is in the list of `bug-orig`, not in `all`.
2. **Two roles of the original.** As *oracle* (token numbers,
   declarations, TED's token list, AmiBlitz3 for `tests/ab3`) it defines
   BB2 and stays. As *reference of behaviour* (ref sessions, `libs_tests`
   pairs, `ted_ab` R combos, `checkdata`) it conflicts with fixes.
3. **The pattern exists.** Lists of intended differences with issue
   numbers: `libs/deflibs/declarations-intended.txt` (for
   `blitzlibcompare`), the `#969:` note row in `tests/data/blitz2-cd.tsv`,
   `cli/NAME.ref.expect` of `tests/ab3`, `; @known` of `ted_ab`.
4. **Fixes were absorbed by editing tests**: #120 removed days 1096 and
   -365 from corpus `t30_date.bas` (commit `fd76b11`) because `clib` must
   give `ref`'s output; #969 replaced one row of `blitz2-cd.tsv`
   (`8f1303e`). The corpus tests less with each fix.
5. **The runner** (`tests/corpus/runner/bugorig.h`, `bugorig.c:364-384`):
   pass with `config=ref` (original 1.95, or the CD's 2.10 for RWE tests)
   and judged configs `port`/`clib` (119 of the 120 `libs/` tests judge
   `clib` only via `; @configs clib`). Verdicts BROKEN (ref meets the
   expectation), FIXED, REPRODUCED (config = ref in the expectation's
   sections), DIFFERS; exit 0/1/3/4; ctest passes only when all judged
   configs are FIXED.

### What compares with the original today, and what happens at a fix

| Group | Check | Compares with | At a fix | Decision |
|---|---|---|---|---|
| A1/A2 | reference builds of blitz2 (`programs/blitz2/CMakeLists.txt:124-160`) and TED (`programs/ted/CMakeLists.txt:19-42`), vasm + listing verify | the original sources assembled | valid; consumers: bug-orig `ref`, TED oracle generator, `ted_ab` R5/Rc | stays; opt-in (not `ALL`) once recording exists |
| A3 | `checkdata` (`programs/ted/CMakeLists.txt:123-131`, `programs/ted/checkdata.tsv`, 534 entries) | data tables of `ted.sym` vs the reference build | fails at a fix that changes a table entry; the 22 TED bug-orig tests are logic bugs, most fixes will not touch it | intended list, at the first such fix |
| A4-A12 | checklabels, hunkcheck, checklink, checkcore, blitzlibcheck, biglib, cmdref, RWE residents/refbuild staging, `__saveds` | structure, not behaviour | untouched | unchanged |
| B1 | `declarations_acidlibs/deflibs` (`libs/CMakeLists.txt:60-76`) | CD declarations, `declarations-intended.txt` | has the mechanism (#959) | unchanged; optional dump into `tests/data` |
| B2 | `asmcmp_runerrlib/debuglib/dhandlerlib/memacclib` (`libs/acidlibs/CMakeLists.txt:68-83`) | CD routines, explicit routine list | fails at a fix inside a listed routine | per-routine list rule |
| B3 | `tokens_port`, `tokens_cd`, `tokens_broken` | `tests/data/ted-tokens.txt` | a fix adds no command | unchanged; `tokens_cd` to `originals` |
| B4 | `rwe_declarations_NAME` (28 host + 25 `emulator`; `libs/userlibs/rwe/CMakeLists.txt:201`) | CD / AmiBlitz3 SVN / refbuild binary | **no intended list passed** (`blitzlibcompare ${reffile} ${obj}`): first declaration change fails | `NAME.declarations-intended.txt` |
| B5 | `rwe_lesfont_cd` | CD binary at offset 3460 | untouched | `originals` |
| B7 | `ted_core_*` (10 snitch programs, 6 use `tests/data/ted-oracle.sha256`, SHA-256 of the original TED's detok/tokit2/load-save output for 715 CD sources, `test_tokit.cpp:2`) | original TED run under blitz2 1.95 | a fix on the detok/tokit/load-save path (`ted_detok_no_length_check`, `ted_tokit_reads_past_line`, `ted_readline_*`) changes the hash; regeneration reproduces the buggy hash | override rows with `#NNN`, generator keeps them |
| C1 | `libs_emulator` (17 snitch cases; 9 C test files acidlibs + 53 RWE userlibs; `sysload.h` loads NAME.obj and NAME.orig and compares results, registers, memory) | CD library | fails at the first fix a scenario sees; fixes so far did not hit it | `expect_fixed(NNN, value)` per check |
| C2 | corpus: `corpus` 68, `corpus_runerr` 131, `corpus_req` 3, `corpus_serial(_runerr)` (`tests/corpus/CMakeLists.txt:38-130`, `corpusrun`); configs `ref` (CD 2.10 + CD libs), `port` (blitz2-c + CD libs, exe byte-identical + output), `clib` (blitz2-c + C libs, output) | CD | fails at the first fix in its area unless a program is edited (#120) | committed expect files + `prev` exe config |
| C3 | `corpus_cd` (724 CD sources compiled; `tests/data/blitz2-cd.tsv`, `#969:` note rows), `corpus_cd_ab3` | the table made by the original 2.10 | N rows to replace by hand; the generator `blitz2-cd-reference` overwrites the port's rows | `blitz2-cd-update` keeps the note column |
| C4 | `ted_ab` (45 scripts, 4 combos R5/C5/Cc/Rc, 2 use `tedsave`, 1 `cd`, 13 `corpus`; `@known`) | original TED under both compilers | fails at the first visible TED fix | `; @fixed #NNN PAIR` + `P5`/`Pc` combos |
| C5 | `rwe_refbuild` + 17 `rwe_refbuild_NAME` + selftest | CD 2.10 compiles the RWE sources | untouched | `originals` |
| C6 | `rwe_residents_cd` | CD resident format | untouched | `originals` |
| C7 | `blitz2_*` (`tests/blitz2`, `ID.expect`, `ref` = blitz2 + CD libs, `clib`) | expectation, already | a library fix makes the CD-libs config differ | drop the CD-libs config |
| C9-C11 | `bug_port_963_*`, `lsclear_*`, `tests/ab3` (`ab3_bb2_equivalence`, `ab3_cli`, `ab3_tokenise`, `ab3_tokens`), `host_emulator` | expectations / AmiBlitz3 | untouched | unchanged |
| D | preset `oracle`: `ab3_cli_ref`, `ab3_cli_ref_broken*` | AmiBlitz3 v3.14.0's compiler checks our expect files (`cli/NAME.ref.expect`, `docs/ab3/variant.md`) | untouched | stays as it is; the model for the rest |

## Goals / Non-Goals

**Goals:**
- A fix never needs a test to be edited around it: the expected
  behaviour is committed, the difference from the original is visible
  with its issue.
- Every fixed bug is guarded by a test in a preset that is run as
  acceptance (`all`).
- `all` runs without the CD; the originals validate the committed data
  under their own label.
- "Byte-identical to the previous commit unless intended" is a test.

**Non-Goals:**
- A bug-preserving variant of the compiler or libraries (user decision).
- Fixing any bug (the fixes are the area rounds' work).
- The coverage work of `test-coverage` (only its test selection gets a
  small delta later).
- Changing the structure checks A4-A12 or the AmiBlitz3 comparisons.

## Decisions

**D1 - `; @fixed #NNN` in place (decision 1).** The header marks the
test; the directory stays `tests/bug-orig/` (history, globs, no renames).
The runner treats a pass without `ref` as "judge against the expectation
only"; the test gets the label `emulator` and not `bug-orig`; the issue
number says where it came from. Alternative considered: move to a
directory `fixed/` or to the expect suite; rejected (renames, no gain).
The ~20 passing tests are marked in step 1; `bugorig_verdict` (host)
gets cases without ref. The rule "a fix turns its test into a fixed test
in the same commit" goes into tests/README.md and CLAUDE.md.

**D2 - Recorded `ID.orig` (decision 2).** A recording run (ctest
`bug_orig_record_ID` under `originals`, or `BUGORIG_RECORD=1`) writes the
`ref` observation (five sections, raw) next to the test. The normal run
judges `port`/`clib` against `ID.expect` (FIXED) and `ID.orig`
(REPRODUCED/DIFFERS); BROKEN is `ID.orig` meeting `ID.expect`, checked at
parse time, so `bug_orig_selftest_broken` works on the recorded file.
Gains: 350 sessions fewer (about a third of the preset's 17 minutes),
bug-orig runs without the CD except tests that `mount=BB2` for data, the
original's behaviour is in the repository. Risk of nondeterministic bits
(addresses, dates, timing): today's `same_as` compares ref and port raw
as well, so the exposure is the same; the expectation's wildcards (`*`,
`...`) restrict what counts and a recorded file may be hand-edited with
them. TED bug-orig tests (`tedab` bugorig mode, R5/C5): the R5
observation (result + missed expectations) is recorded the same way.
REPRODUCED stays meaningful while 862 bugs are open; the six accepted
memory-layout DIFFERS of TODO section 2 stay DIFFERS. When few bugs
remain, `.orig` files can be dropped and the runner becomes a plain
expect runner. The `bug-port` label keeps its meaning (a port defect;
the fix removes the label).

**D3 - `expect_fixed(NNN, value)` (decision 3).** In `libs/tests`
(`sysload.h`), the pair comparison stays (best oracle for the unfixed
bugs for years); a check with `expect_fixed` asserts the correct value
for the C library and no longer requires equality with `orig`. No
scenario editing. Without the CD these values are the only assertions, so
every library fix writes one. A stale exemption (C library equals the
original) fails. The 8 snitch cases of `libs/tests/bugorig.c` need
nothing: a fix removes the hidden tag `[.bug-orig]` and the ctest label.
Alternatives rejected: editing scenarios (today's corpus practice, hides
behaviour); splitting the suite in "pairs with CD" and "golden values".

**D4 - Corpus expect files and `prev` (decision 4).**
`tests/corpus/expect/NAME.out` (~200 files for 68+131+3 programs and the
serial tests) generated once from today's `ref` run by a target
`corpus-expect-update`, reviewed with `git diff`; a runner mode
`expect_dir=` compares every config's output and serial bytes. The
executable comparison moves from the CD's compiler to the `prev` build:
config `prev|exe|<prev>/blitz2|<prev>/deflibs|<prev>/acidlibs`, present
when the prev build exists (D5). `corpus_cd` keeps the committed table:
`blitz2-cd-update` takes the port's `corpus_cd.tsv` and rewrites
`tests/data/blitz2-cd.tsv`, keeping the note column (issue, the CD's
hash); the table then is both the previous result and the documented
delta (the CD's sources remain a required input, skip without them).

**D5 - `prev` build.** Option `BLITZ2_PREV` (default `HEAD~1`, or a
build directory): `git archive` into `build/<preset>/prev/src`, an
ExternalProject of the Amiga side only (blitz2, blitz2-ab3, ted,
acidlibs, deflibs, variants, userlibs), results in
`build/<preset>/prev/blitz2/`. Not in the default configure (builds
twice). Consumers: corpus `prev` exe config, `ted_ab` combos `P5`/`Pc`,
host test `prev_results` comparing every library file byte by byte
(intended files via `-DPREV_INTENDED=ON` or a list). Replaces today's
manual worktree build of `HEAD~1`.

**D6 - Label `originals` (decision 5).** New label and preset
`originals` for everything that needs the CD or reference build: `ref`
configs of corpus and `ted_ab` (R combos), `rwe_refbuild*`,
`rwe_residents_cd`, `rwe_lesfont_cd`, `tokens_cd`, the recording of D2
and the generators. `oracle` stays AmiBlitz3's (D in the table), so a
preset "originals" does not mix two meanings. `all` then runs without
the CD; tests that need the CD's *sources* as input (`corpus_cd`, B7
oracle tests) still skip without them. A1/A2 become opt-in targets once
D2 exists (consumers only for regeneration).

**D7 - Override rows (decision 6).** `tests/data/ted-oracle.sha256` and
`tests/data/blitz2-cd.tsv` get a note column `#NNN`; the generators
(`ted-oracle`, `blitz2-cd-reference`/`blitz2-cd-update`) keep such rows
instead of the current rule "put the row back by hand". Alternative
(regenerate from the port) rejected: it loses the visible BB2 delta.

**D8 - Timing (decision 7).** Steps 1-5 of the order now (they remove
the immediate hazards); steps 6-10 per area at the first fix round of that
area. The intended lists of `checkdata`, `rwe_declarations_NAME` and the
`asmcmp` rule are built when the first fix of their area needs them, but
their format is fixed here (like `blitzlibcompare INTENDED`: each entry
must occur, a stale entry fails; each needs a test with a deliberately
stale and a missing entry).

**D9 - Rules rewritten.** CLAUDE.md "Do not fix original bugs" becomes
the fix workflow of the bug-fix phase. Each fix: issue (one per bug);
expected-behaviour test (`@fixed`); `BUG(orig)` becomes "Fixed (#N)";
`docs/*/deviations.md` entry as intended deviation (kind F); the website
page `site/content/docs/compared-with-original/fixed-bugs.md`; label
`fixed-in-port`; `Fixes #N` in the commit message (closes the issue on the push). The rule "results
byte-identical unless intended" stays. The same in
`.claude/agents/implementer.md` (its "REPRODUCED with the reference and
FIXED with our build" line, which presumes a live reference) and
`tester.md`; `tests/README.md` (presets table, bug-orig section "Adding
a test": "the result must be REPRODUCED" and "Look at the observation of
ref"), `docs/development.md`, `README.md` (line 5 "apart from documented
fixes" stays), `docs/libs/deviations.md` header rule ("bugs of the
reference are reproduced and documented, not fixed" is superseded).

## Risks / Trade-offs

- [Raw `.orig` observations hold nondeterministic bits] -> same exposure
  as today's `same_as`; wildcards and hand edits; review of the 350
  files; a recording run twice must give the same files before they are
  committed.
- [Corpus expect files generated from the original lock in original
  bugs the corpus happens to run] -> they are the expected output only
  where the original is right; a fix changes the file in its commit (the
  review of the first generation lists outputs suspect as a bug).
- [`prev` build doubles build time] -> only on request; skipped (77)
  otherwise, so the acceptance still needs it for a rework (a task runs
  it by hand until D5 exists).
- [`expect_fixed` values are hand-written golden values] -> each is
  checked against the pair's other check or the documented behaviour of
  the fix's issue, and the stale-exemption failure keeps them honest.
- [A `; @fixed` test passes with `ref` absent: a wrong expectation is
  not noticed] -> BROKEN is checked against `ID.orig` once recorded
  (D2); until then the test was REPRODUCED-checked when it was created.
- [Recording changes the meaning of `bug-orig` tests of RWE libraries
  that reference the CD's 2.10] -> the recording run uses the same ref
  as today; the file records which.

## Migration Plan

Order of the task groups (1-5 now, 6-10 per area): 1 `@fixed` and the
rule documents; 2 expect suite of Blitz-program tests; 3 intended lists;
4 `libs_emulator` `expect_fixed`; 5 `ted_ab` directive; 6 recorded
`.orig`; 7 corpus expect files, CD table, TED oracle rows; 8 `prev`
build and consumers; 9 `originals` label; 10 close-out and archive. No rollback needed:
every step is test infrastructure; a step that fails review is reverted
as a commit. After each step `ctest --preset host`/`all` and the
changed tests run; results of programs stay byte-identical (no program
code changes in this change).

## Open Questions

- The inventory's recommendation for a single label `oracle` was
  superseded by the user's decision for a new label `originals`; the
  preset name for `originals` (own preset, or label filter only) is
  open: this design assumes a preset of the same name.
- `fixed-in-port`: the inventory treated the label as a historical marker
  of the 23 pre-port issues (new fixes: closed `bug-orig` + kind F); the
  user's brief makes it part of every fix. This design follows the brief;
  whether the old 23 and the new ones are told apart (for example by the
  closing date) is open.
- Where the ~20 already FIXED tests get their `; @fixed` header: one
  commit, or each with its area; this design assumes one commit (task
  1.2).
- The test-selection delta (modifying `testing/test-selection` for
  `.orig` files and `tests/corpus/expect/` as inputs) cannot be written
  before `test-coverage` is archived; see task 9.3.

## Findings

- **Task 1.1 (2026-11-07):** a fixed test is judged in `clib` only and
  uses nothing of the CD: `port` needs the CD's libraries, the `BB2:`
  mount and `blitz2:defaultdbug` come from the CD as well, so with them
  no fixed test could run without the CD (spec: "No reference needed").
  A test that needs the CD names it in its `@pass` lines
  (`mount=BB2|@CD@`, `defaultdbug=@CD_BLITZ2@/DEFAULTDBUG`) and is
  skipped without it. The verdict without a reference is FIXED or
  REGRESSED (exit status 1). The 54 tests of task 1.2 come out FIXED
  without the CD and without `blitz2:defaultdbug`, also the two with
  `runtime_errors=on` (`compiler-even-pad-byte*`).
- **Task 1.2 (2026-11-07):** the full `ctest --preset bug-orig` run of
  main `d9eba7f` gave 54 passing bug-orig tests (all with a Blitz
  program; none of `libs/tests/bugorig.c` and none of TED). Evidence 1
  was inaccurate in two points: `windowslib_taglist_ueberlauf` is #317
  (not #329), and `cliargs_parpath_cli` is not FIXED (#107, ParPath$ of
  a CLI argument, still reproduced). `compiler_wb_currentdir` is FIXED
  by a DIFF (libnix's startup code restores the directory) while #21 is
  still open. The header line shifts the source by one line: the seven
  expectations with a compile error line (`compiler-incbin-leaks*`,
  `compiler-macro-uninit-free*`, `compiler-macrobuff-size-small`,
  `compiler-newtype-self-field*`) got the new line number.
- **Task 2.1 (2026-11-07):** the expect suite is a runner mode, not a
  new directory: the pass key `regression=on` now means an expect test
  without a reference (like `fixed=`, but without an issue: FIXED or
  REGRESSED, exit status 1; `defaultdbug=` optional), used by
  `tests/blitz2` and `tests/bug-orig/regression/`. Both keep their place
  and their header lines (`; @runerrs` there, the bug-orig header lines
  here); the `.expect` format is the same (`bugorig.h`). The old meaning
  (the expectation is what the reference does, BROKEN otherwise) and
  `bugorig_judge_regression` are gone. A port regression test runs
  `clib` only unless `; @configs` names others; with `@rwe` it gets no
  reference library and no fixture `rwe_refbuild`. The self test
  `regression_selftest_broken` (WANT 3, needed the reference) became
  `regression_selftest_wrong` (an expectation the port does not meet,
  WANT 1). With `archive/cd` absent, `blitz2_debug_off`,
  `blitz2_debug_on`, `blitz2_stop_off`,
  `regression_ricompactdisclib_scsi_after_plain` and
  `regression_selftest_wrong` pass (before: skipped without the CD).
- **Task 3.2 (2026-11-07):** an entry of `programs/ted/checkdata-intended.txt`
  is a whole check, named by its C symbol (unique in `checkdata.tsv`):
  `CSYM<TAB>#NNN<TAB>NOTE`, the note optional, a symbol at most once.
  Only a byte difference can be intended: a missing symbol or label
  still fails, and its entry is stale as well. Like the
  `declarations-intended.txt` files (and `rwe_declarations_NAME`'s of
  task 3.1) the list is found at configure time: after adding the file,
  configure again. The ctests `ted_checkdata_intended`,
  `ted_checkdata_unlisted` and `ted_checkdata_stale` run the tool on the
  real `ted.sym` with a deliberately wrong table entry of the build
  directory.
- **Task 4.1 (2026-11-07):** `expect_fixed(NNN, value)` is a value
  (`struct fixed`) handed to the check, not a call before it: every
  test program has its own `SAME` (and `fail`, `cur`), so `sysload.h`
  adds `SAME_FIXED(what, expr, expect_fixed(NNN, value))`, which uses
  the `cur` and `fail` of the program it is expanded in, and
  `check_fixed(what, fx, orig, c)` for the programs that compare with
  `check()` (systest.c, libtest.c). Verdict: C equal to the original
  is "stale" (checked first), C different from `value` is "wrong". The
  self test `selftest_fixed.c` compares values from memory, so its three
  ctests (`libs_expect_fixed`, `_wrong`, `_stale`) need no CD; the two
  failing cases are hidden (`[.selftest]`), their ctests pass on the
  printed failure line and the check counts (`PASS_REGULAR_EXPRESSION`,
  like the `*_broken` tests); the passing case also runs in
  `libs_emulator`. Checked by mutation: without the stale branch
  `libs_expect_fixed_stale` fails, without the wrong branch
  `libs_expect_fixed_wrong` fails.
- **Task 5.1 (2026-11-07):** `; @fixed #NNN PAIR...` is parsed by
  `tedab_test_parse` into `fixed[4]`, the issue per index of
  `tedab_pairs` (0 = the pair must be the same). A pair is named as the
  report names it (`R5-C5`, `Rc-Cc`, `R5-Rc`, `C5-Cc`; `C5-R5` or the
  uncompared `R5-Cc` are refused), at most once per script (one line per
  issue), and both its combinations must be in `@combos`: a pair that is
  never compared could never show the fix. `tedab_pair_verdict` turns
  the comparison into SAME/DIFF/FAIL, for a named pair into FIXED (it
  differs) or BROKEN (it is the same: stale); FAIL stays FAIL (a run that
  is not OK shows nothing). The report line reads
  `R5<->C5: FIXED #NNN (differs)` with the differences below it, or
  `R5<->C5: BROKEN (@fixed #NNN: the pair does not differ, stale)`; the
  summary counts `(N SAME, N FIXED)`. The bug-orig mode of the runner
  (`tests/bug-orig/ted`) parses the directive as well, but its exit
  status comes from the verdicts, not from the pairs; how a fixed TED
  bug-orig test is marked (without `ref`) is left to group 6.2. Tests:
  the `[fixed]` cases of `test_tedab` (parsing, refusals with their
  messages, verdicts, and two run directories compared by
  `tedab_compare_runs`: the same - BROKEN, one file changed - FIXED);
  ctests `ted_ab_lint_fixed` (`tests/ted-ab/lint/fixed/`, the sample of
  `tests/README.md`, accepted) and `ted_ab_lint_fixed_wrong`
  (`tests/ted-ab/lint/fixed-wrong/`, four scripts broken on purpose,
  each refused with its message). Checked by mutation: with a stale pair
  judged SAME, two `[fixed]` cases fail.
- **Task 6.1-6.4 (2026-11-07):** `ID.orig` is the observation as
  `bugorig_observe` makes it (the five sections, raw) behind a first
  comment line `# NOTE` that names the original (blitz2 1.95 built from
  `blitz2.s`, or the CD's 2.10 for `@rwe` tests); the TED file has
  `[state]` and `[problems]` (the R5 run's result and the expectations it
  misses). Pass keys `orig=`, `orig_by=`; a bug-orig pass without `orig=`
  is refused. `corpusrun`/`tedab` with `BUGORIG_RECORD=1` (ctest
  `bug_orig_record_ID`, label `originals`, new preset `originals`; `host`,
  `emulator`, `all`, `native` and `coverage` exclude the label) run the
  reference only and write the file, after a read-back check (a line of
  output that looks like a `[section]` header is refused); the normal run
  drops the reference configuration and what is only for it
  (`corpus_select`), checks `ID.orig` against `ID.expect` before any
  session (BROKEN, exit 3; a missing or unreadable file exit 2) and judges
  the rest with `bugorig_judge`. A line of a recorded file may be edited to
  a wildcard of the expectation's kind (`same_as` matches the way an
  expectation does). Without the CD the normal run does not judge `port`
  and runs without `blitz2:defaultdbug`; it is skipped (77) with runtime
  error checking (the debugger is part of the program), when the source
  names `BB2:` (the volume is mounted only then; before it was mounted
  for every test) and for the TED tests (C5 uses the CD's compiler). The
  mount changes the memory layout: `blitzcoplib-maxslice-zero` crashes as
  a time out with the volume and as a halt of the CPU without it, so it
  mounts it explicitly. The TED verdict has no run-directory comparison
  any more (the recorded state and problems are all of the original).
  Recording twice gave identical files for 276 of 277 tests; the original
  of `rishapeslib-read-palette-colour-count` (#820, a 768 KB wild write)
  ends as `halt` or as `done` with the guard message from run to run (the
  first state is committed; port and clib show `done`, `timeout`, or the
  guard line, so the verdict stays DIFFERS as with the live reference).
- **Task 8.1-8.4 (2026-11-08):** `BLITZ2_PREV` is `ON` (`HEAD~1`, the first
  parent of a merge), a revision, or the absolute path of a build
  directory; `cmake/prev.cmake` defines the ExternalProject `blitz2_prev`
  (part of ALL), whose build step `cmake/prev-build.cmake` takes `git
  archive` of the revision into `prev/src` (data git does not hold is
  linked in), configures with the commit's own toolchain file and builds
  everything of it. The revision is resolved on every build; when it
  moved (`HEAD~1` does with each commit) `prev/` is removed and built
  afresh, because the files of an archive carry the commit's time and a
  build directory of another commit would take them for unchanged.
  `prev_results` is `tools/prevcmp CUR PREV`: `blitz2/acidlibs`,
  `blitz2/deflibs` and every `.obj` below `libs/` and `variants/` (outside
  `CMakeFiles/`); `PREV_INTENDED` is `OFF`, `ON` (every difference) or a
  `;`-list of paths relative to the build directory, each of which must
  differ (stale entries fail); a file only the current build has is
  counted, not judged; exit status 77 (skipped) without a prev build.
  The corpus compares `prev` with `clib` (not with the reference): the
  config line is `config=prev|exe:clib|...`, where `exe:BASE` and
  `output:BASE` compare with the configuration BASE; the previous
  commit's compiler with its C libraries must write what the current one
  writes. `ted_ab` gets `P5` (previous TED + blitz2 2.10) and `Pc`
  (previous TED + previous blitz2, CD libraries as in the others), pairs
  `C5-P5` and `Cc-Pc`; with six runs per test a session's budget and limit
  are one and a half times as large. The corpus and `ted_ab` have no list
  of intended differences: a fix reports the `prev` lines that differ and
  the commit names them. Checked with a temporary commit that adds a nop
  before the final `rts` of every program (dropped): `corpus` reports
  `prev: EXE DIFFERS` for all 71 sources.
