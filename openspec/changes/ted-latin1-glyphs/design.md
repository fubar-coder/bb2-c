## Context

Observed at `b0d1d1ee`:

- TED's fonts are raw 1-bit pictures, 128 px (16 bytes) wide, 8 strips of
  16 glyphs, `h` rows per strip: row r of glyph c is byte
  `(c & 15) + (c >> 4) * 16 * h + r * 16` (`archive/sources/ted-1.1/chars8.b`
  1024 bytes, `chars.b` 768 bytes; `docs/ted/reference.md:50-51`).
  Bit 7 is the left pixel; spacing is inside the glyph (cells touch).
  `fontcalc` (`programs/ted/src/core/keymap.c:66-76`) computes the
  pointer, with an `ORIG_HAZARD` for c >= 128; its two formulas are
  `(c >> 4) * 128` and `(c >> 4) * 96` (`high << 1` + `high << 2` with
  `high = c & 0xf0`), so they already give the right strip for every code
  up to 255. `print4` and `qprint` (`programs/ted/src/screen.c:651-684`)
  copy `fonthite` bytes and step 16 bytes per row. The data is embedded in
  `programs/ted/src/core/res.c:26-34` with `#embed` and `static_assert` on
  the sizes; `programs/ted/checkdata.tsv:7-8` compares the two arrays with
  the reference build.
- Contents of the originals: code 0 is a corner shape at 8 px and a
  single pixel at 6 px (measured); 1-25 are blank at 8 px and a single
  pixel at 6 px (the report of the research); 26-31 are the umlauts ä ö ü
  Ä Ö Ü (`germs`, `keymap.c:18-21`); 32-126 ASCII; 127 blank. 8 px: 2-px
  strokes, columns 1-6, capitals rows 0-6, x-height rows 2-6, baseline 6,
  descenders row 7. 6 px: capitals rows 0-4, mostly 7 wide, x-height rows
  1-4, descender row 5, 2-px strokes. The umlauts are not uniform: at 8 px
  `ä` = dots row 0, gap, unchanged `a` rows 2-6; `ö ü` dots row 1, gap,
  the letter squashed to 4 rows; `Ä Ö Ü` dots row 0, gap, a 5-row capital.
  At 6 px `ä ö ü` = dots, gap, a 3-row letter (`ä` only an arch), `Ä Ö Ü`
  two corner pixels in row 0 on a 4-row capital.
- `chmap` is one byte per cell; bit 7 marks a token's colour (`print3`,
  `screen.c:583-591`), so a character >= $80 cannot live there (design F2
  of `literal-characters`). A literal cell holds `TED_LITGLYPH` (0); only
  the cursor row knows the characters (`litrowmark`,
  `programs/ted/src/core/literal.c:31-33`). Sites that redraw cells from
  `chmap`: `refresh` (`programs/ted/src/input.c:163-180`),
  `scrollrite`/`scrollleft` (`screen.c:527-532`, `564-569`), `reline`
  (`input.c:399-404`), the last cell after a delete (`input.c:363`).
  Sites that draw a literal directly: `sa_tokprint`
  (`core/fileio.c:1097-1101`), typing (`input.c:275-281`, `gl_islit` in
  `core/lineedit.c:228-233`), `mouse.c:752-758`.
- The A/B runner decodes the screen with its own copy of the fonts:
  `teddump_load_fonts` reads `chars8.b`/`chars.b`
  (`tests/ted-ab/runner/teddump.c:153-168`), the decoder table is at
  `teddump.c:209-260`; an unknown cell is the middle dot.
- Existing rule for tools: logic in `tools/lib/`, the tool `main.c` in
  `tools/NAME/`, tests in `tools/tests/` with inputs from memory
  (`tools/CMakeLists.txt`); a build check belongs in the same build step
  as its result (`CLAUDE.md`).
- Candidate fonts were compared (research of 2026-11-13, not in the
  repository): unscii-8 (viznut, "Public Domain (or CC-0)", 8 x 8) is the
  only one close to TED's 8 px letters (38 of the 94 glyphs 33-126
  pixel-identical, 454 differing pixels, same strokes, columns, x-height,
  descender row and accent scheme); font8x8, kbd lat1-08 and the others
  are poor fits or have licence problems (Linux `font_8x8.c` is GPL-2.0
  only); no existing font has an 8 x 6 cell with 2-px strokes.
- On the Amiga, topaz 8 draws $80-$9F and $00-$1F with its "not found"
  glyph, a hollow box (KS 3.1: rows 1-6, columns 1-6, 2-px sides, 1-px top
  and bottom); `$A0` is the blank of `$20`; the console never draws
  $80-$9F. Dina (Aminet) has 1-px strokes and other cell sizes.

## Goals / Non-Goals

**Goals:**
- Every literal character TED-C can hold ($81-$FF) is drawn with a glyph
  of its own at both heights, in the colour of the text; the glyphs fit
  TED's letters.
- The glyph data is readable, reviewable text in the repository, with its
  provenance stated, and the fonts TED embeds are complete and checked.
- Redraws (scroll, refresh) show the same glyphs as typing.
- No file TED writes, no compiler output, no library file changes.

**Non-Goals:**
- Windows-1252 shapes for $80-$9F; $80 stays rejected (`literal-characters`
  decision 8 a).
- Changing glyphs 0-127 of the originals, except the 6 px glyph 26.
- A third-party font file in `archive/`; a copy of unscii in the repository.
- A different spacing model, another font height, a font setting at run
  time.

## Decisions

### D1 The glyph set

8 px: variant B of the research. $A1-$FF are unscii-8, adapted: the six
umlauts $E4 $F6 $FC $C4 $D6 $DC are TED's own glyphs 26-31; the other
accented letters with a one-row mark (grave, acute, diaeresis, plus the
`ë ï` and `Ë Ï`) are unscii's mark on TED's letter (lower case: TED's
letter rows 2-7 unchanged; capitals: the 5-row capital in rows 2-6, taken
from TED's `Ä Ö Ü` for A O U, hand-drawn for E I N Y); `ç Ç` are TED's
`c C` with unscii's cedilla; everything else is unscii. About 20 of the 96
glyphs differ from plain unscii (118 pixels). `$AD` (soft hyphen) is
drawn as a dash, as topaz does; the check is a task.

6 px: the drawn draft. Lower case is a one-row mark in row 0 on TED's
unchanged letter; capitals are a one-row mark on the 4-row capital (seven
hand-drawn); about 45 signs are drawn by hand. Circumflex, tilde and ring
are conventions, `¼ ½ ¾ § ¶` approximations; the look is reviewed on a real
screen (task 5.2). TED's `ä` (glyph 26) at 6 px is redrawn: the dots in
row 0 on TED's `a` in rows 1-4, like `à á â`. Glyphs 27-31 stay at 6 px;
all of 26-31 stay at 8 px.

$81-$9F: one glyph, a hollow box with 1-px lines (the convention of
topaz's not-found glyph, with thinner lines):
8 px rows `.######.` `.#....#.` (x5) `.######.` and a blank row 7;
6 px rows `.######.` `.#....#.` (x3) `.######.` blank row 5.
It differs from TED's 2-px `O`, `0`, `D`. $80 holds the
same box only because the font is complete; no cell shows it (a byte $80
is stored as `?`). $A0 is blank (as topaz and the console).

### D2 The data form: a text source, a tool, generated fonts, a check

Decided by the user (2026-11-13): the new glyphs are a readable text
source (`#` pixel, `.` blank), checked in; the repository's rule is that
archive material (the original glyphs 0-127) is not copied or edited, so
the complete 256-glyph fonts are generated in the build directory.

- Source: `programs/ted/fonts/glyphs8.txt` and `glyphs6.txt`. Format: a
  header of `;` comment lines (the source note, D3), `h=8` or `h=6`, then
  per glyph `@XX` (hex code) and `h` rows of 8 characters `#`/`.`. The 6 px
  file also holds `@1A` (the redrawn `ä`). Text is chosen over a C table
  or a `.b` blob because it is diffable and reviewable, and the strip
  layout is no business of a reviewer.
- Tool: `tedfont` (`tools/tedfont/main.c`, logic in `tools/lib/tedfont/`):
  `tedfont HEIGHT ARCHIVE.b SOURCE.txt OUT.b`. It copies the archive
  file's 128 glyphs, applies `@1A` if present, adds the glyphs $80-$FF in
  strips 8-15 and writes 256 glyphs (2048 bytes at 8 px, 1536 at 6 px).
  Tests in `tools/tests/test_tedfont.c` with inputs from memory.
- Check, in the same build step as the output (a second `COMMAND` of the
  custom command; a failure removes the output, so `ted` is not built):
  the output size is exact; every code $80-$FF is given exactly once; every
  glyph block has `h` rows of exactly 8 characters from `#.`; strips 0-7 of
  the output equal the archive file except the glyph 26 of the 6 px font;
  at 8 px glyphs 26-31 are the archive's. Every check has a test with a
  deliberately broken input (tool tests; one ctest that builds a broken
  source).
- TED: `res.c` embeds `ted_font8.b`/`ted_font6.b` from the build directory
  (`#embed`, `static_assert` on the sizes); the include path of `#embed`
  is extended. `checkdata.tsv` compares the arrays with the reference
  build: size is an intended difference, the first 1024 / 768 bytes stay
  compared, except 6 px glyph 26 (a `checkdata-intended.txt` entry).
- Alternatives dropped: C tables with binary literals (reviewable but the
  user preferred a blob; the build would hold no archive check) and a
  `.b` blob checked in (opaque in review).

### D3 Provenance

The header of both glyph sources says: unscii-8 by viznut
(`viznut/unscii`, public domain / CC0), URL, the pinned commit (task 1.1;
the studied copy's `unscii-8.hex` has the SHA-256 prefix `03094f7fbab7085c`),
which glyphs are unscii's unchanged, which are adapted to TED's letters
(and from which TED glyph) and which are drawn new (6 px: all of them,
except the reuse of TED's letters). No copy of `unscii-8.hex` goes into
`archive/` (decision of the user); `README.md` "License" gets one sentence.
TED's own glyphs are TED 1.1 material (`archive/README.md`) under the
project's licence situation as before.

### D4 Redraw: the character at a redraw site

The cell must tell its character where it is redrawn from `chmap`. Two
ways:

- **(a) A whole-screen literal map**: `chcols * edrows` bytes beside
  `chmap`, holding the character of a literal cell (0 elsewhere); it
  generalises the cursor row's `litrowmark` and `markof()`
  (`core/literal.c`). Every redraw reads `chmap` and, where the cell is
  `TED_LITGLYPH`, the map. It must follow every operation that moves or
  clears `chmap`: allocation and release (`screen.c:249`, `369`),
  `chmap_scrollup/scrolldown/eraseline/cls` (`core/tokit.c:425-493`),
  `sa_normprint` (`core/fileio.c:1079-1087`), and the sites that write a
  literal. Pro: a redraw needs no line; one memory operation per move;
  the redraw code is tiny. Con: a second array that can get out of step
  with `chmap`; every new `chmap` writer must remember it.
- **(b) A lookup in the line**: at a redraw the character is found in the
  row's line (from `thisitem`, counting columns as `litrow_sync` does).
  Pro: no state to keep in step. Con: the redraw depends on the line list
  being reachable and consistent at that moment (scroll inside a block
  operation, a line being edited), and costs a walk of the line per
  redraw.

Decided (user, 2026-11-13): (a). The same mechanism already runs for the
cursor row; the whole-screen version removes the special case, keeps the
original's redraw speed (no walk of a line per redraw), and the host
tests of group 4 check that the map and `chmap` agree after each
operation. A screen model that stores per cell the position in the line
instead of the character is a larger rework of TED, kept as an idea in
`IDEA.md`. Either way `TED_LITGLYPH` stays
the marker byte in `chmap`, and `print4`/`qprint` take the real code.

### D5 What reads the fonts

`fontcalc` serves every code 0-255 (its formulas need no change; only the
`ORIG_HAZARD` for c >= 128 goes). The A/B runner decodes with the same
generated files (the runner is given the build directory instead of the
archive directory). TED's `litrowmark`/`TED_LITGLYPH` marker is not drawn
any more when a character is known; a literal cell without a known
character draws glyph 0 (a safe fallback, never reached by a stored
pair).

## Risks / Trade-offs

- [The 6 px glyphs are a first draft and the accents are conventions] →
  task 5.2 looks at a real screen; the sources are text, so a change is a
  diff of a few lines.
- [Whole-screen map out of step with `chmap`] → host tests after every
  `chmap` operation (task 4.2); a debug-build check in the host tests that
  every `TED_LITGLYPH` cell has a non-zero map byte.
- [Redrawn glyph 26 at 6 px changes an original glyph] → intended and
  listed in `docs/ted/deviations.md` (kind B); TED A/B scripts that show
  an `ä` at 6 px are re-recorded and listed in the commit.
- [Provenance] → the unscii commit is pinned (task 1.1); unscii-8 is
  public domain / CC0, compatible with GPL-3.0-or-later.
- [`checkdata` and the reference build compare the fonts] → size is an
  intended difference, the original bytes stay compared.
- [A build that changes what TED draws but not what it writes] →
  `prev_results` and the `ted_ab` combinations `P5`/`Pc` stay the proof;
  nothing a file holds depends on a glyph.

## Migration Plan

No migration: TED-C draws more characters properly; stored files, the
original TED and the compilers are unaffected. A rollback is reverting the
commits of the group, since the font data is derived at build time.

## Open Questions

None. Decided by the user (2026-11-13): the redraw uses the whole-screen
literal map (D4a); the 6 px approximations (circumflex, tilde, ring,
¼ ½ ¾ § ¶) are acceptable, and the user's review of the screenshots of
task 5.2 is the gate.

## Findings

### The pinned unscii (task 1.1)
- `viznut/unscii` has the default branch `main` (the `master` of the raw
  URLs the research used is redirected). Pinned: commit
  `257b4dfeb50651510fe8b9cf4f151a148a4490e4` (2021-02-24, the head of
  `main` on 2026-11-13); `fontfiles/unscii-8.hex` was last changed by
  `a965767e3a594a8f760c2b5b003a224cd985aad9`. Its SHA-256 at the pinned
  commit is
  `03094f7fbab7085cf6a6b624cee61e47e71ce5d0c2f308c2f4436afdc17f776c`, the
  file the drafts were derived from: no differences.
- Licence text of the repository (`README.md`): "Licensing: You can
  consider it Public Domain (or CC-0) except for the files derived from or
  containing parts of Roman Czyborra's Unifont project (unifont.hex,
  hex2bdf.pl, unscii-16-full.*) which fall under GPL." `unscii-8.hex` is
  not one of them. URL for the header: `https://github.com/viznut/unscii`.
- Measured against the pinned file: of the draft's 95 glyphs $A1-$FF at
  8 px, 75 are unscii-8 unchanged (among them $C4 `Ä` and $E4 `ä`, which
  are TED's glyphs 29 and 26 too) and 20 are adapted (118 pixels): `È É Ë`
  (a 5-row capital E drawn for them), `Ò Ó Ö` (TED's `Ö`, glyph 30),
  `Ù Ú Ü` (TED's `U` rows 2-6, `Ü` glyph 31), `ç` (TED's `c`),
  `è é ë ù ú` (TED's `e`/`u` rows 2-7), `ì í ï` (the stem of TED's `i`),
  `ö ü` (TED's glyphs 27, 28). The glyph sources list them in their
  headers.

### The measured facts of the specs (task 1.2)
- `chars.b` (6 px): glyphs 0-25 are each a single pixel (row 0, column 0);
  127 is blank. `chars8.b` glyph 0 is the corner shape.
- The strip layout: the first `128 * h` bytes of a 256-glyph font in the
  layout `(c & 15) + (c >> 4) * 16 * h + r * 16` are strips 0-7, so the
  archive file is the first half of the font unchanged; `fontcalc`'s two
  formulas (`(c >> 4) * 128`, `(c >> 4) * 96`) give strips 8-15 for
  128-255. `print4`/`qprint` step 16 bytes per row.

### The tool (tasks 2.2-2.5, 4.4)
- `tedfont` follows the convention of `tools/README.md` rather than the
  `main.c` of D2: `tools/tedfont/main.cpp` on CLI11 with the subcommands
  `build` and `check` (the two commands of the build step), a README
  with its help (`help_tedfont`, `readme_tedfont`); the logic is C in
  `tools/lib/tedfont/`.
- The fonts are generated twice, by the same function
  (`programs/ted/fonts/fonts.cmake`): in the m68k build
  (`build/amiga/programs/ted/fonts/`, for `ted`) and on the host side
  (`build/amiga/host/ted-fonts/`, for the host tests of the TED core,
  which compile `res.c` too, and for the TED A/B runner). The host step
  runs first; a broken source stops the build there.
- The ctest of the real step on broken inputs (`tedfont_build_broken`,
  host side) runs the step's commands as `fonts.cmake` defines them, on
  copies of the sources broken in the build directory.

### TED (tasks 1.3, 3.1-3.3, 4.1, 4.2)
- `checkdata` compares the length of the reference range from the C
  symbol on: `_font8`'s first 1024 bytes are `chars8.b` unchanged, so it
  is equal and needs no entry (an entry would be stale and fail); only
  `_font6` (glyph 26, 5 bytes) is listed in `checkdata-intended.txt`,
  under #1018 (the change has no issue of its own).
- The sites of task 1.3 (`rg 'chmap|TED_LITGLYPH' programs/ted/src`) and
  their tests (`test_literal_redraw.cpp` unless named):
  - drawing a literal: `sa_tokprint` -> `sa_litprint` (`core/fileio.c`;
    "every row draws ..."); typing, `input.c` islit -> `printlit`
    (`screen.c`) after `gl_islit` ("typing, deleting and reline ...");
    `showmice` (`mouse.c`) -> `litnext` ("showmice ...");
  - redraws from `chmap`, all through `print3at` -> `cellglyph`:
    `refresh`, `reline`, the last cell after a delete (`input.c`),
    `scrollrite`, `scrollleft` (`screen.c`) ("horizontal scroll ...",
    "typing, deleting and reline ...");
  - moving or clearing `chmap`: `chmap_scrollup`, `chmap_scrolldown`,
    `chmap_eraseline`, `chmap_cls` (`core/tokit.c`) -> `litmap_*`
    ("the vertical scrolls ..."); allocation and release (`screen.c`
    `screenopen2`, `freechmap`) -> `litmap_init` (the host environment
    does the same, `stubs.c`);
  - other cells written: `sa_print`, `sa_normprint`, the token cells of
    `sa_tokprint` clear the map byte (`inStep` after every test step);
    `print2` writes no map byte: its only caller (`.isasc`) comes after
    `gl_isasc`, whose shift cleared it;
  - the cursor row's operations (`gl_*` in `core/lineedit.c`, `join`,
    `split`, `bcopyok`, `charstobuff`) work on the cursor row's part of
    the map through `litrow_*` and `litat` as before
    (`test_literal_edit.cpp`, "typing, deleting and reline ...").
- The redraw loops stay in the AmigaOS part as the original has them;
  the host tests check `cellglyph` of the cells each loop reads, which is
  what `print3at` draws.
- `showmice` with tokenise off draws a raw token byte >= $80 with the
  glyph of its code; the original read behind its font there (garbage),
  TED-C's glyph is a Latin-1 character. The label list shows only the
  label name, which rarely holds a token.
