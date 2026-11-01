# Tasks

Prerequisite: `ab3-mode-g2` applied. Every commit: one topic, English
texts; `blitz2`'s results, `ted`'s output and all library files
byte-identical to the previous commit; clean build without warnings;
`ctest --preset host`; `ab3_bb2_equivalence*`, `ab3_cli`,
`ab3_cli_ref`, `ab3_tokens`, `corpus_cd_ab3`.

## 1. The rule

- [x] 1.1 Add the extension point `ext_assigncheck` in `dolet` (D1) and its AB3 implementation; verify: `blitz2`'s results byte-identical, `ab3_bb2_equivalence*` pass
- [x] 1.2 Map our right-side types to AmiBlitz3's (D2) and add `ab3_cli` cases (also in `ab3_cli_ref`) for every scenario of `ab3-compat/type-rules`, including a typed constant and a function result; verify: all pass, each refused case gives AmiBlitz3's message, each accepted case runs with AmiBlitz3's output

## 2. Integration

- [x] 2.1 `exec_data` expect files: AmiBlitz3's error now; remove its "Differences" entry in `docs/ab3/variant.md` and add a note under G8 that NewType left sides and the `SYNTAX 4` part are still open; verify: `ab3_cli`, `ab3_cli_ref` pass
- [x] 2.2 Full check: clean build without warnings, `ctest --preset all` and `ctest --preset bug-orig` with the known verdicts

## Workflow follow-up

- Archive the change after review (`/opsx:archive`), syncing its spec.
