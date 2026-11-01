# Tasks

Prerequisite: change `blitz2-ab3-extension-points` applied. Every
commit: one topic, English texts; `blitz2`'s results (executables of
corpus, bug-orig and CD sources), `ted`'s output and all library files
byte-identical to the previous commit; clean build without warnings;
`ctest --preset host`; `ab3_bb2_equivalence*`; for AB3 code also
`ab3_cli`, `ab3_cli_ref`, `ab3_tokens`, `corpus_cd_ab3`. New AB3
behaviour only in `ab3*.c` behind extension points (D1).

## 1. Tokeniser component

- [x] 1.1 Add `shared/tokenizer` (D2): the object, AB3 rule set (case-sensitive whole words, word-start rules, `name$` first, first entry wins, strings/comments untouched, 250-character lines), m68k and host builds, host tests with every rule and broken input; verify: host tests pass, also with `-DHOST_SANITIZE=ON`
- [ ] 1.2 Use it in `ab3cli` for AB3-mode sources and text includes (D3), TED's `tokit` for BB2 mode; verify: `ab3_cli` passes, `exec_data` now gives AmiBlitz3's error (expect files of `exec_data` updated, entry removed from "Differences" in `docs/ab3/variant.md`)
- [ ] 1.3 Compare the tokens of the `ab3_cli` programs with AmiBlitz3's (`-save` in CLI mode or an equivalent dump; resolve the open question of design.md); verify: a ctest compares them, a changed token fails it

## 2. Directives

- [ ] 2.1 `OPTIMIZE` (D5): constant expression, flags replaced, reset per pass, no effect in skipped `CNIF`, value 2 refused, 1/8/16 accepted, the `setfpumode` call; verify: `ab3_cli` cases for each scenario of `ab3-compat/directives` OPTIMIZE requirements
- [ ] 2.2 `SYNTAX n` accepted with no code change; verify: a case where the executable with and without `SYNTAX 2` is the same
- [ ] 2.3 `RunErrsOff` stack, `LastRunErrsMode`, both errors; verify: `ab3_cli` cases for the scenarios
- [ ] 2.4 `BSS`/`Chip` sections (D6; tokens instead of the TAS/MOVEP data in `toklist.h`'s AB3 names); verify: the chip-memory case and the second-BSS error, hunk layout checked with `hunkcheck`, BB2 hunk layout unchanged

## 3. Syntax rules

- [ ] 3.1 New syntax: macro calls without braces; verify: both scenarios in `ab3_cli`
- [ ] 3.2 New syntax: labels without colon (`allowtab`) and labels of another procedure; verify: the scenarios in `ab3_cli`
- [ ] 3.3 `@@` line joining (parameter boundaries, macro text, multiple assignment, the include restriction); verify: the scenarios in `ab3_cli`
- [ ] 3.4 Global `SHARED`, `_`-identifiers, `.name` labels, the `+` prefix, `:@` in comments; verify: one `ab3_cli` case each
- [ ] 3.5 `#__include`, `#__debug`, `@#name` (also in `CNIF`); verify: the include-guard scenario in `ab3_cli`

## 4. Includes

- [ ] 4.1 AmiBlitz3's include rules in AB3 mode (D4): INCDIR + name, search order, extension fallback, type by name, refusal, repeated `INCLUDE`/`XINCLUDE`; the G1 other-mode check kept; update the `inctext_bbname` case and the "Differences" entry; verify: the scenarios of the modified `ab3-compat/source-mode` in `ab3_cli`

## 5. Test programs and integration

- [ ] 5.1 Add AmiBlitz3's test programs that G2 makes compile (hex_error, hexdigits, int_problem, int_tests, jsr, macro_test, memoryleak_test with its `OPTIMIZE 13` line, exec_data and unknown_function as expected errors; kick13/windowtags if headless) to `ab3_cli` and `ab3_cli_ref` (fetch-originals at `58a11658` with SHA-256); verify: all pass, differences to AmiBlitz3 recorded in `docs/ab3/variant.md`
- [ ] 5.2 Full check: clean build without warnings, `blitz2`'s results unchanged, `ctest --preset all` and `ctest --preset bug-orig` with the known verdicts; update `docs/ab3/variant.md` (G2 done, measured share of the 68 test programs) and `TODO.md`

## Workflow follow-up

- Archive the change after review (`/opsx:archive`), syncing its specs.
