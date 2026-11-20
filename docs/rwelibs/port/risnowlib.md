# risnowlib (195): falling snow

Reflective Images, 1994. Snow that falls through a bitmap, kept as
lists of pixels; four commands:

* `InitSnowDrop(listnum,numpoints,bitmap,bp_size)` – allocates the lists
  of a snow field (memlib's `globalloc`, `MEMF_CLEAR`) and returns the
  address of the library's list table.
* `LDoSnow listnum,randomtable` – one step: every flake moves one row
  down if the pixel there is clear, else to the side (left or right
  first, alternating per blocked flake) where that pixel and the one
  below it are clear; a flake that cannot move stops if plane 1 has a
  pixel below it (it is set in plane 1 and leaves the list), a flake
  below `bp_size` is removed. Then a new flake from the random table
  (word index, then byte pairs offset/bit; the index steps by 2 modulo
  1024). COLOR00 is red while the routine runs (`$F00`, then 0: a
  debug flash).
* `KillPoint(listnum,x,y)` – a "blue" pixel (plane 2) at x,y is cleared
  in all three planes and the pixel above it added to the kill list:
  -1, else 0.
* `LDoSnowActivate(listnum)` – for every entry of the kill list, stopped
  snow (plane 1 set, plane 2 clear) in the row below it, left and right
  (the left-first table), and at the entry itself falls again (cleared
  in plane 1, added to the drop list); if any did, the entry moves one
  row up into the other kill list. Returns the number of entries + 1.

The list table has 4 longs per list number (drop list A and B – read
and written in turns –, kill list 1 and 2). The block of a field holds
the four lists of numpoints × 16 bytes each: a drop list starts with
size, bitmap object, bp_size and "last" (its end), a kill list with
"last"; entries are offset.w (the byte in the plane), bit.w, ended by
`$ABCD`. Plane 0 is the snow, plane 1 the stopped snow, plane 2 the
blue pixels. init clears the list table, finit is empty.

* **Source:** `rwelibsrc/Snow/SnowLib3.bb` (as TED shows it:
  `build/amiga/rwelibsrc/Snow/SnowLib3.bb`).
* **Reference:** the reference build of the source by blitz2 2.10
  (`refbuild`, `build/amiga/libs/userlibs/rwe/ref/risnowlib.obj`); no
  binary of the library exists (`docs/rwelibs/README.md` §2).
  Declarations equal (ctest `rwe_declarations_risnowlib`).
* **C file:** `libs/userlibs/rwe/risnowlib.c`; test
  `libs/tests/rwe/risnowlib.c` (snitch case "rwe risnowlib").

## C and assembler

`LDoSnow` is an assembler entry, the original's loop (its registers: a1
plane 0, a4 plane 1, a0 the list read, a2 the list written, a5 the
random table, d7 bp_size, d6 left or right first, a3 the side table): as
C it took 4.2 times the original's cycles per call, with the bit
operations as inline assembler still 1.7 times (perf `bsnow`: 163309
and 67339 against 38979 cycles for a field of about 255 falling
flakes); the assembler takes 39009 (1.00). It keeps d1/a0/a1 and a4-a6.
The rest is C behind `BLITZ_THUNK`s (keep d1/a0/a1, d0 too for init and
finit; the original changes d0–d7/a0–a3 and keeps a4–a6). The bit
operations are the original's `BTST`/`BCLR`/`BSET Dn,0(An,Dm.w)` on
bytes (bit modulo 8, word indexes) as inline assembler, the lookup
tables keep their order (left-first, then right-first), COLOR00 is
written as the original writes it.

## Fixed (intended deviations)

* InitSnowDrop computes the size of its block (4 × numpoints × 16 + 40)
  as a long; the original took the factor 4 with `ASL.w #2`, which
  keeps the low word only: from numpoints 1024 on the block was too
  small and the lists were written behind it (#912, test
  `rwe/risnowlib-initsnowdrop-size-aslw`).
* InitSnowDrop sets the drop lists' "last" to their end mark (their
  first entry), as it does for the kill lists; the original skipped the
  field (`ADDQ.l #4,a0`), so it stayed 0 until the first LDoSnow and an
  LDoSnowActivate before it appended its entries at address 0 and wrote
  `$ABCD` there (#908, test `rwe/risnowlib-activate-before-dosnow`).
* LDoSnowActivate stores the new end of the drop list into its "last"
  (at `'endthis`); the original stored only the kill list's, so a second
  LDoSnowActivate before the next LDoSnow started at the old end and
  overwrote the flakes the first had released: cleared in plane 1, they
  vanished without falling (#909, test
  `rwe/risnowlib-activate-last-not-updated`).
* init clears the list table, as at the first load; the original's init
  is empty, so in a second run of a program from the editor (the
  library stays loaded, memlib has freed the blocks) every entry pointed
  to a list the first run had freed, and a snow command before
  InitSnowDrop worked on it (#1135; D-019: `libs_emulator` cases
  `[fixed #1135]` of `rwe_risnowlib`, the scenario's second run).
* InitSnowDrop frees the block the list number had (memlib's
  `globfree`; the block starts with drop list A, the lower of the two
  drop list pointers, which LDoSnow swaps) before it allocates the new
  one; the original overwrote the pointers, so every repeated
  InitSnowDrop of a number lost numpoints × 64 + 40 bytes until the end
  of the program (#911, test `rwe/risnowlib-initsnowdrop-no-free`). It
  needs the cleared table of #1135: a first run's block is never freed
  twice.

* With runtime error checking every command refuses a list number
  outside 0–3: "Snow list number must be 0 to 3" (the original has no
  error check area). The release code is unchanged (an invalid argument,
  D-005): the number indexes the table of 4 × 16 bytes with `ASL.w #4`
  and no check, so 4 and up, negative numbers and a non-zero upper word
  read and write behind or before the table (`InitSnowDrop` writes its
  four pointers there) (#913, test `rwe/risnowlib-listnum-unchecked`).
  With it `LDoSnow`, `KillPoint` and `LDoSnowActivate` refuse a list
  number without `InitSnowDrop` ("Snow list needs InitSnowDrop": its
  entry is 0, the lists and the bitmap would be read at address 0), and
  `KillPoint` a pixel outside the 320 pixels of a row or the rows of the
  field's bitmap ("KillPoint outside the bitmap": the library assumes 40
  bytes per row).

* LDoSnow takes the bit of a new flake from the random table modulo 8
  (`AND.w #7` behind the byte read, 8 cycles per call); the original took
  the whole byte, and a blocked flake with a bit above 7 looked up its
  side moves bit × 8 bytes into the tables of 8 rows: bits 8–15 read the
  right-first table behind the left-first one (the wrong neighbours),
  higher ones the code behind the tables, and the flake jumped up to
  ±32 KB from its place, every frame again (#910, test
  `rwe/risnowlib-bit-above-7-lookup`). The bit operations themselves took
  the bit modulo 8.

* The lists are bounded: a list of numpoints × 16 bytes takes at most
  numpoints × 4 − 1 entries and its end mark. `LDoSnow` takes no new
  flake into a full list (one compare per call: every other flake it
  writes replaces one it read; about 40 cycles per call in `bsnow`,
  0.1 %),
  `LDoSnowActivate` leaves stopped snow as it is when the drop list is
  full, `KillPoint` does nothing and returns 0 when the kill list is
  full; `LDoSnowActivate` writes at most as many kill entries as it
  reads. The original counted nothing: a field whose flakes pile up (a
  bitmap without ground, a large bp_size) overran its drop list into the
  next list and, from the last list, behind the block (#914, test
  `rwe/risnowlib-lists-unbounded`).

## Original behaviour, not defects

* The random table's offsets are bytes: new flakes start in the first
  6.4 rows (offsets 0–255).
* The allocation is not checked: memlib's `globalloc` raises Out of
  Memory itself.
* LDoSnowActivate tests plane 2 at the killed pixel again (always clear
  there): redundant, without effect.
* The end of LDoSnow behind its loop (`'endthis`) is dead code: the loop
  ends only at `$ABCD`, with a new flake.

## Test

Each run loads one library set (the C build or the original); the
scenario writes its observations as cases, compared with the expected
data `libs/tests/expected/rwe_risnowlib.txt` recorded from the original.
A bitmap object with three planes of 40 × 48 bytes in one
memory area with margins (flakes at the edges read and write next to
the planes), the planes filled with random ground, obstacles,
pause and blue pixels; a random table with bits 0–7. COLOR00 is
redirected: every absolute `$DFF180` in the code of the loaded
library is patched to a word in RAM (the display is not touched; at
least one address must be patched, their number differs between the
builds). memlib is loaded for `globalloc`. Observed after every call: the
memory area, the random table, the list table and the field blocks
(pointers as offsets), the COLOR00 word, d0 where there is a result,
kept registers. Cases: two fields on the same bitmap, 140 LDoSnow steps
on field 0 and 47 on field 1 (the table index wraps at 1024), 120
KillPoints (blue and clear pixels, row 0) and 20 LDoSnowActivates,
then two pairs of KillPoint and LDoSnowActivate without an LDoSnow
between them, each releasing the stopped snow above a blue pixel made
for it (#909), and InitSnowDrop again for list number 1 (#911: the
memory the old block keeps, AvailMem before and after).
Then a second run of a program with the library still
loaded (as from the editor): memlib's finit has freed the blocks,
memlib's and the library's init run and the list table is observed
(#1135).
Checked to fail when the alternation, the stop in plane 1, the removal
at the bottom, the second side or one of the LDoSnowActivate tests is
changed. Not covered: the fixed cases above in which the original
writes outside the library's memory (#913, #912, #914, #910, #908);
their bug-orig tests show them.
