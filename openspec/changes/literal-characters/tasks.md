# Tasks

Decided: no file format of its own, the reservation of the first byte
`$90`, the library `literallib` (design D2, D3, D7), the umlaut rule D4.
Decided too: a text save writes only text (D6).
Group 1 is analysis and changes no code. Every
later group is one commit (or several, one topic each). Every commit:
clean `cmake --build --preset amiga` without warnings, `ctest --preset
all` with a prev build (`cmake --preset amiga -DBLITZ2_PREV=ON`):
executables, library files and the files TED writes stay byte-identical
for every source without a literal character (`prev_results`, corpus
`prev`, `ted_ab` P5/Pc); differences only as named in the task. A change
of `tests/ab3` is checked with `ctest --preset oracle`. #374 is fixed
(design Context); group 2 starts from what it landed.

## 1. Checks before any code

- [x] 1.1 Token space: the AmiBlitz3 and CD check of design D2 is done
  (2026-11-11). Remaining: scan the corpus (`tests/corpus`, the Aminet
  archive) and every library the repository builds for tokens with the
  first byte $90 (the CD's userlibs: only `RIDOSLIB.OBJ`, done).
  Verify: the list in the commit message of group 3; a hit other than
  `ridoslib` (library 33 in the RWE variants, design D2) goes to the user.
- [x] 1.2 Read every place that walks a line pair by pair or clears bit 7
  in `programs/ted/src` and `programs/ted/src/core` (`c & 0x80`,
  `& 0x7F`: `fileio.c`, `lines.c:901,945`, `loadsave.c`, `services.c`,
  `tokit.c`, `lineedit.c`, `screen.c`, `reqs.c`, `mouse.c`) and the
  corresponding places in `programs/blitz2/src`, `shared/tokenizer`,
  `tools/lib/bbdetok`, `tools/lib/bblines`. Verify: a table (place, what
  it does with a pair, needed change) in the commit message of group 2;
  every place is a task below or "no change".
- [x] 1.3 Screen font: which glyphs the TED screen font (`topaz` 8 or the
  option's font) has for $80..$FF, and what the font's default glyph is.
  Verify: the answer in the commit message of group 2; if the font stops
  below $A0 the spec's rule for a missing glyph is checked on that range.
- [x] 1.4 Compiler audit and measurement: list every reader of string
  characters in blitz2-c (`litstring`, `fetchqasm`, `Data$`, `Dc.b`,
  `ab3chr.c`, `ext_strescape`, the include and macro scans) and the
  original's (`blitz2.s` `litstring` and its siblings); list every caller
  of `findtoke` (`compile.c:1426`, `eval.c:1881,2220`, `tokens1.c:214`,
  `asmlibs.c:1035,1156`, `libcall.c:832`) and what a word `$90cc` does in
  each (design D3: an error and no code); run a source with `$90 DF`
  inside a string and a comment through blitz2-c, `blitz2-ab3` and the
  original compiler (C5, emulator) and record the compiled bytes; run a
  raw $DF in an AB3 text source through `blitz2-ab3` today. Verify: the
  tables and the byte results in the commit message of group 4; a reader
  that does not give the byte `cc` is a task in group 4.
- [x] 1.5 What #374 landed (`15762513`, `826d8987`): re-read `readline`
  (`fileio.c:262-345`), `realsave` and Block Save (`loadsave.c:324`,
  `lines.c:827`) and `test_readline_latin1.cpp`; write down the points D6
  changes (the pair for every byte >= $80 but the six umlauts, with
  tokenise on and off; the literal as `cc` in a text save).
  Verify: the note in the commit message of group 2.

## 2. TED core: the stored form

- [x] 2.1 `findtoke` answers a word `$90cc` (cc >= $80) from the token
  list when it has the entry and otherwise from a constant table of 128
  one-character strings (design D3); a word `$90cc` with cc < $80 stays
  `tokeerr`. Verify: a host test (one program per test file) for every cc
  with `tokens == 0` and with a list that holds the 128 entries; `detok`
  and `detoka5` give the character; `ctest --preset host`.
- [x] 2.2 The tokeniser (`tokskip`) emits a character >= $80 as the pair
  in strings, comments and outside them, and keeps the six umlauts as the
  codes 26..31 (D4: a literal counts everywhere; guaranteed only in strings and comments); keyword matching unchanged (a name byte >= $80
  never matches, `tokit.c:106-114`: a host test with the 128 entries in
  the list). Verify: host tests: `a$="Gr"+$DF` and `é` inside and outside
  quotes, in a comment, at the end of a line, at the byte limit (64 and 65
  literals, D4a); a source without a byte >= $80 tokenises as in the `prev`
  build (compare over the corpus).
- [x] 2.3 Typing: a key that gives a character >= $80 other than the six
  umlauts stores the pair, with tokenise on and off (D6). Verify: host
  test of the keymap path and the line editor (cursor over the pair,
  delete, backspace, insert in the middle).
- [x] 2.4 `readline` (text files): a byte >= $80 becomes a pair or, for
  the six umlauts, the code 26..31; LF and TAB after such a byte keep
  their meaning; the tab column counts a pair as 1 (the #377 rule via
  `dispcol`). Tokenised files are loaded as stored (D4). Verify: host
  tests with the files of #374 (`28-long.drv`'s data, `Größe`, an umlaut
  at the end of a line, a Latin-1 byte before the end of the file).
- [x] 2.5 Display, colour, help, print: `sa_tokprint` draws a literal with
  `normcol` and no token flag; `prtfile_line` sends the byte and no ESC
  sequence; `help_core` finds nothing and counts one column (D3). Verify:
  host tests of `help_core` and `prtfile_line`; the TED A/B script of
  group 7 for the screen.
- [x] 2.6 Search, replace and blocks: confirm by tests that `isithere2`
  via `detoka5` finds a literal typed in the find string; Block Copy, Cut,
  Paste, Load and Save keep a pair at the first and last place of a
  block; `lines.c:901,945` as decided in 1.2. Verify: host tests.
- [x] 2.7 Join, split (#375), the line limit: a pair is never cut in two
  at the byte limit. Verify: host test at the limit.
- [x] 2.8 Save: a tokenised source is written as before (no header); a
  text save writes only text: a literal as `cc`, 26..31 as ä ö ü Ä Ö Ü,
  tokens as their names (D6, D7). Verify: a text save of a line with
  code 26 and a token gives `$E4` and the name; host tests: no literal gives the
  `prev` build's bytes (compare over the corpus), load and save of a file
  with literals is byte-identical, text round trip of a file with `ß`.

## 3. The library and the reservation

- [x] 3.1 `libs/acidlibs/literallib.c` and `LIB_literallib 33` in
  `libs/variants/bb2-2.10/libnums.h`, left out of the RWE variants
  (`#undef LIB_literallib` in `libs/variants/rwe-1999/libnums.h`, which
  the two sub-variants include, and `acidlibs_build`
  (`libs/acidlibs/CMakeLists.txt:43`) skipping `literallib` for the
  variants in a list `ACIDLIBS_WITHOUT_LITERAL`; design D2, D3): 128 `BLITZ_NULLSUB` entries (kind 0, no code) with the names
  `"\x80"`..`"\xFF"` (an X-macro or generated lines; no help text), a
  `BLITZ_LIBHEADER` without init and finit; in `ACIDLIBS_ORDER`
  (`libs/acidlibs/CMakeLists.txt`). Verify: the library builds, `blitzlib
  check` passes, `bbsrc tokens` lists `$9080`..`$90FF` with the names;
  `tokens_port` (see 3.2); the `acidlibs` of `rwe-1999` and its two
  sub-variants have no `literallib`, those of the other variants have it.
- [x] 3.2 The checks that compare with the original: `tokens_port`
  (`libs/CMakeLists.txt:76-91`) expects the original's list
  (`tests/data/ted-tokens.txt`) plus the 128 tokens (a second expected
  file or a generated one, named as an addition); the comparison of
  `acidlibs` with the CD's names the library as an addition; the command
  reference check (`CMDREF`, `libs/CMakeLists.txt:53-66`) gets the
  library's JSON or an exemption; `docs/libs/deviations.md` names the
  library (kind F: an addition). Verify: `tokens_port` passes and fails
  with one token removed (a test with a deliberately broken input);
  `ctest --preset all` with `-DPREV_INTENDED` naming `acidlibs`.
- [x] 3.3 The userlib file for the original Blitz 2: `literallib.obj` for
  `blitz2:userlibs/` from the same C source (`libs/userlibs/CMakeLists.txt`,
  a directory of its own or `rwe`'s pattern), in the results the
  distribution holds. Verify: `blitzlib check`; `bbsrc tokens` over the
  CD's compiler and the file lists the 128 tokens; the original compiler
  in Amiberry compiles a string with `$90 DF` with the file in
  `userlibs` and stops with "Syntax Error" for a literal as a statement.
- [x] 3.4 The reservation in the build: `blitzlib numbers` and `check`
  refuse a library numbered 32 and any library other than `literallib`
  numbered 33, except `ridoslib` in the `rwe-1999` variants (the check
  takes the exception as a parameter, so it holds for no other variant;
  `tools/blitzlib`, `tools/lib/biglib`; tests in `tools/tests/` with
  inputs from memory: a library with 32, one with 33, the literal library
  once, twice, `ridoslib` with and without the exception). Verify: the tool tests and the ctest
  `biglib_check_clash` pattern (`libs/CMakeLists.txt:94-110`) with a
  deliberately broken input.
- [x] 3.5 `ridoslib` keeps library 33 in `rwe-1999` and its sub-variants
  (`libs/variants/rwe-1999/libnums.h:12`, no renumbering) and these
  variants have no literal library (3.1); `rwe_numbers` and
  `rwe_declarations_ridoslib` stay as they are. `docs/rwelibs/README.md`
  §3.1 and `docs/rwelibs/port/ridoslib.md` say that `literallib` and
  RIDosLib cannot be in one library set and that the RWE variants have no
  literal characters. Verify: `ctest --preset host`, `ctest --preset
  emulator -R rwe` unchanged; the RWE variants' library files
  byte-identical to the `prev` build.

## 4. blitz2-c: the compiler

- [x] 4.1 Fix what the audit (1.4) found: string readers that do not give
  the byte `cc`, the length count of a literal in a quoted constant
  (`ab3chr.c:52`), a literal outside a string gives an error and no code
  (design D3, D5). Verify: tests of every reader with `tedstub` or
  `ab3cli` (strings, `Data$`, `Dc.b`, quoted constant, macro, comment);
  a stray literal as statement and in an expression ("Syntax Error"
  with the library loaded, "Token Not Found : $90xx." in AB3 mode); the
  expected bytes from the spec.
- [x] 4.2 An `Include` of a tokenised file with a literal compiles the
  byte (no code in `readinc`: `libcall.c`, `ext_bb2.c`, `ab3inc.c` are
  checked). Verify: a test in `tests/blitz2` with an include that holds a
  literal, run with blitz2-c and the original compiler.
- [x] 4.3 The six umlauts and `gs` unchanged: a regression test that a
  code 27 in a string gives $F6 and a pair `$90 F6` gives $F6. Verify: the
  test; `prev_results`: executables of the corpus byte-identical.

## 5. blitz2-ab3

- [x] 5.1 `shared/tokenizer` (rules AB3) and `ab3job`/`ab3cli`: bytes >=
  $80 inside a string and a comment of a text source become pairs
  (including the six umlauts, design D4); outside them as before (the loader,
  not `getcharb`).
  Verify: tests in `tests/ab3` that compile a string with ö and ß and
  compare the bytes with what AmiBlitz3's compiler gives (`ctest --preset
  oracle`); sources without such bytes unchanged; `ab3_tokens` still equal
  to AmiBlitz3's list.
- [x] 5.2 `docs/ab3/variant.md`: the section on source loading names the
  rule and the measurement of 1.4. Verify: the text names file and line.

## 6. Host tools

- [x] 6.1 `bbdetok` writes a literal as its byte; `tedsrc tok`/`detok`
  through the TED core; `bblines` test with a literal at the start and
  end of a line. Verify: tool tests per spec (round trip `detok` then
  `tok`, a literal at the end of a line).
- [x] 6.2 `bbsrc tokens` over the built `acidlibs` lists the library
  (spec `tooling/literal-characters`); `tokens-check.cmake` is covered by
  3.2. Verify: a tool test with the library's file.

## 7. Tests across programs

- [x] 7.1 TED A/B script (`tests/ted-ab/scripts/NN-literal.drv`): type
  `ß` and `é` in a string and a comment, save, reload, search, compile
  with the original compiler (C5) and blitz2-c (Cc). Verify: C5 equals
  Cc; R5 and Rc cannot type the characters: the pairs R5-C5 and Rc-Cc
  differ and are named in the script (`@fixed #374 R5-C5 Rc-Cc` or the
  number of the issue filed for this change); `ted_ab_lint` passes.
- [x] 7.2 A TED A/B script that loads a tokenised file with literals (made
  by 7.1's TED-C) in the original TED with the userlib and in TED-C:
  screen and save. Verify: load and save without loss (the saved file
  equals the loaded one); the screen of the original shows the
  characters; named differences (token colour) in the script.
- [x] 7.3 Corpus: one program with a literal in a string and one in a
  comment, in `tests/corpus`, expecting the string bytes. Verify: corpus
  runs Cc and, for the program that holds only strings, C5.
- [x] 7.4 The prev comparison: `ctest --preset all` with a prev build:
  `prev_results`, corpus `prev`, `ted_ab` P5/Pc unchanged for all
  scripts without a literal, and the new script, `acidlibs` (not the RWE
  variants') and the userlib file named in `PREV_INTENDED`. Verify: the report.

## 8. Docs

- [x] 8.1 The reservation of `$90` (libraries 32 and 33, "wherever
  possible": the RWE variants keep RIDosLib as 33 and have no literal
  library; the userlib `literallib.obj` works only if no other library
  numbered 33 is loaded before it, the loaders take the first match),
  where library numbers are documented: `docs/token-table.md` (the first bytes, the
  comparison tables), `docs/formats.md` §3.2 (the token word) and §1.4
  if it lists token ranges, `docs/libs/variants.md` (the library numbers
  of a variant), `libs/variants/*/libnums.h` (a comment at
  `LIB_literallib`), `docs/libs/deflibs.md` and `docs/libs/bb2-readme.md`
  where they list the numbers and files of the libraries. Verify: each
  text names `$90`, libraries 32 and 33, and the decision of 2026-11-11.
- [x] 8.2 `docs/formats.md` §3.2 (the pair), §4.1 (26..31 stay, the
  literal for the rest) and §4.2 (load and save); `docs/rwelibs/README.md`
  and `docs/ab3/userlibs.md` for RIDosLib (33 in the RWE variants, no literal
  library there). Verify: the
  texts name file and line of the code as the asserts do.
- [x] 8.3 `docs/ted/` (`deviations.md`, `changes.md`) and
  `docs/blitz2/`: the intended deviation (kind F) for #374 and the issue
  of this change; `docs/libs/deviations.md` for the new library (3.2).
  Verify: the rows.
- [x] 8.4 `TODO.md`: the entry for the change as a task list; #374's
  entry points to it. Verify: format `- [ ] **Title** ...`, no
  strikethrough.

## 9. Final checks

- [x] 9.1 `ctest --preset host`, `--preset all`, `--preset bug-orig`,
  `--preset oracle`; a build with `-DHOST_SANITIZE=ON` for the new host
  tests. Verify: no verdict changed except the intended ones.
- [x] 9.2 Manual run in Amiberry with `tools/amiga_run`: the original TED
  with the userlib loads a file with `ß`, `é` (the names of the library
  reach the screen as the characters; their colour; the cursor over a
  literal; save without loss); TED-C types `ß`, `é`, `ä` in a string,
  saves, reloads, compiles and runs with blitz2-c and the original
  compiler; the program prints the characters. Verify: a screenshot
  named in the report; if the original TED draws a name byte >= $80
  wrongly, the limit goes into design D8.

## Workflow follow-up

- Coordinator: the issue for this change (and the link from #374), the
  site entry (editor and compiler pages, a note on the reserved library
  numbers and the userlib), the label `fixed-in-port`, the issue closed
  after the push.
- Archive the change after acceptance.
