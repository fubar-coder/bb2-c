## ADDED Requirements

### Requirement: Behaviour does not change
A change made for speed SHALL NOT change what a command returns, draws or reports, and SHALL NOT change an error check. The library tests (`libs_emulator`), the corpus, the bug-orig tests and the other acceptance tests SHALL pass unchanged except for the cases of the new register rule (`libs/register-contract`).

#### Scenario: An assembler block replaces C
- **WHEN** the loop of long division is replaced by an assembler block
- **THEN** every quotient, remainder and the fix kept at the entry (#200) give the values of the expected data

### Requirement: The speed profile
The libraries SHALL be built at `-Os` except those in the speed profile. The speed profile (`ACIDLIBS_SPEED`) SHALL contain bitmaplib, sortlib, stringfunclib, vallib, strings1lib, strings2lib, medlib and trackerlib.

#### Scenario: The profile list
- **WHEN** the library build is configured
- **THEN** the eight libraries are compiled at `-O2` with the profile's flags and every other acidlib at `-Os`

### Requirement: Hot routines may be assembler blocks
Where C does not reach the speed of the original for a hot routine, the routine MAY be an assembler block in its library's C file, taking the original's algorithm from the CD's library. A block SHALL keep the registers its entry requires under `libs/register-contract`, SHALL keep the decided fixes at its entry, and SHALL be covered by a library test case that the original and the C build both pass. The order of the work SHALL be: the loop of long and quick division (ldivlib); the string copy loops of strings1lib and strings2lib; the small entries (Peek/Poke, Joyx/Joyy/Joyb/Joyr, MouseX/MouseY/MouseXSpeed/MouseYSpeed, RawStatus, DoColl/SColl/PColl, ShapeWidth/ShapeHeight/Handle, Colour/Locate, QLimit/Rnd, Plot/Point); the blit core (blitlib, qblitlib, bblitlib, ClipBlit, stencillib) and sortlib.

#### Scenario: A small entry
- **WHEN** `Peek.b` is an assembler routine
- **THEN** it costs within the measured budget of the original (60 cycles instead of 146) and returns the same value

#### Scenario: Division
- **WHEN** a program divides two longs
- **THEN** the quotient is as before and the call costs no more than the budget of the benchmark (original 2661 cycles, port before the change 5674)

### Requirement: A speed change is measured
A commit that changes a routine for speed SHALL name the measured cycles per call before and after, from the benchmark (`testing/performance-budgets`), and SHALL NOT keep a change that does not improve the measured value.

#### Scenario: A change without effect
- **WHEN** a block measures the same as the C it replaces
- **THEN** the change is not committed
