# tooling/ted-font Specification

## Purpose
Says how the complete TED fonts are generated from the glyph text sources
and the archive files, and what the build checks.

## Requirements

### Requirement: The fonts are generated from a text source and the archive files
The build SHALL generate the 256-glyph fonts in the build directory from
the checked-in glyph text sources (`#` set pixel, `.` blank pixel, one
block `@XX` plus `h` rows of 8 characters per glyph) and the unchanged
archive files `chars8.b` and `chars.b`, in the strip layout of those
files. The tool SHALL be a C program in `tools/` with its logic in
`tools/lib/`. TED SHALL embed the generated files. The generated files
SHALL NOT be checked in and `archive/` SHALL NOT be changed.

#### Scenario: Generation
- **WHEN** the build runs with the sources and the archive files
- **THEN** `ted_font8.b` has 2048 bytes and `ted_font6.b` 1536 bytes, and their strips 0-7 equal the archive files except the 6 px glyph 26

#### Scenario: Reproducible
- **WHEN** the build runs twice
- **THEN** the generated files are byte-identical

### Requirement: The build checks size and completeness
In the same build step as the generation the build SHALL check: the exact
output size; that every code $80-$FF is given exactly once; that every
block has `h` rows of exactly 8 characters from `#` and `.`; that the
archive file has the size of 128 glyphs; that the original glyphs are not
changed except the 6 px glyph 26. A failed check SHALL leave no output file
and SHALL stop the build of `ted`. Every check SHALL have a test with a
deliberately broken input.

#### Scenario: Missing glyph
- **WHEN** a text source lacks the block `@E9`
- **THEN** the tool fails with a message naming `E9` and writes no file

#### Scenario: Wrong row length
- **WHEN** a row of a block has 7 characters
- **THEN** the tool fails with a message naming the file and line

#### Scenario: Changed original glyph
- **WHEN** a text source defines `@1B` (glyph 27)
- **THEN** the tool fails and writes no file

### Requirement: The source states its origin
The header of each glyph text source SHALL name the origin of the glyphs
adapted from unscii-8 (author, licence, URL, pinned commit) and say which
glyphs are unchanged, adapted or drawn new.

#### Scenario: Header
- **WHEN** a glyph text source is opened
- **THEN** its first lines name unscii-8, its licence and the pinned commit
