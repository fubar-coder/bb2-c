## Why

TED's screen fonts have glyphs for the codes 0-127 only
(`archive/sources/ted-1.1/chars8.b`, 1024 bytes, 8 x 8; `chars.b`,
768 bytes, 8 x 6; embedded by `programs/ted/src/core/res.c:26-34`;
`fontcalc` reads behind the font for c >= 128,
`programs/ted/src/core/keymap.c:66-76`). The change `literal-characters`
(#1018) lets a source hold any Latin-1 character, but TED-C can show none of
them: every literal character is drawn with `TED_LITGLYPH`, glyph 0
(`programs/ted/src/core/literal.h:48`; design F3 of the archived change,
which left glyphs of TED-C's own as a later change, 2026-11-13). Glyph 0 is
the corner shape at 8 px and, measured, a single pixel at 6 px, so a `ß`,
`é` or `£` is a stray dot or a corner. The user (2026-11-13) wants real
glyphs for $A0-$FF in both font heights.

## What Changes

- **Glyphs.** TED-C draws every literal character with a glyph of its own:
  96 glyphs for $A0-$FF at 8 px and at 6 px; one replacement glyph, a
  hollow box with 1-px lines, for $81-$9F (C1 controls, which a text file
  can hold); $A0 is blank; no Windows-1252 shapes; $80 stays rejected (it
  is stored as `?`). At 8 px the set is unscii-8 (viznut, public domain /
  CC0) adapted to TED's letters (the six umlauts keep TED's own glyphs
  26-31); at 6 px the glyphs are drawn new in the style of TED's 6 px font.
- **Glyph 26 at 6 px.** TED's own `ä` at 6 px (dots, gap, a three-row
  letter) looks like an `ö` cut off at the bottom. It is redrawn: dots in
  row 0 on TED's unchanged `a` (rows 1-4), like the new `à á â`. The 8 px
  glyph 26 and glyphs 27-31 stay. This changes an original glyph on
  purpose (`docs/ted/deviations.md`).
- **Data form.** The new glyphs are a readable `#`/`.` text source in the
  repository (`$80`-`$FF`, and the 6 px glyph 26). A new host tool in
  `tools/` generates two complete 256-glyph fonts in the build directory
  from it and the unchanged archive files (`chars8.b`, `chars.b`), in the
  strip layout of the originals; TED embeds them with `#embed` and
  `static_assert`. A check in the same build step as the generation
  verifies size and completeness; a test feeds it broken input. The source
  note (unscii-8, licence, commit, which glyphs are unchanged, adapted or
  new) is the header of the text source; `archive/` gets no copy of
  unscii.
- **Redraw.** Where TED redraws a cell from `chmap` (horizontal scroll,
  `refresh`, `reline`) it needs the character, not the marker byte
  `TED_LITGLYPH`. The design compares a whole-screen literal map with a
  lookup in the line and recommends the map (design D4).
- `fontcalc` loses its `ORIG_HAZARD` for c >= 128 and reads the new
  strips; the A/B runner's decoder reads the same generated fonts.
- Docs: `docs/ted/deviations.md`, `docs/ted/reference.md`, the
  website text of #1018 (coordinator), the licence sentence of `README.md`.

This is an extension, not a fix of an original bug: it changes TED's
display only.

## Out of scope

- Any file TED writes, the compilers' output, the libraries, the line
  format: `prev_results`, the corpus configuration `prev` and the TED A/B
  combinations `P5`/`Pc` stay green and byte-identical (acceptance rule of
  `CLAUDE.md`). Only the program `ted` changes, and the commit names it.
- The original TED and the original fonts (`archive/` is not touched).
- Characters outside Latin-1, glyphs for $80 (not storable), Windows-1252.
- A font choice at run time, proportional spacing, other font heights.

## Capabilities

### New Capabilities

- `editor/screen-font`: TED-C's screen fonts have 256 glyphs per height,
  which glyphs the new ones are, and that the original 0-127 stay as the
  original drew them (except 6 px glyph 26).
- `tooling/ted-font`: the tool that generates the fonts from the text
  source and the archive files, and its checks.

### Modified Capabilities

- `editor/literal-characters`: the "default glyph" rule of "Display, help
  and colour" becomes "the glyph of its code".

## Impact

- `programs/ted/src/core/keymap.c` (`fontcalc`), `res.c` (the two
  `#embed`s), `literal.c`/`literal.h`, `programs/ted/src/screen.c`,
  `input.c`, `mouse.c`, `core/fileio.c`, `core/tokit.c` (the redraw
  sites, design D4), `programs/ted/CMakeLists.txt`,
  `programs/ted/checkdata.tsv`, a glyph source directory under
  `programs/ted/`.
- `tools/` (a new tool, its library in `tools/lib/`, tests in
  `tools/tests/`).
- `tests/ted-ab/runner/teddump.c` and the scripts `31-literal`,
  `32-literal-load`, `23-font6`; host tests of the TED core.
- Docs as above; `TODO.md`.
- Results: no executable of the compilers, no library, no TED output file
  changes; the program `ted` changes.
