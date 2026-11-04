# Tasks

**Rules for every commit:**

- One topic per commit, all texts in English.
- `blitz2`'s results are byte-identical to the previous commit: the
  executables of the corpus, bug-orig and CD sources, `ted`'s output
  and all library files.
- A clean build without warnings, and `ctest --preset host`.
- `ab3_bb2_equivalence*` passes. For AB3 code also `ab3_cli`,
  `ab3_cli_ref`, `ab3_tokens` and `corpus_cd_ab3`.
- New AB3 behaviour goes only into `ab3*.c`, behind extension points
  (D1).

## 1. Date patch for the tests

- [x] 1.1 Add `datestamp=YYYY-MM-DD` to `inject` (D7): the usage text, a `SetFunction` on `DateStamp`, the original restored at the end, a usage error for an invalid date. Check whether AmiBlitz3's `SystemDate` goes through `DateStamp` (a probe compiled by AmiBlitz3 under `inject datestamp`). Verify: a bug-orig-style self test of `inject`, where a program printing `DateStamp`'s days gives the patched value and an invalid date gives the usage error; the result for AmiBlitz3 is noted in design.md (Risks)
- [x] 1.2 Make `inject` usable in `tests/ab3`: `cli.cmake` copies it, the `ab3_cli_case` option `DATESTAMP <date>` and the case-line field, the header comment and `README.md` ("Tests") updated. Verify: an `ab3_cli` and an `ab3_cli_ref` case with `DATESTAMP` that runs a program printing the date through `Date$(SystemDate)` gives the patched date in both

## 2. Escape sequences in string literals

- [x] 2.1 Add the extension point `ext_strescape` (`litstring`, `ext_bb2.c` FALSE) and `ab3str.c` with the recogniser (D2): `\\hh`, `\\N`, `\\\\`, `\\Rddc`, "Unknown Escape Sequence" (D3), only in AB3 mode with new syntax. Verify: `ab3_cli` cases for the requirements "Escapes only with new syntax in AB3 mode", "An escape starts with two backslashes", "Byte and line feed escapes", "Repeat escape", "An unknown escape is an error"; `ab3_cli_ref` for the forms AmiBlitz3 shares, `REF_EXPECT` for `\\R` and the unknown forms; `blitz2`'s results unchanged
- [x] 2.2 `\\__THIS_FUNCTION` (from `thisproc`, `GLOBAL` outside; in a macro the expanding procedure) and `\\__VER_*` (from the header, `0` without). Verify: `ab3_cli` and `ab3_cli_ref` cases for both requirements, with the probe's values (`[st]`, `[GLOBALXY]`, `[1.2.3/42]`); the prefix case `\\__THIS_FUNCXYZ` and `\\__VER_XXXXX` as errors with `REF_EXPECT`
- [x] 2.3 `\\__DATE__` and `\\__DATE_GER__` (D6, day number to date in C). Verify: a host test of the date conversion (1978-01-01, leap days, 2000-02-29, 2026-10-25); `ab3_cli`/`ab3_cli_ref` cases with `DATESTAMP 2026-10-25` giving `[10.25.2026] [25.10.2026]`
- [x] 2.4 The line counter and the current include (D5): `\\__CODELINE` and `\\__THIS_INCLUDE`. Verify: `ab3_cli` cases for both requirements (header offset, after an include, back in the main source); `ab3_cli_ref` cases for the main source and for macro, `@@`, `CNIF` and repeated `XINCLUDE` lines; differences recorded per design.md (Risks)
- [x] 2.5 `\\__THIS_POINTER` refused (D3). Verify: the `ab3_cli` case of the requirement "The pointer escape is refused", `REF_EXPECT` with AmiBlitz3's output
- [x] 2.6 Document the escapes in `docs/ab3/variant.md`: the G3 section, and one "Differences" row each for `\\R`, the unknown and prefix forms, the empty values, `\\__THIS_POINTER` and `\\__THIS_INCLUDE` (I-59). Verify: every `REF_EXPECT` case of group 2 is named in a row

## 3. Dc.b

- [x] 3.1 Add the extension point `ext_dcbescape` (`dodc`) for the date and version escapes in AB3 mode, with or without new syntax (D2). Verify: the `ab3_cli`/`ab3_cli_ref` case of the requirement "Escapes in Dc.b" (`V1.2 [\\41]`); in BB2 mode the bytes are as written (a `hunkcheck` or byte compare of the data); the variant.md G3 section updated

## 4. Character literals

- [x] 4.1 Add the extension point `ext_element` for `@"…"` (D4): byte, word, long and float targets, the byte order in memory, `@""`, syntax error when too long and with new syntax off, "Can't Convert Types" for quick and string targets, an error in constant expressions. Verify: `ab3_cli` cases for both requirements, including the scenarios "Bytes in memory" and "Word in memory"; `ab3_cli_ref` for byte, word and long (also in memory), `REF_EXPECT` for float (a deviation row in variant.md); the open question of design.md (AmiBlitz3's quick text) answered there
- [x] 4.2 Check `@"…"` against the other `@` forms (`@#name`, `@@`) and AmiBlitz3's includes using it (e.g. `file.include`'s chunk IDs). Verify: an `ab3_cli` case that includes such an include and writes and reads a chunk ID; `ab3_cli` and `ab3_cli_ref` pass as before
  Done with the other `@` forms (case `chr_at`); the include case (`longlong.include.ab3`, prepared) is blocked by "Illegaly nested Condition" with nested macro calls (under analysis) and is a follow-up in `TODO.md` (user decision 2026-11-04).

## 5. Integration

- [x] 5.1 Remeasure AmiBlitz3's 68 test programs (the throw-away run of variant.md, "Measured share") and compile its `Sourcecodes/Includes` that use escapes or `@"…"`. Verify: the numbers are updated in variant.md, new compiling test programs are added to `ab3_cli`/`ab3_cli_ref` if they run headless, and each remaining difference is explained
- [ ] 5.2 Full check: a clean build without warnings, `blitz2`'s results unchanged, `ctest --preset all` and `ctest --preset bug-orig` with the known verdicts; `docs/ab3/variant.md` (G3 done) and `TODO.md` (item G3–G5: G3 done) updated

## Workflow follow-up

- Archive the change with `/opsx:archive ab3-mode-g3` after 5.2. This
  syncs `ab3-compat/strings` into `openspec/specs/`.
