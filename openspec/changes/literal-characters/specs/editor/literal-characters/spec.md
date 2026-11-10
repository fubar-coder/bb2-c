## Purpose
Says how TED-C keeps a Latin-1 character in a source: the literal
character token pair `$90 cc`, which characters become one, and how the
editor shows, edits, finds, copies, prints and saves it, so that no
character other than the six umlauts is lost or changed.

## ADDED Requirements

### Requirement: A literal character is the token pair $90 cc
A literal character SHALL be stored in a line as two bytes: `$90`
followed by the character's code `cc` (`$80` to `$FF`). A line SHALL hold
no byte >= $80 that is not the first byte of a token pair, so every
reader that walks a line pair by pair stays in step. The editor SHALL
treat the pair as one character in every place that counts or shows
characters: column, cursor movement, tab stops, the line limit in
characters, search and replace.

#### Scenario: Pair shape
- **WHEN** a line holds `a$90 E9b`
- **THEN** the editor shows `aéb`, the cursor moves over the pair in one step, and `b` is in column 2

#### Scenario: Tab stop
- **WHEN** a line begins with two literal characters followed by a TAB
- **THEN** the TAB is expanded to the next tab stop counted from column 2, not from the 4 bytes stored

### Requirement: Which characters become a literal character
Every character with a code >= $80 that enters the editor SHALL be stored
as a literal character, except ä, ö, ü, Ä, Ö and Ü, which SHALL keep the
codes 26 to 31 of the umlaut mapping (`germs`). The characters enter by
typing, by loading a text file, by loading a block, by pasting, and by
the tokeniser. The tokeniser SHALL NOT clear bit 7 of any character
(today `c & 0x7F`), inside strings, inside comments and outside them. A
literal character of one of the six umlauts that is loaded from a file
SHALL be changed to the umlaut's code 26 to 31, so an umlaut has one
stored form.

#### Scenario: Typing ß in a string
- **WHEN** the user types `ß` between double quotes with tokenise on
- **THEN** the line holds `$90 DF` at that place and the screen shows `ß`

#### Scenario: Typing ä
- **WHEN** the user types `ä`
- **THEN** the line holds the code 26 as today, not a literal character

#### Scenario: Outside strings
- **WHEN** the user types `é` outside a string and a comment
- **THEN** the line holds `$90 E9`, not `i`

#### Scenario: Text file
- **WHEN** a text file with the line `a$="Größe"` is loaded
- **THEN** `ö` is the code 27 and `ß` is `$90 DF`, and the line after it is not merged into this one

#### Scenario: Tokenise off
- **WHEN** tokenise is off and the user types `é`
- **THEN** the line holds `$90 E9` (the stored form does not depend on the option)

### Requirement: Display, help and colour
The editor SHALL draw a literal character as the one glyph of its code in
the colour of ordinary text (not the token colour) and SHALL NOT offer
help for it. A character the screen font has no glyph for SHALL be drawn
as the font's default glyph; the stored pair SHALL NOT change.

#### Scenario: Colour
- **WHEN** a line with a literal character is shown with tokenise on
- **THEN** the character has the colour of the text next to it

#### Scenario: Help key on a literal character
- **WHEN** the cursor is on a literal character and the help key is pressed
- **THEN** no token help is shown

### Requirement: Find, blocks and print work on the character
Search and replace SHALL compare a literal character as the character
(`é` typed in the find string finds the pair `$90 E9`). Block copy,
cut, paste, Block Load and Block Save SHALL keep the pairs intact, also
when a block's first or last character is one. Printing SHALL send the
character's code, not the two bytes and not an ESC sequence of a token.

#### Scenario: Search
- **WHEN** the find string is `Größe` and the text holds `Gr`, the code 27, `$90 DF`, `e`
- **THEN** the search finds it

#### Scenario: Print
- **WHEN** a line with a literal character is printed
- **THEN** the printer gets the one byte `cc` at that place

### Requirement: Saving a file
With tokenise on, TED-C SHALL write each line with the 0 byte as today;
if at least one line holds a literal character the file SHALL be in the
`BB23` format (`formats/bb23-source`), otherwise in TED's format without
a header, byte for byte as before this change. With tokenise off (a
text file), TED-C SHALL write a literal character as the byte `cc` and no
header.

#### Scenario: No literal character
- **WHEN** a tokenised source without a literal character is saved
- **THEN** the file is byte-identical to the file the `prev` build writes

#### Scenario: Literal character
- **WHEN** a tokenised source with one `ß` is saved
- **THEN** the file begins with `BB23`

#### Scenario: Text file round trip
- **WHEN** a text file with `ß` is loaded and saved with tokenise off
- **THEN** the file holds the byte $DF again, as it did

### Requirement: Loading a file
TED-C SHALL recognise a `BB23` file by its header, read the lines after
it as TED's lines, and not show the header as text. The literal
characters in the lines SHALL be kept as they are. A `BB23` file the
reader refuses (formats/bb23-source) SHALL be reported to the user
without loading part of it.

#### Scenario: Round trip
- **WHEN** a `BB23` file is loaded and saved without change
- **THEN** the lines of the saved file are those of the loaded file

#### Scenario: Refused header
- **WHEN** a file begins with `BB23` and a version TED-C does not know
- **THEN** TED-C reports it and loads nothing
