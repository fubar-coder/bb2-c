# rizonejoylib (64): zones and joysticks

Reflective Images, last change 12/8/95. Two parts:

* **Zones** – rectangles and circles in up to 16 zone tables (count.l,
  then x1.w, x2.w, y1.w, y2.w per zone; x1 < 0 = not set, a circle has
  y2 = -1 and the radius squared in x2): `ZoneInit` (all, one, a range),
  `SetZone num,x1,y1,x2,y2` / `SetZone num,x,y,radius`, `Zone(x,y)` (the
  first zone that contains the point, -1), `ZoneTest(start[,end],x,y)`,
  `ZoneTable`, `ZoneTableSize`, `NewZoneTable`, `UseZoneTable`,
  `FreeZoneTable`, `ZoneTableNumber`, `GetZoneX1/Y1/X2/Y2`, and the 68020
  mode (`ZoneMode`, `SetZoneMode`; on by default on a 68020 or better),
  which tests rectangles with `CMP2`. init allocates table 0 with 256
  zones, finit frees every table.
* **Joysticks** – `JFire`, `JVert`, `JHoriz` for port 1, 2 and the four
  player adaptor's ports 3, 4 (`JAdaptorStatus` switches them on/off),
  `AllFire([bits])`, `ReadJoyPad(port)` for the CD32 pad.

* **Source:** `rwelibsrc/zonejoylib/ZoneJoy-Library4.bb` (as TED shows
  it: `build/amiga/rwelibsrc/zonejoylib/ZoneJoy-Library4.bb`); versions 3
  and 4 have the same declarations, the reference build of 4 equals the
  binary (`docs/rwelibs/README.md` 3.1).
* **Reference:** BSS2 `RIZONEJOYLIB.OBJ`; declarations equal (ctest
  `rwe_declarations_rizonejoylib`).
* **C file:** `libs/userlibs/rwe/rizonejoylib.c`; test
  `libs/tests/rwe/rizonejoylib.c` (snitch case "rwe rizonejoylib").

## C and assembler

All routines are C behind `BLITZ_THUNK`; no other assembler.

* `CMP2.w` (68020 mode) is the same signed bounds test in C (the
  library is built for the 68000); the 68000 mode keeps its own tests
  (`BGT` for x1/y1, `BMI` on the word difference for x2/y2).
* The word arithmetic of the original (`ASL.w`, `ADDQ.w` on a long
  counter, the start zone through `MOVE.b`) is kept with explicit masks,
  so out-of-range numbers address the same places (#878).
* The data `endzone`, `_zonetable`, `zonetablelist`, `zones020`, `is020`
  are one packed struct in the original's order: the list entry -1 (a
  negative table number) is `_zonetable`, as in the original.
* The hardware registers (CIA A/B, POTGO, POTINP, JOYxDAT) are volatile
  accesses of the original's width (byte reads of `$dff016` for
  `BTST #n,$dff016`). ReadJoyPad clocks the pad's shift register through
  the fire line; its delays are reads of CIA A, each synchronised to the
  E clock, kept as the same volatile reads in the same order – its timing
  does not depend on the instructions around them, so it stays C.
* GetZoneX1… replace only the low word of d0 (`MOVE.w`), JAdaptorStatus
  only the low byte, ZoneMode the low byte (thunk `move.b d0,3(sp)`);
  JHoriz(1) returns d0 unchanged for "right", like the original.
* ZoneInit/Zone without a table: the original reads the count from
  address 0 before it notices; the C build checks first (no visible
  difference). GetZoneX1… without a table read from address 0 in both.
* The thunks keep d1/a0/a1, the C code d2-d7/a2-a6; the original changes
  d1-d7, a0, a1 and a6 in places.

## BUG(orig)

* No routine checks a zone number for < 0 (ZoneInit, SetZone write before
  the table) (#873), nor NewZoneTable/UseZoneTable/FreeZoneTable a
  negative table number (entry -1 is `_zonetable` itself) (#872).
* NewZoneTable initialises the **current** table (`ZoneInit1` on
  `_zonetable`), not the new one, which keeps AllocMem's garbage (#875).
  If it replaces a table and the AllocMem fails, the entry keeps the
  freed block; if the replaced table is the current one, `_zonetable`
  points to freed memory, which ZoneInit1 then writes (#874).
* FreeZoneTable leaves the freed block in its entry (finit, or a
  NewZoneTable of that number, frees it again) (#868) and always makes
  table 0 the current one (#869). finit leaves all entries set, init
  clears only entry 0: a second init/finit frees the other tables again
  (#871).
* SetZone without a table returns -1 (= done); ZoneTable without a table
  returns -1, not 0 (#876).
* ZoneTest(start,end,x,y) finds the start zone through the low **byte**
  of start while the counter is a word: from zone 256 on it tests the
  wrong zones under the right numbers (#881).
* The 68000 test of x2/y2 uses `BMI` (the sign of x2-x), not `BLT`: wrong
  when the difference exceeds 32767 (e.g. y1 = -32768, y2 = 32767)
  (#879). The circle test squares word differences; the radius squared
  is stored as a word (radius >= 256 wraps) (#867), and the 68020 mode
  compares radius² and distance² as words (radius >= 182 fails) (#880).
  In the 68020 mode every zone with a negative y2 is a circle, in the
  68000 mode only y2 = -1 (#866).
* GetZoneX1/Y1/X2/Y2 compare the zone **count** with the field offset
  (0/2/4/6) instead of the zone number: no range check (a table of up to
  6 zones returns -1 for some fields), the index is a word, and without a
  table they read from address 0 (#870).
* ReadJoyPad reads the joystick port for every port number but 2 (#877).

Original behaviour that is no defect:

* JHoriz(1) "right" returns d0 unchanged: the port number 1 (upper bits
  included), which is also the result for "right".
* ZoneTest clamps a start behind the table to the last zone.
* NewZoneTable, UseZoneTable, FreeZoneTable test the table number with
  `BPL` on number-16 (signed, no overflow check); negative numbers are
  #872.

## Test

Each run loads one library set (the C build or the original); the
scenario writes its observations as cases, compared with the expected
data `libs/tests/expected/rwe_rizonejoylib.txt` recorded from the
original. Every form, in two sessions. With table 0: init,
SetZone (rectangles with swapped corners, a point, circles with radius 40,
200, 300 and 1, y2 = -1, a negative y2, x1 < 0, a y range that overflows,
a zone out of range), Zone on a 19 x 19 grid in the 68020 mode and the
68000 mode (341 / 270 hits – the modes differ),
ZoneTest with one zone and with ranges (clamped start, bad ranges, upper
bits), GetZoneX1/Y1/X2/Y2, ZoneInit's three forms with bad ranges;
NewZoneTable with bad numbers and sizes, a 300 zone table (ZoneTest from
zone 256 on), replacing a table, a 2 zone table (GetZoneY2 -1),
UseZoneTable, ZoneTableNumber, ZoneTableSize, FreeZoneTable (bad numbers,
the current table; its entry stays set); the joystick commands for ports
0..5 with the adaptor off and on, AllFire with all 16 bit masks,
ReadJoyPad for ports 0..2; finit (all memory back). Without memory
(AllocMem patched to fail for init): every zone command without a table.
Kept registers on every call. The test finds `zonetablelist` in the
library's data and clears the entries of freed tables, so they are not
freed twice. Not covered: negative zone/table numbers and zones behind a
table (they write or read outside the table), real joystick input (the
emulator has nobody at the joysticks: the idle values are read).
