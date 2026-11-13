# Tasks

Every task leaves `ctest --preset host` green; groups 4 and 5 also
`ctest --preset all` (needs Amiberry) against a prev build
(`cmake --preset amiga -DBLITZ2_PREV=ON`): executables of the compilers,
library files and the files TED writes stay byte-identical
(`prev_results`, corpus `prev`, `ted_ab` P5/Pc). The program `ted` changes
and is named in the commits. Group 1 changes no code.

## 1. Checks before any code

- [x] 1.1 Pin the unscii commit: fetch `viznut/unscii` (`unscii-8.hex`,
  SHA-256 prefix of the studied copy `03094f7fbab7085c`), record the
  commit hash, the licence text of the repository and the URL for the
  source header (design D3). Verify: the hash of the pinned file equals
  the one the draft glyphs were derived from, or the differences are listed.
- [ ] 1.2 Confirm the measured facts the specs rely on: glyph 0 at 6 px is
  one pixel (`chars.b` strip 0); `fontcalc` gives the right offsets for
  c >= 128 with the strip layout (`(c & 15) + (c >> 4) * 16 * h + r * 16`);
  `print4`/`qprint` (`programs/ted/src/screen.c:651-684`) step 16 bytes
  per row. Verify: a host test of `fontcalc` on a 256-glyph array (task 4.1).
- [ ] 1.3 List every site that draws or redraws a cell from `chmap` or
  passes a character to `print`/`qprint` (the ones of design D4, plus any
  found by `rg 'chmap|TED_LITGLYPH' programs/ted/src`). Verify: the list
  is in the commit of group 3 and every site has a test.

## 2. The glyph source and the tool

- [x] 2.1 `programs/ted/fonts/glyphs8.txt` and `glyphs6.txt` (design D1):
  header comment with the source note (D3), then `h=8`/`h=6` and one block
  `@XX` plus h rows of `#`/`.` per glyph, for $80-$FF (8 px: $80 = the
  box too, so the tool has a complete range; $81-$9F the hollow box, $A0
  blank, $A1-$FF from the drafts `variant-B-8px` / `variant-B-6px`) and, in
  the 6 px file, `@1A` (the redrawn ä). Verify: every glyph of the draft is
  present, the umlauts $E4 $F6 $FC $C4 $D6 $DC equal TED's 26-31 at 8 px;
  `$AD` is a dash; the file has 128 blocks (+1).
- [x] 2.2 The library `tools/lib/tedfont/` and the tool `tools/tedfont/`
  (`main.c`): parse the text source, read `chars8.b`/`chars.b`, write the
  1-bit strip-layout font of 256 glyphs (2048 / 1536 bytes), replacing
  glyph 26 where the source gives it (D2). Errors name file and line.
  Verify: a tool test (task 2.4) builds a font from inputs held in memory.
- [x] 2.3 The check in the same build step (D2): the output has the exact
  size; every glyph of $80-$FF came from the source (no gap, no duplicate,
  no unknown code, rows of exactly 8 characters, exactly h rows); strips
  0-7 equal the archive file except glyph 26 at 6 px; glyph 26 at 8 px and
  27-31 are unchanged. A failing check leaves no output file.
- [x] 2.4 `tools/tests/test_tedfont.c` (inputs from memory, registered in
  `tools/tests/CMakeLists.txt`): a good source; broken inputs (a missing
  glyph, a duplicate, a row of 7 or 9 columns, a wrong number of rows, a
  code outside $80-$FF, a glyph source that changes glyph 27, an archive
  file of the wrong size) each fail with their message. Verify: each
  broken input fails and writes no output.
- [ ] 2.5 `programs/ted/CMakeLists.txt`: a custom command that runs the
  tool into the build directory (`ted_font8.b`, `ted_font6.b`) with the
  check as another `COMMAND`, `DEPENDS` on tool, sources and archive
  files; the include path of `#embed` gets the build directory. Verify:
  the files are rebuilt after a change of a glyph and not otherwise; a
  broken source fails the build step and `ted` is not built.

## 3. TED

- [ ] 3.1 `res.c`: `font8` and `font6` embed the generated files;
  `static_assert` on 2048 and 1536 bytes. `fontcalc`: remove the
  `ORIG_HAZARD` for c >= 128 (the formula covers 0-255; the original
  note `ted.s:5214` stays). `checkdata.tsv`: the two entries are listed as
  intended differences in size, with the first 1024 / 768 bytes still
  compared (`checkdata-intended.txt`). Verify: host test of `fontcalc`
  (every code 0-255 gives a pointer inside the font; the strips 0-7 are
  the archive bytes); `ted_checkdata_*` tests pass.
- [ ] 3.2 The redraw design chosen in D4: the character reaches every site
  that draws a literal cell: `print4`/`qprint` callers `sa_tokprint`
  (`core/fileio.c:1097-1101`), typing (`input.c:275-281`, `gl_islit` in
  `core/lineedit.c:228-233`), `mouse.c:752-758`, and the redraws `refresh`
  (`input.c:163-180`), `scrollrite`/`scrollleft` (`screen.c:527-532`,
  `564-569`), `reline` (`input.c:399-404`), the last cell after a delete
  (`input.c:363`). `TED_LITGLYPH` stays the marker byte in `chmap`
  (D4). Verify: host tests of task 4.2.
- [ ] 3.3 `literal.h`/`literal.c`: the comments that say "every literal is
  drawn with glyph 0" describe the new rule; `docs/ted/reference.md` font
  rows (50-51) say 256 glyphs.

## 4. Tests

- [ ] 4.1 Host test `test_fontlit.cpp` (one program per file): for both
  heights every $A0-$FF has a glyph; $A0 is blank; $81-$9F are the box;
  the six umlauts at 8 px are glyphs 26-31; the 6 px glyph 26 has dots in
  row 0 and TED's `a` in rows 1-4; digest of the generated fonts
  (`sha256`) so an accidental change fails.
- [ ] 4.2 Host tests with literal characters in rows other than the cursor
  row: horizontal scroll right and left, `refresh`, `reline`, delete at
  the end of a row, mouse positioning; each checks the characters the
  screen routines draw, not `TED_LITGLYPH`.
- [ ] 4.3 `tests/ted-ab/runner/teddump.c`: `teddump_load_fonts` reads the
  generated fonts (the runner gets the build directory); the decoder table
  (`teddump.c:209-260`) maps a 256-glyph cell back to its character, an
  unknown cell stays the middle dot. `31-literal` and `32-literal-load`
  expect the characters as typed or loaded; one script at `@font 6`
  (`23-font6` or a new one). Verify: `ctest --preset all` with the
  scripts; the TED A/B expectations that change are listed in the commit.
- [x] 4.4 Tests of the tool's check are task 2.4; a ctest that runs the
  real build check on a deliberately broken text source in the build
  (`PASS_REGULAR_EXPRESSION`) shows the build step fails.

## 5. Acceptance

- [ ] 5.1 Clean `cmake --build --preset amiga` without warnings;
  `ctest --preset host` (also `-DHOST_SANITIZE=ON`: the tool and its
  tests); `ctest --preset all` with the prev build: no compiler output,
  library or TED file differs.
- [ ] 5.2 Look at the screen: 8 px and 6 px with a sample text of all
  $A0-$FF and the boxes, in the emulator (screenshots for the site, the
  design pass for the 6 px circumflex/tilde/ring and ¼ ½ ¾ § ¶, which are
  conventions and approximations, D1). Verify: the screenshots are in the
  report; the user reviews them.

## 6. Documents

- [ ] 6.1 `docs/ted/deviations.md`: the row "Literal characters" says the
  glyphs (the font has 256 glyphs, no longer glyph 0); a new row for the
  6 px glyph 26 (kind **B**, decided by the user 2026-11-13: an original
  glyph redrawn; TED-C's display only, no file differs).
- [ ] 6.2 `docs/ted/reference.md` (font rows), `docs/structure.md` if the
  tool or directory needs a line, `README.md` "License" (one sentence:
  glyphs adapted from unscii-8, public domain / CC0), `TODO.md` (done entry).
- [ ] 6.3 Report to the coordinator: the site text of #1018 (a screenshot of
  `Print "Größe é £ ½"` at both heights), the label, the issue of the
  change (an extension, no `fixed-in-port` workflow).
- [ ] 6.4 Sync `specs/editor/literal-characters`, `editor/screen-font` and
  `tooling/ted-font` into `openspec/specs` when archiving.
