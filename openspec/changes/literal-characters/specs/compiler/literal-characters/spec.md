## Purpose
Says what blitz2-c and blitz2-ab3 do with a literal character
(`$90 cc`) in a source: the compiled string holds the byte, comments and
macros carry it unchanged, a literal outside a string is an error, and an
`Include` file in `BB23` is read.

## ADDED Requirements

### Requirement: A literal character in a string compiles to its byte
Wherever the compiler reads the characters of a string literal, of a
`Data$` or `Dc.b` string and of a quoted character constant (`"ab"` as a
number), a literal character `$90 cc` SHALL give the one byte `cc`, in BB2
mode and in AB3 mode. The mapping of the codes 26 to 31 in strings
(`gs`) SHALL stay as it is: the code 27 gives the byte $F6.

#### Scenario: String literal
- **WHEN** a source has `a$="Gr` `ö` `$90 DF` `e"` (ö as the code 27) and prints the string
- **THEN** the compiled string holds the bytes `47 72 F6 DF 65`

#### Scenario: Quoted constant
- **WHEN** a source has the pair `$90 DF` between the quotes of a one-character constant in an assembler operand
- **THEN** the constant is $DF

#### Scenario: No change for other sources
- **WHEN** a source has no literal character
- **THEN** the executable is byte-identical to the one the `prev` build makes

### Requirement: Comments and macros carry the pair
A literal character in a comment SHALL be skipped with the comment. In
the text of a macro and in the scans for the end of a parameter or a
macro (`findit`, the macro body scan in `compile`) it SHALL be read as one
token pair: its second byte is never read as a delimiter or a quote.
(The second byte of a literal character is $80 or more, but the scans do
not rely on that.)

#### Scenario: Pair in a macro body
- **WHEN** a macro body holds a string with `$90 DF`
- **THEN** the expanded text holds the same pair and the string compiles as above

### Requirement: A literal character outside a string is an error
A literal character outside a string and a comment SHALL end the compile
with a syntax error at its line. The compiler SHALL NOT call a library
function, read a token table entry or write code for it.

#### Scenario: Stray character
- **WHEN** a source has `$90 E9` where a statement is expected
- **THEN** the compile stops with a syntax error and the editor shows that line

### Requirement: Compiling from the editor and with the original compiler
A source that holds literal characters in strings and comments only SHALL
compile to the same string bytes with blitz2-c and with the original
compiler when both are called from TED-C (combinations Cc and C5).

#### Scenario: C5
- **WHEN** TED-C holds a program with `ß` in a string and compiles it with the original compiler (C5)
- **THEN** the string in the executable holds the byte $DF, as with blitz2-c (Cc)

### Requirement: Include files in BB23
`Include` of a source in `BB23` SHALL skip the header and read the lines
after it. An include in another format SHALL be read as before.

#### Scenario: Include with a literal character
- **WHEN** the included file begins with `BB23` and holds a string with `$90 DF`
- **THEN** the including program compiles the string with the byte $DF
