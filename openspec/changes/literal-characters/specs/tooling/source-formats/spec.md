## Purpose
Says how the host tools and the shared source-format code (`bbsrc`,
`tedsrc`, `xtra`, `tools/lib/bb22`, `tools/lib/bbdetok`, `shared/xtra`)
recognise, read and convert `BB23` sources and literal characters, with
the strictness of the existing `BB22` reader.

## ADDED Requirements

### Requirement: xtra_detect recognises BB23
`xtra_detect` SHALL return a new type `XTRA_T_BB23` for a buffer that
begins with `BB23` and the word `1`. A buffer that begins with `BB22` SHALL
still give `XTRA_T_BB22`. `xtra_load` SHALL read the header of a
`BB23` source into `editor` (line = `top + row`, column, top) and set
`hdr` to 14, and the settings SHALL come from the companion `NAME.xtra`
as for `BB22`.

#### Scenario: Detection
- **WHEN** a buffer begins with `BB23` and the word 1
- **THEN** `xtra_detect` returns `XTRA_T_BB23` and `xtra_load` gives `hdr` 14 and the cursor of the header

#### Scenario: Other types unchanged
- **WHEN** the existing detection tests (TED, `BB22`, AB3, text) run
- **THEN** their results are the same as before

### Requirement: A BB23 reader and a converter
`tools/lib/bb22` SHALL read `BB23` with the refusals of the format spec
(offset or line named, no output), and `bbsrc` SHALL offer the two
operations it has for `BB22`: `bb23 IN OUT` writes the lines after the
header (TED's lines with their literal characters, no header) and
`bb23-info IN` shows version, cursor, line count, longest line and the
number of literal characters. A file that is not `BB23` SHALL be refused
by these subcommands.

#### Scenario: Convert
- **WHEN** `bbsrc bb23 IN OUT` runs on a valid `BB23` file
- **THEN** `OUT` is the part of `IN` after the 14 header bytes

#### Scenario: Broken input
- **WHEN** the same command runs on a file with a truncated pair, a version 2 or an LF in a line
- **THEN** it exits with an error naming the offset or line and writes nothing

### Requirement: Detokenising and tokenising keep the character
`bbsrc detok` and `tedsrc detok` SHALL write a literal character as the
byte `cc` and the codes 26 to 31 as they write them today; `tedsrc tok` and
`bbsrc` with the TED rules SHALL turn a byte >= $80 of the text (other than
the six umlauts) into a literal character, so `tok(detok(x))` is `x` for
every line with literal characters in strings and comments. `tedsrc tok`
SHALL write a `BB23` file if a literal character occurs and TED's format
otherwise.

#### Scenario: Round trip
- **WHEN** a `BB23` file is detokenised to text and tokenised again
- **THEN** the lines are the same as in the file

#### Scenario: Text with ß
- **WHEN** `tedsrc tok` reads a text with `ß` (byte $DF) in a string
- **THEN** the output begins with `BB23` and the line holds `$90 DF`

### Requirement: BB22 is unchanged
The `BB22` reader, its subcommands and its tests SHALL keep their
behaviour; `BB23` SHALL NOT make the reader accept `BB22` files with a
prefix other than 0 or a version other than 8 or 10.

#### Scenario: Existing archive
- **WHEN** the existing `bb22_rwelibsrc` test runs
- **THEN** it passes as before
