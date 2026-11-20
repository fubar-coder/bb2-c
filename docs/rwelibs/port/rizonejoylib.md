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

`Zone` and `ZoneTest` (both forms) are assembler entries without a
thunk, for speed: a game tests a point against the zone table every
frame (`tests/perf/bench/bzone.bas`). In the C port behind a thunk they
took 915 cycles for `ZoneTest` of one rectangle against the original's
393, 975 for a `Zone` that hits zone 0 against 437; now 361 and 284
(`budgets.txt`). They change d1-d7 and a0 (`ZoneTest` with a range
keeps d2 and d3), as the original does, which the register rule allows
for a user-level function. The loops count with `DBF` and read a set
zone with `MOVEM.w`; `Zone` goes to the loop without the range checks
(start 0, end the last zone) unless the count is 0 or above $80000000.
All other routines are C behind `BLITZ_THUNK`.

* The 68020 mode tests rectangles with the original's `CMP2.w` (encoded
  as data, the library is built for the 68000; the mode is on only on a
  68020 or better); the 68000 mode keeps its own tests (`BGT` for x1/y1,
  `BLT` for x2/y2: fixed, #879).
* The word arithmetic of the original (`ADDQ.w` on a long counter, the
  start zone through `MOVE.b`) is kept with explicit masks, so
  out-of-range numbers address the same places; the zone offset of
  SetZone and ZoneInit is a long (fixed, #878).
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
  difference). GetZoneX1… without a table: -1 (fixed, #870).
* The thunks keep d1/a0/a1, the C code d2-d7/a2-a6; the original changes
  d1-d7, a0, a1 and a6 in places.

## Fixed original bugs (intended deviations)

* NewZoneTable, UseZoneTable and FreeZoneTable compare the table number
  unsigned: a negative number is refused (0), as the manual's "Valid
  zonetable numbers range from 0 to 15" says. The original compared it
  signed (`BPL` on number - 16): `UseZoneTable -1` selected entry -1 of
  the list, `_zonetable` itself; `FreeZoneTable -1` and `NewZoneTable -1`
  freed the current table and left it there (#872, test
  `rwe/rizonejoylib-negative-table-number`).
* ZoneInit (one zone and a range) and SetZone (both forms) compare the
  zone number unsigned: a negative number is refused, as the manual's "A
  zone number outside the range of the current table will cause this
  command to abort" says. The original checked only the upper bound
  (`CMP.l : BMI`): zone -1 is the 8 bytes in front of zone 0, the long in
  front of the table block and the table's count (#873, test
  `rwe/rizonejoylib-negative-zone-number`).
* SetZone and ZoneInit compute a zone's offset as a long: zones 8192 and
  above of a table of up to 65535 zones (the manual: "from 1 to 65536 in
  size") are their own. The original's `ASL.w #3` kept the low word, so
  zone n of a large table was zone n mod 8192 (#878, test
  `rwe/rizonejoylib-zone-offset-word`).
* ZoneTest finds its start zone through the whole number. The original
  took its low **byte** (`MOVE.b`) while it counts a word: from zone 256
  on it tested the wrong zones under the right numbers
  (`ZoneTest(260,5,5)` tested zone 4 and returned 260). With the long
  offset the range checks are unsigned (`BCC`/`BCS` for `BPL`/`BMI`), so
  the zones read stay inside the table: a negative start is beyond the
  table (the last zone, like any start beyond it), a negative end is
  refused (-1); a table with a count of 0 gives -1, as in the original
  (#881, test `rwe/rizonejoylib-zonetest-start-byte`; `libs_emulator`
  cases `[fixed #881]` of `rwe_rizonejoylib`, ZoneTest from zone 256 on
  in table 3).
* The 68000 mode tests x <= x2 and y <= y2 with signed compares (`BLT`).
  The original took the sign of the word differences x2 - x, y2 - y
  (`BMI`): wrong when a difference is beyond 32767, so a zone from y
  -32768 to 32767 did not contain the point (5,-1) (#879, test
  `rwe/rizonejoylib-zonetest000-bmi`; `libs_emulator` cases `[fixed #879]`
  of `rwe_rizonejoylib`, the grid of the 68000 mode).
* The 68020 mode compares a point's squared distance from a circle's
  centre with the radius squared as longs, as the 68000 mode does. The
  original compared them as words (`CMP.w`): a point whose squared
  distance has a low word at or below the radius squared hit (with
  radius 10 the point 256 pixels to the side, `Zone(256,3)`), and a
  radius of 182 or more (a radius squared above 32767) missed points
  inside (#880, test `rwe/rizonejoylib-zonetest020-circle-word`;
  `libs_emulator` cases `[fixed #880]` of `rwe_rizonejoylib`, the grid
  of the 68020 mode).
* Both modes take only y2 = -1 for a circle's mark (the manual's
  ZoneTable entry: "+6: -1 <-- this is set to show that the zone is
  circular"), and `SetZone num,x1,y1,x2,y2` refuses a rectangle whose
  bottom edge (the larger y) is exactly -1: it returns 0 and stores
  nothing, since such a rectangle cannot be told from a circle.
  Rectangles with y2 <= -2 stay valid and are rectangles in both modes.
  The original's 68020 mode took every zone with a negative y2 for a
  circle (a rectangle above the screen was tested as a circle around its
  corner), its 68000 mode only y2 = -1, and SetZone stored a bottom edge
  of -1 (#866, W7 decision Q2 (a); test `rwe/rizonejoylib-circle-marker-y2`;
  `libs_emulator` cases `[fixed #866]` of `rwe_rizonejoylib`). With #879
  and #880 the two modes give the same results for every zone SetZone
  makes; `ZoneMode`/`SetZoneMode` only choose the instructions (`CMP2`
  for rectangles in the 68020 mode).
* `SetZone num,x,y,radius` refuses a radius outside 0-255: it returns 0
  and stores nothing (W7 decision Q1 (a)). The radius is a word, like
  the coordinates (the original's `MULS.w`; the upper word of the long
  argument is not read). The original stored the low word of the radius
  squared, so radius 256 and more wrapped: 256 became a circle of radius
  0, 300 one of radius 156 (#867, test
  `rwe/rizonejoylib-circle-radius-word`; `libs_emulator` cases
  `[fixed #867]` of `rwe_rizonejoylib`, the circle of radius 300). Kept
  from the original: the zone tests square the word differences
  x - x1, y - y1, so a point more than 32767 from a circle's centre (far
  outside any screen) wraps and may hit the circle; not changed, it
  would cost an instruction per zone in the per-frame loop (the second
  half of #867).
* GetZoneX1/Y1/X2/Y2 return -1 without a table and for a zone at or
  beyond the table's count, as the manual says ("If the zone number
  supplied goes outside the size of the zonetable, then this command
  returns -1"); the zone (the word argument, unsigned) is indexed as a
  long. The original compared the zone **count** with the field's
  offset (0/2/4/6) instead of the zone number: no range check (a zone
  behind the table was read), -1 for X2, Y1 or Y2 of every zone of a
  table of up to 6 zones (a table of 2: X2, Y1, Y2), a word index, and
  without a table it read address 0 (#870, test
  `rwe/rizonejoylib-getzone-count-offset`; `libs_emulator` cases
  `[fixed #870]` of `rwe_rizonejoylib`: the 2-zone table, the calls
  without a table).
* Without a table (init could not allocate table 0) SetZone returns 0,
  as the manual's "returns TRUE or FALSE to say whether or not the
  change was made" says, and ZoneTable returns 0 (D-018: a function that
  returns an address returns 0 on out of memory). The original returned
  -1 for both: "done" and an address (#876, test
  `rwe/rizonejoylib-no-table-results`; `libs_emulator` cases
  `[fixed #876]` of `rwe_rizonejoylib`, the run without memory).
* FreeZoneTable clears the table's list entry. The original left the
  freed block there, so finit (or a NewZoneTable of the number) freed it
  a second time, and UseZoneTable of the number selected it: every
  program that used FreeZoneTable freed a block twice at its end (#868,
  test `rwe/rizonejoylib-freezonetable-entry-stays`; `libs_emulator`
  cases `[fixed #868]` of `rwe_rizonejoylib`: the entry after
  FreeZoneTable 7, UseZoneTable 7 after it).
* finit clears the list entries and the current table after it frees
  the tables, and init clears all 16 entries, so the next run of a
  program with the library still loaded (from the editor) starts with
  table 0 alone. The original's finit left them set and its init
  cleared only entry 0: the next run found the tables of the run
  before, freed (UseZoneTable selected one), and its finit freed them
  again (#871; D-019: no Blitz program can run twice with the library
  loaded, `libs_emulator` cases `[fixed #871]` of `rwe_rizonejoylib`,
  the scenario's second init).
* init sets the four player adaptor's status to on, as the manual says
  ("Default status for the adaptor is on"). The original set it only at
  the load (`Dc.b -1`), so after `JAdaptorStatus Off` a second run of
  the program from the editor started with the adaptor off (#1132;
  D-019, `libs_emulator` case `[fixed #1132]` of `rwe_rizonejoylib`:
  JAdaptorStatus's old status in the scenario's second run).
* NewZoneTable clears the zones of the new table and leaves the current
  table alone, as the manual says ("all zones are automatically reset in
  the new table and ... creating a table does not make it the current
  table"); a replaced current table stays current under its new block.
  The original ran `ZoneInit1` on the **current** table (`_zonetable`),
  so every NewZoneTable cleared the zones of the table in use and left
  AllocMem's garbage in the new one, and a replaced current table left
  `_zonetable` on the freed block (#875, test
  `rwe/rizonejoylib-newzonetable-inits-current`; `libs_emulator` cases
  `[fixed #875]` of `rwe_rizonejoylib`: table 0 after NewZoneTable 3).
* If NewZoneTable has freed the old table of that number and the
  AllocMem fails, the table no longer exists: its entry is cleared, the
  result is 0, and if it was the current table, table 0 becomes current
  (as FreeZoneTable does) (D-018). The original left the freed block in
  the entry, where UseZoneTable selected it and finit freed it again,
  and, for the current table, in `_zonetable` (#874, test
  `rwe/rizonejoylib-newzonetable-allocfail-stale-entry`).

## BUG(orig)

* FreeZoneTable always makes table 0 the current one (#869).
* ReadJoyPad reads the joystick port for every port number but 2 (#877).

Original behaviour that is no defect:

* JHoriz(1) "right" returns d0 unchanged: the port number 1 (upper bits
  included), which is also the result for "right".
* ZoneTest clamps a start behind the table to the last zone.

## Test

Each run loads one library set (the C build or the original); the
scenario writes its observations as cases, compared with the expected
data `libs/tests/expected/rwe_rizonejoylib.txt` recorded from the
original. Every form, in two sessions. With table 0: init,
SetZone (rectangles with swapped corners, a point, circles with radius 40,
200, 300 and 1, y2 = -1, a negative y2, x1 < 0, a y range that overflows,
a zone out of range), Zone on a 19 x 19 grid in the 68020 mode and the
68000 mode (270 hits in both; the original 341 / 270),
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
