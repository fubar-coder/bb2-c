# Spec Delta

## MODIFIED Requirements

### Requirement: The mode holds for the whole program
The mode is determined by the main source only and SHALL apply to the whole compile, including every included file.

#### Scenario: Text include in an AB3 program
- **WHEN** an AB3-mode program includes a text file named `*.ab3` or `*.ab2` without the embedded header
- **THEN** the include is tokenised and compiled in AB3 mode

#### Scenario: Text include in a BB2 program
- **WHEN** a BB2-mode program includes a text file that has the embedded header
- **THEN** the include is tokenised and compiled in BB2 mode

## ADDED Requirements

### Requirement: Includes of an AB3 program follow AmiBlitz3's rules
In AB3 mode an include name SHALL be the `INCDIR` string followed by the name, searched in AmiBlitz3's order (the includes directory, the main source's directory, the current include's directory, the includes directory with only the file part), each place trying the name as given, then with the extension replaced by `.ab3`, `.ab2`, `.bb2`, then with these appended.

#### Scenario: Extension fallback
- **WHEN** an AB3 program has `INCLUDE "mylib"` and the main source's directory holds `mylib.ab3`
- **THEN** `mylib.ab3` is included

### Requirement: The type of an AB3 include comes from its name
In AB3 mode a name ending (case-sensitively) in `ab3`, `AB3` or `ab2` SHALL be tokenised as text (its embedded header skipped), a name ending in `bb2` or `.bb` SHALL be read as tokenised, and any other name SHALL stop the compile with "You can only include tokenized files".

#### Scenario: Other extension refused
- **WHEN** an AB3 program has `INCLUDE "defs.h"`
- **THEN** the compile fails with "You can only include tokenized files"

#### Scenario: Tokenised by name
- **WHEN** an AB3 program includes `old.bb2`, a tokenised file with an AB3-format `.xtra`
- **THEN** it is read as tokenised and compiled

### Requirement: Repeated includes in an AB3 program
In AB3 mode including the same file a second time with `INCLUDE` SHALL be an error, and a second `XINCLUDE` of the same file SHALL be skipped.

#### Scenario: Repeated XINCLUDE
- **WHEN** an AB3 program has `XINCLUDE "lib.ab3"` twice
- **THEN** the file is compiled once and the program compiles
