## Purpose
Says what blitz2-c and blitz2-ab3 do with a literal character
(`$90 cc`) in a source: the compiled string holds the byte, comments and
macros carry it unchanged, a literal outside a string is an error, and an
`Include` file with literal characters is read like any other.

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
with an error at its line, and the compiler SHALL NOT call a library
function or write code for it. With the library of `libs/literal-library`
loaded (blitz2-c's `acidlibs`, or the userlib with the original compiler)
the error SHALL be "Syntax Error", as the entries have neither the
statement nor the function flag. Without the library (AB3 mode) it SHALL be
the existing "Token Not Found : $90xx.". Neither needs new compiler code.

#### Scenario: Stray character as a statement
- **WHEN** a source has `$90 E9` where a statement is expected and the library is loaded
- **THEN** the compile stops with a syntax error and the editor shows that line

#### Scenario: Stray character in an expression
- **WHEN** a source has `a=` `$90 E9` and the library is loaded
- **THEN** the compile stops with a syntax error at that line

#### Scenario: AB3 mode
- **WHEN** blitz2-ab3 compiles a source with `$90 E9` outside a string
- **THEN** it stops with "Token Not Found : $90E9." and writes no executable

### Requirement: Compiling from the editor and with the original compiler
A source that holds literal characters in strings and comments only SHALL
compile to the same string bytes with blitz2-c and with the original
compiler when both are called from TED-C (combinations Cc and C5), also
when the literal characters are in an `Include`d file.

#### Scenario: C5
- **WHEN** TED-C holds a program with `ß` in a string and compiles it with the original compiler (C5)
- **THEN** the string in the executable holds the byte $DF, as with blitz2-c (Cc)

#### Scenario: Include with a literal character
- **WHEN** an included tokenised file holds a string with `$90 DF`
- **THEN** the including program compiles the string with the byte $DF, with blitz2-c and with the original compiler
