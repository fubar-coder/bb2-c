# tedfont

Makes TED-C's two screen fonts of 256 glyphs (8 x 8 and 8 x 6 pixels):
the glyphs 0-127 of TED 1.1's font files (`archive/sources/ted-1.1/chars8.b`,
`chars.b`, unchanged) and the glyphs $80-$FF of a glyph text source
(`programs/ted/fonts/glyphs8.txt`, `glyphs6.txt`), which at 6 px also
redraws glyph 26 (spec `tooling/ted-font`). `build` writes the font,
`check` is the check of the same build step. The logic is
`tools/lib/tedfont` (`tedfont.h`), tested by `tools/tests/test_tedfont.c`.

## Usage

```
Usage: tedfont [OPTIONS] SUBCOMMAND

TED-C's 256-glyph screen fonts from a glyph text source and TED 1.1's font file.

OPTIONS:
  -h,     --help              Print this help message and exit

SUBCOMMANDS:
  build                       write the 256-glyph font from the archive font and the source
  check                       check a font against the archive font and the source; remove it
                              if wrong
```

```
Usage: tedfont build [OPTIONS] HEIGHT ARCHIVE SOURCE OUT

write the 256-glyph font from the archive font and the source

POSITIONALS:
  HEIGHT INT:{8,6} REQUIRED   the font height: 8 or 6
  ARCHIVE TEXT REQUIRED       TED 1.1's font file of that height (chars8.b, chars.b)
  SOURCE TEXT REQUIRED        the glyph text source (glyphs8.txt, glyphs6.txt)
  OUT TEXT REQUIRED           the font to write

OPTIONS:
  -h,     --help              Print this help message and exit

Exit status: 0 the font is written (build) or right (check), 1 an input is wrong
or cannot be read, the font cannot be written or is wrong (a message on stderr;
OUT does not exist afterwards), 2 usage error.
```

```
Usage: tedfont check [OPTIONS] HEIGHT ARCHIVE SOURCE OUT

check a font against the archive font and the source; remove it if wrong

POSITIONALS:
  HEIGHT INT:{8,6} REQUIRED   the font height: 8 or 6
  ARCHIVE TEXT REQUIRED       TED 1.1's font file of that height (chars8.b, chars.b)
  SOURCE TEXT REQUIRED        the glyph text source (glyphs8.txt, glyphs6.txt)
  OUT TEXT REQUIRED           the font to check

OPTIONS:
  -h,     --help              Print this help message and exit

Exit status: 0 the font is written (build) or right (check), 1 an input is wrong
or cannot be read, the font cannot be written or is wrong (a message on stderr;
OUT does not exist afterwards), 2 usage error.
```

* `build`: reads `SOURCE` and `ARCHIVE` and writes `OUT`, the font in
  the strip layout of the archive file (16 strips of 16 glyphs, `h` rows
  each: row r of glyph c is byte `(c & 15) + (c >> 4) * 16 * h + r * 16`,
  bit 7 the left pixel): 2048 bytes at `HEIGHT` 8, 1536 at 6. Strips 0-7
  are the archive file, glyph 26 replaced if the source gives `@1A`
  (only at 6 px); strips 8-15 are the glyphs $80-$FF of the source.
* `check`: the font `OUT` against its inputs: its exact size; glyphs
  0-127 the archive's (glyph 26 at 6 px the source's, if given); every
  glyph $80-$FF the source's. A font that fails is removed.

## Inputs and outputs

The glyph text source: lines starting with `;` are comments, blank
lines are ignored. First `h=8` or `h=6` (equal to `HEIGHT`), then per
glyph a line `@XX` (the code, two hexadecimal digits; a `;` comment may
follow) and `h` rows of exactly 8 characters, `#` a set pixel and `.` a
blank one:

```
h=6
@A1 ; inverted exclamation mark
...##...
........
...##...
...##...
...##...
........
```

Every code $80-$FF must be given exactly once; `@1A` may be given at
6 px; any other code is refused (the glyphs 0-127 are TED's). The archive
file must have 128 glyphs (1024 or 768 bytes). Both subcommands parse the
source completely; an error names the file and the line.

`OUT` exists afterwards only if `build` succeeded and `check` found it
right: `build` removes an old `OUT` first and writes it only when its
inputs are valid, `check` removes a wrong one.

## Exit codes

* 0 the font is written (`build`) or right (`check`);
* 1 a wrong input (the source, an archive file of the wrong size), a file
  that cannot be read or written, a font that fails the check;
* 2 usage error (every command line the parser refuses).

Messages go to stderr: an error of the source as `SOURCE:LINE: message`
(or `SOURCE: message` for a missing glyph), the other errors as
`tedfont SUB: FILE: message`.

## Callers

* `programs/ted/fonts/fonts.cmake`: the build step of `ted_font8.b` and
  `ted_font6.b` (`build`, then `check` as the second command), in the
  m68k build (`programs/ted/CMakeLists.txt`, for TED) and on the host
  side (`programs/ted/tests/host/CMakeLists.txt`, for the host tests of
  the TED core and the TED A/B runner).
* Tests: `tedfont` (the library), `tedfont_build_broken` (the build
  step on a broken source), `help_tedfont`, `readme_tedfont`.

## Example

```
$ tedfont build 8 archive/sources/ted-1.1/chars8.b programs/ted/fonts/glyphs8.txt ted_font8.b
$ tedfont check 8 archive/sources/ted-1.1/chars8.b programs/ted/fonts/glyphs8.txt ted_font8.b
$ tedfont build 6 archive/sources/ted-1.1/chars.b programs/ted/fonts/glyphs8.txt ted_font6.b
programs/ted/fonts/glyphs8.txt: h=8, but HEIGHT is 6
$ echo $?
1
```
