# ab3-compat/strings Specification

## Purpose

Defines how `blitz2-ab3` treats AmiBlitz3's string features in AB3 mode: escape sequences in string literals and `Dc.b`, and character literals `@"…"`. AmiBlitz3's rules are followed, except that what is a bug in AmiBlitz3 is implemented as intended.

## Requirements

### Requirement: Escapes only with new syntax in AB3 mode
Escape sequences in string literals of run-time expressions SHALL be evaluated only in AB3 mode with new syntax on (`OPTIMIZE` bit 4). Otherwise, and in BB2 mode, a string literal SHALL keep every byte as written. `Data` strings SHALL take no escapes.

#### Scenario: New syntax off
- **WHEN** an AB3 program without `OPTIMIZE 4` has `NPrint "[\\41]"`
- **THEN** the program prints `[\\41]`

#### Scenario: New syntax on
- **WHEN** an AB3 program with `OPTIMIZE 4` has `NPrint "[\\41]"`
- **THEN** the program prints `[A]`

#### Scenario: Data string
- **WHEN** an AB3 program with `OPTIMIZE 4` has `Data$ "[\\41]"` and prints the string it reads
- **THEN** the program prints `[\\41]`

### Requirement: An escape starts with two backslashes
An escape sequence SHALL start with two backslashes `\\`. A single `\` SHALL be an ordinary character. Four backslashes `\\\\` SHALL give the two characters `\\`, and reading SHALL continue after them. Names and hex digits after `\\` SHALL be case-insensitive.

#### Scenario: Single backslash
- **WHEN** new syntax is on and a program prints `"[\41]"`
- **THEN** it prints `[\41]`

#### Scenario: Escaped backslashes
- **WHEN** new syntax is on and a program prints `"[\\\\41]"`
- **THEN** it prints `[\\41]`

### Requirement: Byte and line feed escapes
`\\` followed by two hex digits SHALL give the byte with that code. `\\N` SHALL give a line feed (`$0A`). `\\N` and `\\R` SHALL take precedence over hex: `\\4E` is the byte `$4E`, `\\N4` is a line feed and `4`.

#### Scenario: Hex bytes
- **WHEN** new syntax is on and a program prints `"[\\4e\\4E\\4a]"`
- **THEN** it prints `[NNJ]`

#### Scenario: Line feed
- **WHEN** new syntax is on and a program prints `"a\\nb"`
- **THEN** it prints `a`, a line feed and `b`

### Requirement: Repeat escape
`\\R` followed by two decimal digits dd and a character c SHALL give c exactly dd times, unchanged in case, and reading SHALL continue after c. A count of 00 SHALL give an empty text.

#### Scenario: Repeat
- **WHEN** new syntax is on and a program prints `"[\\R05*]"` and `"[\\R03ab]"`
- **THEN** it prints `[*****]` and `[aaab]`

#### Scenario: Count zero
- **WHEN** new syntax is on and a program prints `"[\\R00x]"`
- **THEN** it prints `[]`

### Requirement: Name of the current procedure
`\\__THIS_FUNCTION` SHALL give the name of the `Function` or `Statement` whose code contains the string, as written in its definition, and `GLOBAL` outside any procedure. In a macro, the procedure in which the macro is expanded SHALL count.

#### Scenario: In a statement
- **WHEN** new syntax is on and `Statement st{}` prints `"[\\__THIS_FUNCTION]"`
- **THEN** the call `st{}` prints `[st]`

#### Scenario: Outside a procedure
- **WHEN** new syntax is on and the main program prints `"[\\__THIS_FUNCTION]"`
- **THEN** it prints `[GLOBAL]`

#### Scenario: Characters after the name
- **WHEN** new syntax is on and the main program prints `"[\\__THIS_FUNCTIONXY]"`
- **THEN** it prints `[GLOBALXY]`

### Requirement: Name of the current include
`\\__THIS_INCLUDE` SHALL give the file name, without its path, of the include file whose line contains the string. In the main source it SHALL give an empty text. After an include ends, the file that included it SHALL count again.

#### Scenario: In an include
- **WHEN** new syntax is on and `INCLUDE "inc.ab3"` contains a statement printing `"[\\__THIS_INCLUDE]"`
- **THEN** a call of that statement prints `[inc.ab3]`

#### Scenario: Back in the main source
- **WHEN** new syntax is on and the main source prints `"[\\__THIS_INCLUDE]"` after the line `INCLUDE "inc.ab3"`
- **THEN** it prints `[]`

### Requirement: The current code line
`\\__CODELINE` SHALL give, in decimal, the number of the line that contains the string. Lines SHALL be counted over the main source and its includes in the order they are compiled. The main source SHALL be counted from the first line after its embedded header, an include from the first line after its own. Lines joined with `@@` SHALL each keep their number, so the number finds the line in the source.

#### Scenario: Main source with header
- **WHEN** new syntax is on and a source has a 4-line embedded header and line 21 of the file prints `"[\\__CODELINE]"`
- **THEN** it prints `[17]`

#### Scenario: After an include
- **WHEN** new syntax is on and a line in the main source that follows an included file of 10 lines prints `"[\\__CODELINE]"`
- **THEN** the number counts the 10 lines of the include as well

#### Scenario: Joined lines
- **WHEN** new syntax is on and lines 5 to 7 of a source without header are joined with `@@` into `NPrint "[\\__CODELINE] ", @@` ⏎ `"[\\__CODELINE] ", @@` ⏎ `"[\\__CODELINE]"`, and line 8 prints `"[\\__CODELINE]"`
- **THEN** it prints `[5] [6] [7]`, then `[8]`: each string gives the number of its own line, and the lines after the join keep theirs

### Requirement: Version escapes
`\\__VER_MAJOR`, `\\__VER_MINOR` and `\\__VER_PATCH` SHALL give, in decimal, the three parts of the `Version` value of the source's embedded header. `\\__VER_BUILD` SHALL give its `NumberOfBuilds`. Without such a value they SHALL give `0`.

#### Scenario: Version from the header
- **WHEN** new syntax is on and a source with header `Version = 1.2.3` and `NumberOfBuilds = 42` prints `"[\\__VER_MAJOR.\\__VER_MINOR.\\__VER_PATCH/\\__VER_BUILD]"`
- **THEN** it prints `[1.2.3/42]`

### Requirement: Date escapes
`\\__DATE__` SHALL give the system date at compile time as `mm.dd.yyyy`, and `\\__DATE_GER__` as `dd.mm.yyyy`. The date SHALL come from `dos.library`'s `DateStamp`.

#### Scenario: Dates
- **WHEN** new syntax is on and a program printing `"[\\__DATE__] [\\__DATE_GER__]"` is compiled on 2026-10-25
- **THEN** it prints `[10.25.2026] [25.10.2026]`

### Requirement: An unknown escape is an error
`\\` followed by anything other than the forms of this spec SHALL stop the compile with the error "Unknown Escape Sequence" at that line. This includes a name that only begins like a known one.

#### Scenario: Unknown form
- **WHEN** new syntax is on and a program has `NPrint "[\\qq]"`
- **THEN** the compile fails with "Unknown Escape Sequence" and no executable is written

#### Scenario: Prefix of a name
- **WHEN** new syntax is on and a program has `NPrint "[\\__THIS_FUNCXYZ]"` or `NPrint "[\\__VER_XXXXX]"`
- **THEN** the compile fails with "Unknown Escape Sequence"

### Requirement: The pointer escape is refused
`\\__THIS_POINTER` SHALL stop the compile with the error "Not supported: \\__THIS_POINTER" at that line.

#### Scenario: THIS_POINTER
- **WHEN** new syntax is on and a program has `NPrint "\\__THIS_POINTER"`
- **THEN** the compile fails with that error and no executable is written

### Requirement: Escapes in Dc.b
In AB3 mode, with or without new syntax, a string in `Dc.b` SHALL take `\\__DATE__`, `\\__DATE_GER__` and the four `\\__VER_*` escapes with the values of the run-time strings. Every other `\\` in `Dc.b` SHALL be copied as written.

#### Scenario: Version string in Dc.b
- **WHEN** a source with header `Version = 1.2.3` has `Dc.b "V\\__VER_MAJOR.\\__VER_MINOR [\\41]",0` and the program prints that string
- **THEN** it prints `V1.2 [\\41]`

#### Scenario: BB2 mode
- **WHEN** the same `Dc.b` line is compiled in BB2 mode
- **THEN** the bytes are as written

### Requirement: Character literals
In AB3 mode with new syntax on, `@"` followed by characters and `"` in a run-time expression SHALL be an integer made of the character codes, first character highest (big-endian), so that the value stored in memory holds the characters in their written order. Up to 1 character SHALL be allowed for a byte, 2 for a word and 4 for a long or float target; a float target SHALL get the integer converted as from a long. `@""` SHALL be 0.

#### Scenario: Long
- **WHEN** new syntax is on and a program has `a.l=@"ILBM"` and prints `Hex$(a)`
- **THEN** it prints `494C424D`

#### Scenario: Bytes in memory
- **WHEN** new syntax is on and a program has `a.l=@"ILBM"` and prints `Peek.b(&a)`, `Peek.b(&a+1)`, `Peek.b(&a+2)`, `Peek.b(&a+3)` as characters, and writes `a` with `WriteMem` to a file
- **THEN** it prints `I`, `L`, `B`, `M` in that order, and the file holds the bytes `49 4C 42 4D`

#### Scenario: Word in memory
- **WHEN** new syntax is on and a program has `w.w=@"AB"` and prints `Peek.b(&w)` and `Peek.b(&w+1)` as characters
- **THEN** it prints `A`, then `B`

#### Scenario: Fewer characters
- **WHEN** new syntax is on and a program has `a.l=@"AB"` and prints `Hex$(a)`
- **THEN** it prints `00004142`

#### Scenario: Byte
- **WHEN** new syntax is on and a program has `b.b=@"A"` and prints `b`
- **THEN** it prints `65`

### Requirement: Character literals outside their rules
Too many characters for the target SHALL be a syntax error, and so SHALL `@"…"` with new syntax off or in BB2 mode. A quick target and a string target SHALL give "Can't Convert Types". In a constant expression (e.g. `#c=@"ILBM"`), `@"…"` SHALL be an error, as in AmiBlitz3.

#### Scenario: Too long for a word
- **WHEN** new syntax is on and a program has `w.w=@"ABC"`
- **THEN** the compile fails with a syntax error

#### Scenario: New syntax off
- **WHEN** an AB3 program without `OPTIMIZE 4` has `a.l=@"ILBM"`
- **THEN** the compile fails with a syntax error

#### Scenario: Quick target
- **WHEN** new syntax is on and a program has `q.q=@"AB"`
- **THEN** the compile fails with "Can't Convert Types"
