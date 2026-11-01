# Spec Delta

## Purpose

Lets a source tell at compile time whether it is compiled for debugging,
through the predefined constant `#__debug` (AmiBlitz3's name and values),
in BB2 mode and in AB3 mode, so that debug-only code such as `Stop` can
be put into `CNIF #__debug … CEND`.

## ADDED Requirements

### Requirement: #__debug tells whether runtime debugging is on
In both modes the constant `#__debug` SHALL be predefined with the value −1 when the program is compiled with runtime error checking (the runtime debugger) and 0 otherwise; it SHALL be usable wherever a constant is (expressions, `CNIF`) without being defined by the source.

#### Scenario: Debug build
- **WHEN** a program with `NPrint #__debug` is compiled with runtime error checking on
- **THEN** it prints -1

#### Scenario: Release build
- **WHEN** the same program is compiled without runtime error checking (e.g. `blitz2-ab3 -r`)
- **THEN** it prints 0

#### Scenario: Debug-only code
- **WHEN** a program has `CNIF #__debug : Stop : CEND` and is compiled without runtime error checking
- **THEN** it compiles (the `Stop` is skipped) and runs to its end

### Requirement: Stop keeps Blitz Basic 2's behaviour
`Stop` SHALL behave as in Blitz Basic 2 in both modes: without runtime error checking the compile fails with "Debugger must be enabled for STOP"; with it the program calls the debugger's `Stop`.

#### Scenario: Stop in a release build
- **WHEN** a program with a plain `Stop` is compiled without runtime error checking
- **THEN** the compile fails with "Debugger must be enabled for STOP"

### Requirement: Sources without #__debug are unchanged
A source that does not use `#__debug` SHALL compile to the same executable as before this change, in both modes.

#### Scenario: Corpus
- **WHEN** the corpus, the bug-orig tests and the CD's sources are compiled before and after this change
- **THEN** every executable is byte-identical
