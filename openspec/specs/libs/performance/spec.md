# libs/performance Specification

## Purpose
Defines how the C libraries are made at least as fast as the original Blitz Basic 2 libraries: results stay bit for bit the same, C is written first and assembler is used only where C stays behind, the slow commands are listed, and every speed change is measured with the benchmark.

## Requirements

### Requirement: Behaviour does not change
A change made for speed SHALL NOT change what a command returns, draws or reports, and SHALL NOT change an error check, except the fixes named in the change (#1058, #1059, #1064 Frac of a Quick). The library tests (`libs_emulator`), the corpus, the bug-orig tests and the other acceptance tests SHALL pass unchanged except for the cases of the new register rule (`libs/register-contract`).

#### Scenario: An assembler block replaces C
- **WHEN** the loop of long division is replaced by an assembler block
- **THEN** every quotient, remainder and the fix kept at the entry (#200) give the values of the expected data

### Requirement: The speed profile
The libraries SHALL be built at `-Os` except those in the speed profile. The speed profile (`ACIDLIBS_SPEED`) SHALL contain bitmaplib, medlib, sortlib, stringfunclib, trackerlib and vallib. A library joins it only where the measurement shows a gain without slowing a command; strings1lib and strings2lib are not in it (they measured faster at `-Os` after their copy loops were rewritten, design.md Decision 15).

#### Scenario: The profile list
- **WHEN** the library build is configured
- **THEN** the six libraries are compiled at `-O2` with the profile's flags and every other acidlib at `-Os`

### Requirement: C first, assembler where C stays behind
A hot routine SHALL first be written in C and measured. It MAY be an assembler block in its library's C file only where C stays more than about 1 to 2 % behind the original's cycles per call, taking the original's algorithm from the CD's library. A block SHALL keep the registers its entry requires under `libs/register-contract`, SHALL keep the decided fixes at its entry, and SHALL be covered by a library test case that the original and the C build both pass.

#### Scenario: The order of the work
- **WHEN** the change is carried out
- **THEN** it goes: the loop of long and quick division (ldivlib, modlib); the string copy loops of strings1lib and strings2lib (C, no block needed); the small entries (Peek/Poke, Joy*, Mouse*, RawStatus, DoColl/SColl/PColl, ShapeWidth/ShapeHeight/Handle, Colour/Locate, QLimit/Rnd, Plot/Point, Int/Sgn/Abs, the event getters of windowslib); the blit core and sortlib; then the remaining slow commands (text, graphics primitives, collisions, display and palettes, Frac, the players, Polyf)

#### Scenario: A small entry
- **WHEN** `Peek.b` is an assembler routine
- **THEN** it costs within the measured budget of the original (60 cycles instead of 146) and returns the same value

#### Scenario: Division
- **WHEN** a program divides two longs
- **THEN** the quotient is as before and the call costs no more than the budget of the benchmark (original 2661 cycles, port before the change 5674)

#### Scenario: A C version reaches the original
- **WHEN** the C copy loop of `copstring` measures within 1 to 2 % of the original's assembler
- **THEN** no assembler block is added

### Requirement: Slow commands are listed
After the work the commands that are still 10 % or more slower than the original SHALL be listed with their cause, from at least three runs of the benchmark and one run of the original (`perf-analysis/final-table.md`). A command whose gap is a decided fix (an intended deviation) or a frame or blitter loop that varies is named as such.

#### Scenario: The final table
- **WHEN** the benchmark has run three times and `perf-original` once
- **THEN** the table lists every command with port, original and ratio, and the commands at 1.10 or more with the library that causes it

### Requirement: A speed change is measured
A commit that changes a routine for speed SHALL name the measured cycles per call before and after, from the benchmark (`testing/performance-budgets`), and SHALL NOT keep a change that does not improve the measured value.

#### Scenario: A change without effect
- **WHEN** a block measures the same as the C it replaces
- **THEN** the change is not committed
