# Spec Delta

## Purpose

Defines how `blitz2-ab3` handles AmiBlitz3's compiler directives
`OPTIMIZE`, `SYNTAX`, `RunErrsOn`/`RunErrsOff`/`LastRunErrsMode`, `BSS`
and `Chip` in AB3 mode, as AmiBlitz3 v3.14.0 does where this repository
supports the feature.

## ADDED Requirements

### Requirement: OPTIMIZE sets the optimisation flags from the line on
In AB3 mode `OPTIMIZE n` SHALL take a constant expression, replace all optimisation flags by its bits (not add to them) and apply to everything compiled after it in source order; the flags SHALL be 0 at the start of every pass. Inside a skipped `CNIF` block it SHALL have no effect.

#### Scenario: Flags replaced
- **WHEN** a program has `OPTIMIZE 4` and later `OPTIMIZE 16`
- **THEN** new syntax is on between the two lines and off after the second

#### Scenario: Expression
- **WHEN** a program has `OPTIMIZE 1|4`
- **THEN** it compiles with new syntax on and the 68020 bit set

### Requirement: The 68020 bit is accepted with 68000 code
`OPTIMIZE` with bit 1 SHALL be accepted; until 68020 code generation exists (phase G7) the program SHALL be compiled with 68000 code. This is a documented deviation from AmiBlitz3.

#### Scenario: OPTIMIZE 5
- **WHEN** a program with `OPTIMIZE 5` (68020 + new syntax) is compiled and run on the A1200
- **THEN** it compiles, runs with the output of AmiBlitz3's executable, and its code contains no 68020-only instructions

### Requirement: The FPU bit is refused until FPU support exists
`OPTIMIZE` with bit 2 SHALL stop the compile with the error "Not supported: OPTIMIZE 2 (FPU)" until FPU code generation exists (phase G6).

#### Scenario: OPTIMIZE 7
- **WHEN** a program with `OPTIMIZE 7` is compiled
- **THEN** the compile fails with that error at the `OPTIMIZE` line and no executable is written

### Requirement: The other OPTIMIZE bits are accepted
Bits 8 (no effect in AmiBlitz3) and 16 (short-branch optimisation) SHALL be accepted; bit 16 SHALL NOT change the program's behaviour (its code size may differ from AmiBlitz3's).

#### Scenario: OPTIMIZE 16
- **WHEN** a program with `OPTIMIZE 16` is compiled and run
- **THEN** it compiles and gives the same output as without the line

### Requirement: OPTIMIZE sets the FPU mode of the run-time libraries
Every `OPTIMIZE` SHALL generate, where it stands, the call of AmiBlitz3's `setfpumode` entry of its strings library with 0 (no FPU), as AmiBlitz3 does.

#### Scenario: Code of OPTIMIZE 4
- **WHEN** a program has `OPTIMIZE 4` before a `Print`
- **THEN** the executable calls `setfpumode` with 0 at that place and the `Print` output is the same as AmiBlitz3's

### Requirement: SYNTAX is accepted without checks
In AB3 mode `SYNTAX n` (a single digit) SHALL be accepted and SHALL NOT change the generated code; its declaration and type checks are not performed (phase G8).

#### Scenario: SYNTAX 2
- **WHEN** a correct program starts with `SYNTAX 2`
- **THEN** it compiles to the same executable as without that line

### Requirement: RunErrsOff nests and LastRunErrsMode restores
In AB3 mode `RunErrsOff` SHALL push the current runtime-error mode and switch checking off, `LastRunErrsMode` SHALL restore the mode before the matching `RunErrsOff`, and `RunErrsOn` SHALL behave as in BB2. More than 17 nested `RunErrsOff` without `LastRunErrsMode` SHALL be the error "Precedence 'RunErrs' stack overflow!", and `LastRunErrsMode` without a `RunErrsOff` the error "called without 'RunErrsOff'".

#### Scenario: Nested modes
- **WHEN** a program with runtime error checking on has `RunErrsOff`, `RunErrsOff`, `LastRunErrsMode`, `LastRunErrsMode`
- **THEN** checking is off after the first line, still off after the third, and on again after the fourth

#### Scenario: Unmatched LastRunErrsMode
- **WHEN** a program has `LastRunErrsMode` without a preceding `RunErrsOff`
- **THEN** the compile fails with "called without 'RunErrsOff'"

### Requirement: BSS and Chip start data sections
In AB3 mode the data compiled after `BSS` SHALL go into a BSS hunk of the executable and the data after `Chip` into a hunk loaded to chip memory; each SHALL be allowed once ("Only one BSS / Chip Segment allowed"); with the runtime debugger on, both SHALL be ignored, as in AmiBlitz3.

#### Scenario: Chip data
- **WHEN** a program puts `Dc.w` data after `Chip` and prints whether its address is in chip memory (`TypeOfMem_`)
- **THEN** the executable has a chip-memory hunk and the program reports chip memory

#### Scenario: Second BSS
- **WHEN** a program has `BSS` twice
- **THEN** the compile fails with "Only one BSS / Chip Segment allowed"
