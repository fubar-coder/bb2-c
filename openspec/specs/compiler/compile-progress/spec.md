# compiler/compile-progress Specification

## Purpose
Says how the progress of a compile started from the editor is shown:
the compiler reports how far it is, the editor draws the progress bar
on its own screen, and neither side relies on a feature the other side
may not have (the original TED, the original compiler, the headless
editor stub of the tests).

## Requirements

### Requirement: The compiler reports progress and never draws into the editor's bitmap
While compiling from an editor that announces the progress call,
blitz2-c SHALL report its progress through that call: the position of
the main bar at every line where it advances, and for each include
nesting level from 1 to 3 the position of its bar at every line read
from an include. blitz2-c SHALL NOT write into the editor's bitmap
(`combitmap`) at any time, whether the editor announces the call or not.

#### Scenario: Compile from TED-C
- **WHEN** a program is compiled from TED-C with blitz2-c
- **THEN** the compiler reports the main bar and the include bars through the progress call, and no byte of the screen's bitmap is written by the compiler

#### Scenario: Wide screen
- **WHEN** blitz2-c compiles from an editor that gives a `bmapmul` of 3641 or more bytes per row
- **THEN** no memory before or after the editor's bitmap is written by the compiler

### Requirement: The editor draws the progress bar only on its open, shown screen
TED-C SHALL draw the progress bar in response to the progress call,
through its screen's RastPort, and only while its screen is open and
shown; a report that arrives while the screen is closed or not shown
SHALL be ignored without error. The bar SHALL look as the original
compiler's racing stripe does on the same screen: the main bar along the
top and bottom line of the title area, the include bars on the lines
inside it, in steps of 8 pixels, up to the same right end.

#### Scenario: Same look as the original
- **WHEN** the same program is compiled from TED-C once with the original compiler and once with blitz2-c
- **THEN** the screen after the compile is the same in both runs

#### Scenario: Screen closed
- **WHEN** the compiler reports progress while TED-C's screen is closed
- **THEN** nothing is drawn, nothing outside TED's data is written, and the compile goes on

### Requirement: The compiler uses the call only when the editor announces it
TED-C SHALL announce the progress call in its `comdata` table after the
fields of the original table, with a marker that no value of the
original TED's data at that place can equal. blitz2-c SHALL use the
call only when the marker is there; without it (the original TED, an
editor stub that does not announce it) blitz2-c SHALL show no progress
bar and SHALL otherwise compile as with the call.

#### Scenario: Original TED with blitz2-c
- **WHEN** a program is compiled from the original TED with blitz2-c
- **THEN** no progress bar is shown, the compile and its results are as from TED-C, and nothing is written into TED's bitmap

### Requirement: TED-C keeps the original interface for the original compiler
TED-C SHALL keep filling the fields of the original `comdata` table,
`combitmap` and `bmapmul` included, as the original TED does.

#### Scenario: TED-C with the original compiler
- **WHEN** a program is compiled from TED-C with the original compiler
- **THEN** the original compiler finds `combitmap` and `bmapmul` as from the original TED and draws its racing stripe as before

### Requirement: The progress display changes no result
The way progress is shown SHALL NOT change the executables the compiler
writes, the files TED writes or any library file.

#### Scenario: Executables unchanged
- **WHEN** the corpus and the CD are compiled with blitz2-c before and after this change
- **THEN** every executable is byte-identical
