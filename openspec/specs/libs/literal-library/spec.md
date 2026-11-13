# libs/literal-library Specification

## Purpose
Defines the library that names the literal-character words `$9081` to
`$90FF` so every editor that reads the compiler's token list shows them,
and the reservation of the first byte `$90` (libraries 32 and 33) for
literal characters.

## Requirements

### Requirement: The first byte $90 is reserved
The first byte `$90`, that is the libraries 32 and 33 and the words
`$9000` to `$90FF`, SHALL be reserved for literal characters. No library
built by this repository other than the literal library SHALL use library
number 32 or 33 in any variant, with one exception: the variants
`rwe-1999`, `rwe-1999-eval` and `rwe-1999-ahi` keep RIDosLib as library 33
with its original number and have no literal library. The documentation of
library numbers SHALL say so. The pair `$90 cc` with `cc` from $81 SHALL be the literal
character `cc`; the words `$9000` to `$9080` SHALL be unused (`$9080` is
not a literal character: blitz2 numbers the tokens of a user library
from `libnum * 128 + 1`, so library 33 has no word `$9080`).

#### Scenario: Variants
- **WHEN** every `libs/variants/*/libnums.h` is read
- **THEN** in the variants without RWE only `LIB_literallib` is 33, in the RWE variants only `LIB_ridoslib` is 33, and nothing is 32

#### Scenario: The documentation
- **WHEN** the documentation of library numbers (`docs/token-table.md`, `docs/libs/variants.md`, `docs/formats.md` §3.2) is read
- **THEN** each names `$90`, libraries 32 and 33, as reserved for literal characters

### Requirement: The literal library
The library `literallib` SHALL have the number 33 and 127 entries, the
words `$9081` to `$90FF`; entry `n` (counted from 1) SHALL be named by the
single character with the code `$80 + n` (Latin-1, one byte, no help text).
Its entries SHALL be dummy tokens (`!dumtoke`, kind 8): neither the
statement nor the function flag, and no code or error address, so that the compiler calls nothing and writes no code for them;
the library SHALL have no init or finit code. It SHALL be part of
`acidlibs` of every variant without RWE and also be built as the userlib file
`literallib.obj` for the original Blitz 2. The library SHALL pass
`blitzlib check`.

#### Scenario: Names
- **WHEN** `bbsrc tokens` runs over the compiler and the built `acidlibs`
- **THEN** the word `$90DF` is named `ß` (the byte $DF) and `$90FF` is named with the byte $FF

#### Scenario: Not callable
- **WHEN** a source uses the word `$90DF` as a statement or in an expression with the library loaded
- **THEN** the compile stops with "Syntax Error"

#### Scenario: Userlib file
- **WHEN** the userlib file is built
- **THEN** its library has number 33, the same 127 names and no code; the documentation says it works only if no other library with number 33 is loaded before it

#### Scenario: RWE variants
- **WHEN** the `acidlibs` of `rwe-1999` is listed
- **THEN** it has no `literallib` and RIDosLib has number 33

### Requirement: Existing results and checks
Library files other than the new ones SHALL stay byte-identical to the
`prev` build. The comparison of the token list with the original's
(`tokens_port`, `tokens_cd`) SHALL name the 127 tokens as an addition and
otherwise stay as strict as before, and the comparison of `acidlibs` with
the CD's SHALL name the library as an addition.

#### Scenario: Token list
- **WHEN** `tokens_port` runs
- **THEN** it passes with the original's list plus the 127 tokens of the library, and fails if one of them is missing or renamed

#### Scenario: Other libraries
- **WHEN** the library files are compared with the `prev` build
- **THEN** only `acidlibs` of the variants without RWE and the new userlib file differ
