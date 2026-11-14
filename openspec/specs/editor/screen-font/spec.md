# editor/screen-font Specification

## Purpose
Says which glyphs TED-C's screen fonts (8 x 8 and 8 x 6) have: the 128 of
the original TED, and 128 more for the codes $80-$FF, so that every
literal character is drawn as itself.

## Requirements

### Requirement: Both fonts have 256 glyphs
TED-C SHALL draw with a font of 256 glyphs at each height, in the strip
layout of the original (16 glyphs per strip, row r of glyph c at byte
`(c & 15) + (c >> 4) * 16 * h + r * 16`, `h` = 8 or 6), so `fontcalc`
serves every code 0-255 and TED-C SHALL NOT read behind the font. Glyphs
0-127 SHALL be the original's, except glyph 26 at 6 px.

#### Scenario: Size
- **WHEN** TED-C is built
- **THEN** the 8 px font has 2048 bytes and the 6 px font 1536 bytes

#### Scenario: The original glyphs
- **WHEN** the first 128 glyphs of the 8 px font are compared with `chars8.b`
- **THEN** they are identical

### Requirement: Glyphs for $A0-$FF
The glyphs for $A1-$FF SHALL show the Latin-1 character of the code, in the
style of the font's own letters (stroke width, x-height, baseline,
descender row). $A0 SHALL be blank. The six umlauts $E4 $F6 $FC $C4 $D6
$DC SHALL be TED's own glyphs 26-31 at 8 px and, at 6 px, TED's umlaut
scheme (`ä` as below, the others the original's). The codes $81-$9F SHALL
share one glyph, a hollow box with 1-px lines; $80 holds the same glyph
(it is never drawn). No glyph SHALL use Windows-1252 shapes.

#### Scenario: Blank no-break space
- **WHEN** a line holds `5`, `$90 A0`, `kg`
- **THEN** the cell of `$A0` is blank (a space wide) and not glyph 0

#### Scenario: C1 controls
- **WHEN** a text file with the byte $85 is loaded
- **THEN** the cell shows the hollow box, in both fonts, and the stored pair is `$90 85`

### Requirement: Glyph 26 at 6 px
The 6 px glyph 26 (`ä`) SHALL be the dots in row 0 on TED's unchanged `a`
in rows 1-4 (the scheme of `à á â`). Glyphs 27-31 at 6 px and 26-31 at 8 px
SHALL be the original's.

#### Scenario: ä at 6 px
- **WHEN** an `ä` is typed in the 6 px font
- **THEN** the cell shows the dots in row 0 and the letter `a` in rows 1-4, not an arch

### Requirement: Display only
The glyph change SHALL NOT change any file TED writes, the line format, the
compilers' output or a library. A literal character SHALL be stored,
found, printed and saved as before.

#### Scenario: Files
- **WHEN** a source with literal characters is saved by this TED-C and by the `prev` build
- **THEN** the files are byte-identical
