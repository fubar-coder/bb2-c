# Tasks

Nothing is decided until the user has answered the open questions of
`design.md` (Q1 to Q11); groups 2 to 8 start after that. Group 1 is
analysis and changes no code. Every later group is one commit (or
several, one topic each). Every commit: clean `cmake --build --preset
amiga` without warnings, `ctest --preset all` with a prev build
(`cmake --preset amiga -DBLITZ2_PREV=ON`): executables, library files and
the files TED writes stay byte-identical for every source without a
literal character (`prev_results`, corpus `prev`, `ted_ab` P5/Pc);
differences only as named in the task. A change of `tests/ab3` is
checked with `ctest --preset oracle`. Group 2 waits for #374 (the interim
rule) to be merged.

## 1. Checks before any code

- [ ] 1.1 Token space: scan the corpus (`tests/corpus`, the Aminet
  archive), the CD's `userlibs` and every library the repository builds
  for tokens with the first byte $90, and list the first bytes used by
  `bbsrc tokens` for the CD's libraries and AmiBlitz3 v3.14.0 (design D2).
  Verify: the list in the commit message of group 2; a hit stops the
  work (Q1).
- [ ] 1.2 Read every place that walks a line pair by pair or clears bit 7
  in `programs/ted/src` and `programs/ted/src/core` (`c & 0x80`,
  `& 0x7F`: `fileio.c`, `lines.c:901,945`, `loadsave.c`, `services.c`,
  `tokit.c`, `lineedit.c`, `screen.c`, `reqs.c`, `mouse.c`) and the
  corresponding places in `programs/blitz2/src`, `shared/tokenizer`,
  `tools/lib/bbdetok`, `tools/lib/bblines`, `tools/lib/bb22`. Verify: a
  table (place, what it does with a pair, needed change) in the commit
  message of group 2; every place is a task below or "no change".
- [ ] 1.3 Screen font: which glyphs the TED screen font (`topaz` 8 or the
  option's font) has for $80..$FF, and what the font's default glyph is.
  Verify: the answer in the commit message of group 3; if the font stops
  below $A0 the spec's rule for a missing glyph is checked on that range.
- [ ] 1.4 Compiler audit and measurement: list every reader of string
  characters in blitz2-c (`litstring`, `fetchqasm`, `Data$`, `Dc.b`,
  `ab3chr.c`, `ext_strescape`, the include and macro scans) and the
  original's (`blitz2.s` `litstring` and its siblings); run a source with
  `$90 DF` inside a string and a comment through blitz2-c, `blitz2-ab3`
  and the original compiler (C5, emulator) and record the compiled bytes;
  run a raw $DF in an AB3 text source through `blitz2-ab3` today.
  Verify: the table and the byte results in the commit message of group
  4; a reader that does not give the byte `cc` is a task in group 4.
- [ ] 1.5 What #374 landed: re-read `readline`, the text save and the
  interim rule after #374 is merged; write down the points D6 changes.
  Verify: the note in the commit message of group 2.

## 2. TED core: the stored form

- [ ] 2.1 `findtoke` answers a word `$90cc` (cc >= $80) with the
  one-character name from a constant table; a word `$90cc` with cc < $80
  stays `tokeerr` (design D3). Verify: a host test (one program per test
  file) for every cc; `detok` and `detoka5` give the character; `ctest
  --preset host`.
- [ ] 2.2 The tokeniser (`tokskip`) emits a character >= $80 as the pair
  in strings, comments and outside them, and keeps the six umlauts as the
  codes 26..31 (Q2, Q3); keyword matching unchanged. Verify: host tests:
  `a$="Gr"+$DF` and `é` inside and outside quotes, in a comment, at the
  end of a line, at the byte limit (64 and 65 literals, Q7); a source
  without a byte >= $80 tokenises as in the `prev` build (compare over
  the corpus).
- [ ] 2.3 Typing: a key that gives a character >= $80 other than the six
  umlauts stores the pair, with tokenise on and off (D6). Verify: host
  test of the keymap path and the line editor (cursor over the pair,
  delete, backspace, insert in the middle).
- [ ] 2.4 `readline` (text files): a byte >= $80 becomes a pair or, for
  the six umlauts, the code 26..31; LF and TAB after such a byte keep
  their meaning; the tab column counts a pair as 1 (the #377 rule via
  `dispcol`). Verify: host tests with the files of #374 (`28-long.drv`'s
  data, `Größe`, an umlaut at the end of a line, a Latin-1 byte before
  the end of the file).
- [ ] 2.5 Display, colour, help, print: `sa_tokprint` draws a literal with
  `normcol` and no token flag; `prtfile_line` sends the byte and no ESC
  sequence; `help_core` finds nothing and counts one column (D3). Verify:
  host tests of `help_core` and `prtfile_line`; the TED A/B script of
  group 7 for the screen.
- [ ] 2.6 Search, replace and blocks: confirm by tests that `isithere2`
  via `detoka5` finds a literal typed in the find string; Block Copy, Cut,
  Paste, Load and Save keep a pair at the first and last place of a
  block; `lines.c:901,945` as decided in 1.2. Verify: host tests.
- [ ] 2.7 Join, split (#375), the line limit: a pair is never cut in two
  at the byte limit. Verify: host test at the limit.

## 3. TED-C: BB23 load and save

- [ ] 3.1 `shared/xtra` and `tools/lib/bb22`: `XTRA_T_BB23`, the BB23
  reader with the refusals of the spec (design D7, D9); the reader is
  strict and gets its tests with deliberately broken input (truncated
  pair, version 2, word at +10, LF in a line, second byte below $80, a
  file that is only `BB23`). Verify: `xtra` and `bb22` unit tests; the
  existing `xtra_originals` and `bb22_rwelibsrc` unchanged.
- [ ] 3.2 TED-C load: recognise `BB23` (the same code as 3.1, linked into
  TED-C's loader or a shared function in `shared/xtra`), skip the header,
  read the lines, restore or ignore the cursor (Q6); a refused header is
  reported with the requester and nothing is loaded. Verify: host test
  with a `BB23` file and with the refused cases; round trip load and
  save is byte-identical.
- [ ] 3.3 TED-C save: write the header iff a line holds a pair with the
  first byte $90 (design D7; the cursor fields from the current state);
  a text save writes `cc`. Verify: host tests: no literal gives the
  `prev` build's bytes (compare over the corpus), one literal gives the
  header, deleting the last one removes it; `.bak` and `.xtra` handling
  unchanged.
- [ ] 3.4 The warning of Q11 if wanted. Verify: test of the requester.

## 4. blitz2-c: the compiler

- [ ] 4.1 Fix what the audit (1.4) found: string readers that do not give
  the byte `cc`, the length count of a literal in a quoted constant
  (`ab3chr.c:52`), a literal outside a string gives a syntax error and
  nothing else (design D5). Verify: tests of every reader with `tedstub`
  or `ab3cli` (strings, `Data$`, `Dc.b`, quoted constant, macro,
  comment); the expected bytes from the spec.
- [ ] 4.2 `readinc` (`libcall.c`, `ext_bb2.c`, `ab3inc.c`): skip the
  `BB23` header of an include. Verify: a test with an include that holds
  a literal (a bug-orig-style test in `tests/blitz2`, with the original
  not able to read it, named so).
- [ ] 4.3 The six umlauts and `gs` unchanged: a regression test that a
  code 27 in a string gives $F6 and a pair `$90 F6` gives $F6. Verify: the
  test; `prev_results`: executables of the corpus byte-identical.

## 5. blitz2-ab3

- [ ] 5.1 `shared/tokenizer` (rules AB3) and `ab3job`/`ab3cli`: bytes >=
  $80 inside a string and a comment of a text source become pairs
  (including the six umlauts, design D4); outside them as before (Q9).
  Verify: tests in `tests/ab3` that compile a string with ö and ß and
  compare the bytes with what AmiBlitz3's compiler gives (`ctest --preset
  oracle`); sources without such bytes unchanged.
- [ ] 5.2 `docs/ab3/variant.md`: the section on source loading names the
  rule and the measurement of 1.4. Verify: the text names file and line.

## 6. Host tools

- [ ] 6.1 `bbsrc bb23`, `bb23-info` (`tools/bbsrc`, `cmd_bb22.cpp` as the
  model) and `tools/xtra` naming the type; `bbdetok` writes a literal as
  its byte; `tedsrc tok`/`detok` through the TED core (written `BB23` iff
  a literal occurs). Verify: tool tests per spec (convert, broken input,
  round trip `detok` then `tok`); `bblines` test with a literal at the
  start and end of a line.
- [ ] 6.2 `bbsrc tokens` (Q10): list the pair or print a line naming the
  reserved first byte. Verify: `tokens-check.cmake` updated and passing.

## 7. Tests across programs

- [ ] 7.1 TED A/B script (`tests/ted-ab/scripts/NN-literal.drv`): type
  `ß` and `é` in a string and a comment, save, reload, search, compile
  with the original compiler (C5) and blitz2-c (Cc). Verify: C5 equals
  Cc; R5 and Rc cannot read the saved file: the pairs R5-C5 and Rc-Cc
  differ and are named in the script (`@fixed #374 R5-C5 Rc-Cc` or the
  number of the issue filed for this change); `ted_ab_lint` passes.
- [ ] 7.2 Corpus: one program with a literal in a string and one in a
  comment, in `tests/corpus`, expecting the string bytes. Verify: corpus
  runs Cc and, for the program that holds only strings, C5.
- [ ] 7.3 The prev comparison: `ctest --preset all` with a prev build:
  `prev_results`, corpus `prev`, `ted_ab` P5/Pc unchanged for all
  scripts without a literal, and the new script named in `PREV_INTENDED`.
  Verify: the report.

## 8. Docs

- [ ] 8.1 `docs/formats.md`: §3.2 (the pair), §4.1 (26..31 stay, the
  literal for the rest), §4.2 (load and save), a new section for `BB23`
  with the header table, and §1.4 (`XTRA_T_BB23`); `docs/rwelibs/README.md`
  §4.1 gets a pointer; `docs/token-table.md` names `$90`. Verify: the
  texts name file and line of the code as the asserts do.
- [ ] 8.2 `docs/ted/` (`deviations.md`, `changes.md`) and
  `docs/blitz2/`: the intended deviation (kind F) for #374 and the issue
  of this change; `docs/libs/deviations.md` if a library file is touched
  (none expected). Verify: the rows.
- [ ] 8.3 `TODO.md`: the entry for the change as a task list; #374's
  entry points to it. Verify: format `- [ ] **Title** ...`, no
  strikethrough.

## 9. Final checks

- [ ] 9.1 `ctest --preset host`, `--preset all`, `--preset bug-orig`,
  `--preset oracle`; a build with `-DHOST_SANITIZE=ON` for the new host
  tests. Verify: no verdict changed except the intended ones.
- [ ] 9.2 Manual run in Amiberry with `tools/amiga_run`: type `ß`, `é`,
  `ä` in a string, save, reload, compile and run with blitz2-c and the
  original compiler; the program prints the characters. Verify: a
  screenshot named in the report.

## Workflow follow-up

- Coordinator: the issue for this change (and the link from #374), the
  site entry (editor and compiler pages, a note on the `BB23` format),
  the label `fixed-in-port`, the issue closed after the push.
- Archive the change after acceptance.
