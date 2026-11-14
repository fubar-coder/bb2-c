# editor/literal-characters Specification

## Purpose
Says how TED-C keeps a Latin-1 character in a source: the literal
character token pair `$90 cc`, which characters become one, and how the
editor shows, edits, finds, copies, prints and saves it, so that no
character other than the six umlauts is lost or changed.

## Requirements

### Requirement: A literal character is the token pair $90 cc
A literal character SHALL be stored in a line as two bytes: `$90`
followed by the character's code `cc` (`$81` to `$FF`). A line SHALL hold
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
Every character with a code >= $81 that enters the editor SHALL be stored
as a literal character, except ä, ö, ü, Ä, Ö and Ü, which SHALL keep the
codes 26 to 31 of the umlaut mapping (`germs`). The characters enter by
typing, by loading a text file, by loading a block, by pasting, and by
the tokeniser. The tokeniser SHALL NOT clear bit 7 of any character
(today `c & 0x7F`) inside strings and inside comments; outside them it
does the same for simplicity, but that is not a guaranteed behaviour and
may change. A
text file or block that holds one of the six umlauts as a byte SHALL give
the umlaut's code 26 to 31, so an umlaut has one stored form. A tokenised
file is loaded as it is stored. The character $80 (a C1 control
character, which no key gives) is no literal character (user decision of
2026-11-13, `libs/literal-library`): a byte $80 of a text file, a block
or a replacement text SHALL be stored as `?`.

#### Scenario: The character $80
- **WHEN** a text file with the line `a$="x` $80 `y"` is loaded
- **THEN** the line holds `a$="x?y"`

#### Scenario: Typing ß in a string
- **WHEN** the user types `ß` between double quotes with tokenise on
- **THEN** the line holds `$90 DF` at that place and the screen shows `ß`

#### Scenario: Typing ä
- **WHEN** the user types `ä`
- **THEN** the line holds the code 26 as today, not a literal character

#### Scenario: Outside strings
- **WHEN** the user types `é` outside a string and a comment
- **THEN** the line holds `$90 E9`, not `i` (this behaviour is not guaranteed outside strings and comments and may change)

#### Scenario: Text file
- **WHEN** a text file with the line `a$="Größe"` is loaded
- **THEN** `ö` is the code 27 and `ß` is `$90 DF`, and the line after it is not merged into this one

#### Scenario: Tokenise off
- **WHEN** tokenise is off and the user types `é`
- **THEN** the line holds `$90 E9` (the stored form does not depend on the option)

### Requirement: Display, help and colour
The editor SHALL draw a literal character as the glyph of its code (the
font of `editor/screen-font`) in the colour of ordinary text (not the
token colour) and SHALL NOT offer help for it. The glyph SHALL be the same
whether the cell was drawn by typing, by loading, or by a redraw (scroll,
refresh, reline), also in rows other than the cursor row. A character the
font has no glyph for SHALL be drawn as the font's default glyph (glyph 0);
the stored pair SHALL NOT change. With the font of this change no
character that can be stored has no glyph.

#### Scenario: Colour
- **WHEN** a line with a literal character is shown with tokenise on
- **THEN** the character has the colour of the text next to it

#### Scenario: Help key on a literal character
- **WHEN** the cursor is on a literal character and the help key is pressed
- **THEN** no token help is shown

#### Scenario: The glyph of the code
- **WHEN** a line holds `$90 DF` (`ß`) and the line is shown
- **THEN** the cell shows the glyph of `ß`, not glyph 0

#### Scenario: Redraw outside the cursor row
- **WHEN** a line with `é` is not the cursor row and the screen scrolls horizontally or is refreshed
- **THEN** the cell still shows the glyph of `é`

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

### Requirement: Saving and loading a file
TED-C SHALL write a tokenised source exactly as before this change: each
line with its 0 byte, no header, no flag; a line that holds a literal
character holds the pair `$90 cc`. A source without a literal character
SHALL be byte for byte what the `prev` build writes. With tokenise off (a
text file), TED-C SHALL write only text: a literal character as the byte
`cc`, the codes 26..31 as ä ö ü Ä Ö Ü (Latin-1), and a token as its name.
Tokenise off SHALL never turn a character into a token. A
tokenised file SHALL be loaded as it is stored, so that load and save
without a change give the same bytes.

#### Scenario: No literal character
- **WHEN** a tokenised source without a literal character is saved
- **THEN** the file is byte-identical to the file the `prev` build writes

#### Scenario: Literal character
- **WHEN** a tokenised source with one `ß` is saved
- **THEN** the file is TED's format without a header and the line holds `$90 DF`

#### Scenario: Round trip
- **WHEN** a tokenised file with literal characters is loaded and saved without change
- **THEN** the saved file is byte-identical to the loaded file

#### Scenario: Text file round trip
- **WHEN** a text file with `ß` is loaded and saved with tokenise off
- **THEN** the file holds the byte $DF again, as it did

#### Scenario: Text save of umlauts and tokens
- **WHEN** a line holds the code 26 (a typed `ä`) and the token of `Print`, and the file is saved with tokenise off
- **THEN** the file holds the byte $E4 and the text `Print`, and no byte of a token pair

### Requirement: The name of a literal character
The editor SHALL show a literal character as the one character of its
code whether or not the compiler's token list names the word: the list's
entry (the library `libs/literal-library`) when it has one, a built-in
table of 127 one-character names when it has not (standalone TED, AB3
mode, a Blitz 2 without the library). A word `$90cc` with `cc` < $81 SHALL
stay an unknown token.

#### Scenario: No token list
- **WHEN** the editor runs with no token list and a line holds `$90 E9`
- **THEN** it shows `é`, not `?????`

#### Scenario: With the library
- **WHEN** the token list holds the 127 entries of the library and a line holds `$90 E9`
- **THEN** it shows `é`, as without the list
