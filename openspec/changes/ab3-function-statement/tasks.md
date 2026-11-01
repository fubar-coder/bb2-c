# Tasks

Prerequisite: `ab3-mode-g2` applied. Every commit: one topic, English
texts; `blitz2`'s results, `ted`'s output and all library files
byte-identical to the previous commit; clean build without warnings;
`ctest --preset host`; `ab3_bb2_equivalence*`, `ab3_cli`,
`ab3_cli_ref`, `ab3_tokens`, `corpus_cd_ab3`.

## 1. The rule

- [x] 1.1 Add an extension point in `handleproc` (`compile.c`) for a procedure with a return type called as a statement; `ext_bb2.c` keeps "Illegal Procedure Call", the AB3 implementation (`ab3syn.c`) compiles the call as for a `Statement` and discards the result (check against AmiBlitz3 what happens to a string result, e.g. the string buffer); verify: `blitz2`'s results byte-identical, `ab3_bb2_equivalence*` pass
- [x] 1.2 `ab3_cli` cases for the three scenarios of the `syntax-rules` delta (also in `ab3_cli_ref`), plus a function without parameters; verify: all pass with AmiBlitz3's output

## 2. Integration

- [x] 2.1 Add AmiBlitz3's test program `jsr` to `ab3_cli`/`ab3_cli_ref` (already fetched at `58a11658`); update the measured share and remove `jsr` from the open list in `docs/ab3/variant.md` and `TODO.md`; verify: `jsr` gives AmiBlitz3's output
- [ ] 2.2 Full check: clean build without warnings, `ctest --preset all` and `ctest --preset bug-orig` with the known verdicts

## Workflow follow-up

- Archive the change after review (`/opsx:archive`), syncing its spec.
