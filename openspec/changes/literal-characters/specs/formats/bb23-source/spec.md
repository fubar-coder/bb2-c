## Purpose
Defines `BB23`, the file format of a tokenised Blitz 2 source that holds
literal characters, and the rule that marks such sources: a file with the
token pair `$90 cc` is `BB23`, a file without it is a TED file, so tools
and editors that do not know the pair never meet it unannounced.

## ADDED Requirements

### Requirement: The BB23 header
A `BB23` file SHALL begin with a 14-byte header of big-endian words: the
four bytes `BB23`, the version word `1`, the word `column`, the word
`row`, the word `0` and the word `top`. `column`, `row` and `top` SHALL
have the meaning of the same fields of `BB22` (the cursor is in line
`top + row`, 0-based, at the column `column` of the line as shown;
`docs/rwelibs/README.md` §4.1). After the header the lines SHALL follow
exactly as in TED's format: tokenised text, each line ended by a 0 byte,
with no prefix bytes. The file SHALL end with a 0 byte.

#### Scenario: Header bytes
- **WHEN** a source with the cursor in line 25, column 13, with line 20 at the top, is saved as `BB23`
- **THEN** the file begins with `42 42 32 33 00 01 00 0D 00 05 00 00 00 14` (the row is 5)

#### Scenario: Lines
- **WHEN** the lines after the header are read
- **THEN** they are TED's lines of the same source, split at every 0 byte that is not the second byte of a token pair

### Requirement: Detection cannot take a TED file for BB23
A reader SHALL treat a file as `BB23` only if it begins with `BB23`
followed by the word `1`. A TED file whose first line is the text `BB23`
(bytes `42 42 32 33 00` followed by anything but `01`) SHALL NOT be taken
for `BB23`. The one remaining ambiguity, a first line `BB23` followed by a
line that begins with the control code $01, SHALL be treated as `BB23`
(the 722 tokenised sources of the CD have five stray $01 bytes, none in
such a place, `docs/formats.md` §3.2).

#### Scenario: A label BB23
- **WHEN** a TED file begins with the line `BB23=1`
- **THEN** it is a TED file

#### Scenario: A line BB23 followed by a control code
- **WHEN** a file begins with the bytes `42 42 32 33 00 01` and the following bytes are not a complete header
- **THEN** a reader refuses it as `BB23` with the offset of the first bad byte; it does not fall back to TED's format

### Requirement: A strict reader
A `BB23` reader SHALL refuse, with a message naming the offset or the
line and without producing output: a version other than 1, a word at +10
other than 0, a line without its 0 byte before the end of the file, a
token start byte without its second byte or followed by a 0 byte, a
literal character pair whose second byte is 0, an LF in a line. A
literal character pair that stands for one of the six umlauts SHALL be
accepted and read as the umlaut's pair, not refused.

#### Scenario: Truncated pair
- **WHEN** the last line ends in `$90` and the 0 byte
- **THEN** the reader refuses the file

#### Scenario: Unknown version
- **WHEN** the version word is 2
- **THEN** the reader refuses the file and names the version

### Requirement: The marker is exact
A source SHALL be written as `BB23` if and only if a line holds a pair
whose first byte is `$90`. No program of this repository SHALL write the
header otherwise, and none SHALL write a `BB23` header into a file of
another format (text, AmiBlitz3's `.ab3`).

#### Scenario: Last literal character removed
- **WHEN** the only `ß` of a `BB23` file is deleted and the file is saved
- **THEN** the file is written in TED's format without the header

### Requirement: Other programs are not affected by files without the token
A file without the token SHALL be byte-identical to what the `prev` build
writes, and the original TED, the original compiler and blitz2-c without
this change SHALL read it as before.

#### Scenario: Original TED
- **WHEN** a source without a literal character, saved by TED-C, is loaded in the original TED
- **THEN** it loads as it did before this change
