# Tasks

Every commit: one topic, English texts; the results of `blitz2`
(executables of corpus, bug-orig and CD sources), `ted`'s output,
`acidlibs`, `deflibs` and all library files byte-identical to the
previous commit; clean rebuild without warnings; `ctest --preset host`;
`ab3_bb2_equivalence*`; `checklink` for both programs; for commits
touching AB3 code also `ab3_cli`, `ab3_cli_ref`, `ab3_tokens`,
`corpus_cd_ab3`. The commit names the rework (rule of 2026-11-01).

## 1. Shared core and the extension header

- [x] 1.1 Add `programs/blitz2/src/ext.h` with the extension points of today's `#if` blocks (D2) and `ext_bb2.c` with their BB2 behaviour; move the mode-dependent data (`progmode`, `tokens`, `mathq`) behind it; verify: `blitz2` builds and its results are byte-identical, `ab3_*` pass
- [ ] 1.2 Build the chunk files and glue once as the object library `blitz2_core`, linked by `blitz2` with `ext_bb2.c` and by `blitz2-ab3` with the AB3 modules; drop the define parameter of `blitz2_program`; verify: both programs link, results byte-identical, `ab3_*` pass

## 2. AB3 blocks into extension points, one area per task

- [ ] 2.1 Libraries: `makeab3libs` (main.c), the BigLib/userlib paths of `loadlibs`/`loadblitzlibs` (asmlibs.c) → `ext_makelibs`/`ext_loadlibs`, AB3 code and data in `ab3libs.c`; verify: no `BLITZ2_AB3` left in those parts, `ab3_cli` (`nolib`, `userlib`) passes, `blitz2` results byte-identical
- [ ] 2.2 Residents and the library error of `compile` → `ext_loadres`, `ext_compilestart`, code in `ab3res.c`; verify: `ab3_cli` (`resident*`, `noresident`) passes, `blitz2` results byte-identical
- [ ] 2.3 Includes (`doinc2`, `readinc`, `closeinc`, `freeincs`) → `ext_incopen`/`ext_readinc`/`ext_incline`/`ext_incdone`, code in `ab3inc.c`; verify: `ab3_cli` (`inc*`) passes, `blitz2` results byte-identical
- [ ] 2.4 Errors and tokens (`ab3_errpos`, `ab3newtoken`, the AB3 token image) → `ext_errpos`/`ext_newtoken`, code in `ab3tok.c`; verify: `ab3_tokens` and `ab3_cli` pass, `blitz2` results byte-identical
- [ ] 2.5 Command line and editor (`main`, `begin`) → `ext_cmdline`/`ext_editor`/`ext_result`, `ab3main.c` keeps the command line; verify: `grep BLITZ2_AB3` finds nothing under `programs/blitz2/src`, all AB3 tests pass, `blitz2 -s x -e y` behaves as before (spec `ab3-compat/bb2-equivalence` "No AB3 mode in blitz2")

## 3. Check and rule

- [ ] 3.1 Add the build check of D4 with a test on broken input; verify: the check passes on the sources, the broken-input test fails as expected
- [ ] 3.2 Document the structure (core, `ext.h`, `ext_bb2.c`, `ab3*.c`) and the rule for G2 on in README ("blitz2-ab3") and `docs/ab3/variant.md`; verify: the documented file list matches `programs/blitz2/src`

## 4. Integration

- [ ] 4.1 Full check: clean rebuild without warnings; `blitz2`'s executables for corpus, bug-orig and CD sources byte-identical to the state before the change; `ctest --preset all` and `ctest --preset bug-orig` with the known verdicts

## Workflow follow-up

- Archive the change after review (`/opsx:archive`), syncing the modified requirement into `openspec/specs/ab3-compat/bb2-equivalence/spec.md`.
