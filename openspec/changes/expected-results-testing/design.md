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
`fixed-in-port`; closing comment after the push. The rule "results
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
