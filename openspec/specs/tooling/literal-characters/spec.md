# tooling/literal-characters Specification

## Purpose
Says how the host tools write and convert literal characters (`bbdetok`,
`tedsrc`, `bblines`, `bbsrc tokens`) and what `blitzlib` checks about the
reserved library numbers. A source with literal characters is an ordinary
tokenised source, so no tool detects a format for it.

## Requirements

### Requirement: Detokenising and tokenising keep the character
`bbsrc detok` and `tedsrc detok` SHALL write a literal character as the
byte `cc` and the codes 26 to 31 as they write them today (not as
`{lib:entry}`); `tedsrc tok` and the tokeniser with the TED rules SHALL
turn a byte >= $80 of the text (other than the six umlauts) into a
literal character, so `tok(detok(x))` is `x` for every line with literal
characters in strings and comments.

#### Scenario: Round trip
- **WHEN** a tokenised file with literal characters is detokenised to text and tokenised again
- **THEN** the lines are the same as in the file

#### Scenario: Text with ß
- **WHEN** `tedsrc tok` reads a text with `ß` (byte $DF) in a string
- **THEN** the line of the output holds `$90 DF`

### Requirement: Line walkers keep a pair together
`bblines` SHALL return a line with a literal character at its start or end
unchanged, with the pair whole.

#### Scenario: Pair at the end of a line
- **WHEN** `bblines` reads a line whose last token is `$90 DF`
- **THEN** the line ends with the whole pair

### Requirement: The token list names the library
`bbsrc tokens` SHALL list the 127 tokens of the literal library like those
of any library, `$9081` to `$90FF` with the one-character names, when the
library is among its inputs.

#### Scenario: Listing
- **WHEN** `bbsrc tokens` runs over the compiler and the built `acidlibs`
- **THEN** its output holds `$9081` to `$90FF`, each with the one-character name of its code

### Requirement: The reserved library numbers
`blitzlib` SHALL refuse, in `numbers` and in `check`, a library file whose
number is 32 and any library other than the literal library whose number
is 33, naming the file and the number, except RIDosLib as library 33 in
the variants `rwe-1999`, `rwe-1999-eval` and `rwe-1999-ahi` (an exception
the caller names); the build step of a library SHALL
fail for it. The one library with number 33 SHALL be the literal
library (identified by its name entry).

#### Scenario: Library numbered 33
- **WHEN** a library file other than the literal library has the number 33
- **THEN** `blitzlib numbers` exits with an error naming that file and 33

#### Scenario: Library numbered 32
- **WHEN** a library file has the number 32
- **THEN** `blitzlib check` exits with an error naming that file and 32

#### Scenario: RIDosLib in an RWE variant
- **WHEN** `blitzlib numbers` runs over the library set of `rwe-1999` with `ridoslib` as 33 and the exception named
- **THEN** it passes; without the exception, or in another variant, it fails naming that file and 33

#### Scenario: The literal library
- **WHEN** `blitzlib numbers` runs over `acidlibs` that holds the literal library once
- **THEN** it passes
