# Spec Delta

## Purpose

Decides whether `blitz2-ab3` compiles a program with Blitz Basic 2's
rules (BB2 mode) or AmiBlitz3's (AB3 mode), and keeps that choice
consistent for everything the program includes.

## ADDED Requirements

### Requirement: AB3 text sources are recognised
`blitz2-ab3` SHALL compile a text source in AB3 mode when its first line is AmiBlitz3's embedded header (`; XTRA`) or when its name ends in `.ab3` (in any case).

#### Scenario: Text with the embedded header
- **WHEN** `blitz2-ab3` compiles a text file `prog.bb` whose first line is `; XTRA`
- **THEN** the program is compiled in AB3 mode

#### Scenario: Text named .ab3 without header
- **WHEN** `blitz2-ab3` compiles a text file `prog.ab3` without the embedded header
- **THEN** the program is compiled in AB3 mode

### Requirement: Tokenised AB3 sources are recognised by their companion file
`blitz2-ab3` SHALL compile a tokenised source in AB3 mode when its companion `.xtra` file has AmiBlitz3's format, and in BB2 mode otherwise.

#### Scenario: Tokenised source with an AB3 .xtra
- **WHEN** `blitz2-ab3` compiles a tokenised source whose `.xtra` has AmiBlitz3's format
- **THEN** the program is compiled in AB3 mode

#### Scenario: Tokenised source with a BB2 .xtra or none
- **WHEN** `blitz2-ab3` compiles a tokenised source whose `.xtra` has BB2's format, or that has no `.xtra`
- **THEN** the program is compiled in BB2 mode

### Requirement: Other sources are BB2 unless the mode is forced
A source that matches none of the AB3 rules SHALL be compiled in BB2 mode, unless the mode option of the command line selects AB3 mode; the option SHALL also be able to force BB2 mode.

#### Scenario: Plain text without header
- **WHEN** `blitz2-ab3` compiles a text file `prog.bb` without the embedded header and without the mode option
- **THEN** the program is compiled in BB2 mode

#### Scenario: Mode forced to AB3
- **WHEN** `blitz2-ab3` compiles the same file with the option selecting AB3 mode
- **THEN** the program is compiled in AB3 mode

### Requirement: The mode holds for the whole program
The mode is determined by the main source only and SHALL apply to the whole compile, including every included file.

#### Scenario: Text include in an AB3 program
- **WHEN** an AB3-mode program includes a text file without the embedded header
- **THEN** the include is tokenised and compiled in AB3 mode

#### Scenario: Text include in a BB2 program
- **WHEN** a BB2-mode program includes a text file that has the embedded header
- **THEN** the include is tokenised and compiled in BB2 mode

### Requirement: Tokenised includes of the other mode are rejected
A tokenised include whose mode (by the same rule as for a main source) differs from the program's mode SHALL stop the compile with an error that names the include; it SHALL NOT be compiled with the program's token table.

#### Scenario: BB2-tokenised include in an AB3 program
- **WHEN** an AB3-mode program includes a tokenised file without an AB3-format `.xtra`
- **THEN** the compile fails with an error naming that file and no executable is written

#### Scenario: AB3-tokenised include in a BB2 program
- **WHEN** a BB2-mode program includes a tokenised file with an AB3-format `.xtra`
- **THEN** the compile fails with an error naming that file and no executable is written

### Requirement: One mode per run, editor sessions are BB2
A command line run of `blitz2-ab3` SHALL compile exactly one program, with the library set of that program's mode only; a session started with the editor SHALL always be in BB2 mode, so BB2's and AB3's libraries are never loaded at the same time.

#### Scenario: Command line run in AB3 mode
- **WHEN** `blitz2-ab3` compiles an AB3 source from the command line
- **THEN** only AmiBlitz3's libraries are loaded during that run

#### Scenario: Editor session
- **WHEN** `blitz2-ab3` is started with the editor and a source is compiled from it
- **THEN** it is compiled in BB2 mode with BB2's libraries, as with `blitz2`
