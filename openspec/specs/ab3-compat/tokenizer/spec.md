# ab3-compat/tokenizer Specification

## Purpose

Defines how text is turned into tokens in AB3 mode, so that a source
written for AmiBlitz3 gets the same tokens as AmiBlitz3 v3.14.0's own
tokeniser gives it.

## Requirements

### Requirement: Names match case-sensitively
In AB3 mode a word SHALL become a token only if it equals a name of the token list exactly, including case; otherwise it SHALL stay a name (variable, label, procedure).

#### Scenario: Lower-case library name
- **WHEN** an AB3 source has `*exec.ExecBase = execbase`
- **THEN** `execbase` stays a variable name (not the function `ExecBase`), with the same tokens as AmiBlitz3 gives the line (AmiBlitz3's "Can't convert types" for it comes from a type rule of assignments, not from tokenising; that rule belongs to the type checks, G8)

#### Scenario: Exact name
- **WHEN** an AB3 source has `NPrint "x"`
- **THEN** `NPrint` is the token of the command

### Requirement: Only whole words after a separator become tokens
In AB3 mode only a whole word starting with a letter SHALL be matched, and only after a space, the line start or an operator character; never directly after `.`, `\`, `#`, `'`, `@`, `_` or a digit.

#### Scenario: Name inside an identifier
- **WHEN** an AB3 source has a procedure call `show{1}` and a variable `sub1`
- **THEN** neither contains a token (the BB2 tokeniser would split them)

#### Scenario: After a backslash
- **WHEN** an AB3 source has the field access `*w.Window\Width`
- **THEN** `Width` after `\` stays a field name

### Requirement: Longest form and first entry win
In AB3 mode `name$` SHALL be tried before `name`, and when a name occurs twice in the token list the first entry SHALL win.

#### Scenario: String function
- **WHEN** an AB3 source has `a$ = Str$(5)`
- **THEN** `Str$` is the token of the string function

### Requirement: Strings, comments and long lines
In AB3 mode string literals and comments SHALL not be tokenised, and a line of the main source longer than 250 characters SHALL be cut there, as in AmiBlitz3.

#### Scenario: Command name in a string
- **WHEN** an AB3 source has `NPrint "Print"`
- **THEN** the program prints `Print`

### Requirement: The same tokens as AmiBlitz3
For the AB3 test programs of the test suite, the tokenised lines produced in AB3 mode SHALL equal those AmiBlitz3 v3.14.0 produces for the same source and libraries.

#### Scenario: Test programs
- **WHEN** the tokens of each test program of `ab3_cli` are compared with AmiBlitz3's (its `-save` output or an equivalent dump)
- **THEN** they are equal
