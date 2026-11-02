# Tasks

Every commit: one topic, English texts; build without warnings;
`ctest --preset host`; `blitz2`'s results byte-identical for sources
without `#__debug` (corpus/bug-orig/CD executables before and after);
`ab3_bb2_equivalence*`, `ab3_cli`, `ab3_cli_ref`, `corpus_cd_ab3`. The
commit names the intended extension of `blitz2` (rule of 2026-11-01).

## 1. The constant

- [x] 1.1 Predefine `#__debug` in the core for both modes (`fetchconst` in `eval.c` and wherever `CNIF` evaluates constants; the value from the compiler's runtime-error/debugger option as `blitz2.s` keeps it, −1/0), and move the AB3 module's `#__debug` handling there (it keeps `#__include`); a source's own `#__debug` definition behaves as AmiBlitz3 (check: its constant list has the entry, so a redefinition is a "Constant already defined"-style error or not – match it in AB3 mode, choose the same in BB2 mode and note it); verify: `blitz2`'s results for the corpus/bug-orig/CD byte-identical, `ab3_*` pass
- [x] 1.2 Tests: BB2 mode (a corpus-style or bug-orig-style case compiled by `blitz2` with and without runtime errors: prints -1/0; `CNIF #__debug : Stop : CEND` compiles without runtime errors; a plain `Stop` still fails without them) and AB3 mode (`ab3_cli` cases with and without `-r`, also in `ab3_cli_ref` where AmiBlitz3 has the same values); verify: all pass

## 2. Docs and integration

- [x] 2.1 Document `#__debug` (README or `docs/blitz2/` on the compiler's predefined constants, `docs/ab3/variant.md`: the `int_tests` difference stays, now with the `CNIF #__debug` advice), `TODO.md` (the `Stop` rule item decided: BB2's behaviour plus `#__debug`); verify: docs name the test cases
- [x] 2.2 Full check: clean build without warnings, `ctest --preset all` and `ctest --preset bug-orig` with the known verdicts

## Workflow follow-up

- Archive the change after review (`/opsx:archive`), syncing its specs.
