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
blue pixels. init and finit are empty.

* **Source:** `rwelibsrc/Snow/SnowLib3.bb` (as TED shows it:
  `build/amiga/rwelibsrc/Snow/SnowLib3.bb`).
* **Reference:** the reference build of the source by blitz2 2.10
  (`refbuild`, `build/amiga/libs/userlibs/rwe/ref/risnowlib.obj`); no
  binary of the library exists (`docs/rwelibs/README.md` §2).
  Declarations equal (ctest `rwe_declarations_risnowlib`).
* **C file:** `libs/userlibs/rwe/risnowlib.c`; test
  `libs/tests/rwe/risnowlib.c` (snitch case "rwe risnowlib").

## C and assembler

All C behind `BLITZ_THUNK`s (keep d1/a0/a1, d0 too for init and finit;
the original changes d0–d7/a0–a3 and keeps a4–a6). The bit operations
are the original's `BTST`/`BCLR`/`BSET` on bytes (bit modulo 8, word
indexes), the lookup tables keep their order (left-first, then
right-first), COLOR00 is written as the original writes it.

## BUG(orig), kept

* No routine checks the list number (0–3; `ASL.w #4` as the index)
  (#913); InitSnowDrop computes the size with `ASL.w #2` on
  numpoints × 16 (from numpoints 1024 on the block is too small) (#912)
  and never frees an earlier block (#911).
* The drop lists' "last" stays 0 until the first LDoSnow:
  LDoSnowActivate before it writes to address 0 (#908).
* The lists have no bounds: more than numpoints × 4 entries run into the
  next list and behind the block (#914).
* A bit number above 7 from the random table indexes the lookup tables
  beyond their end; the table's offsets are bytes (the first rows only)
  (#910).
* LDoSnowActivate does not update the drop list's "last" (#909).

## Original behaviour, not defects

* The allocation is not checked: memlib's `globalloc` raises Out of
  Memory itself.
* LDoSnowActivate tests plane 2 at the killed pixel again (always clear
  there): redundant, without effect.
* The end of LDoSnow behind its loop (`'endthis`) is dead code: the loop
  ends only at `$ABCD`, with a new flake.

## Test

Per set a bitmap object with three planes of 40 × 48 bytes in one
memory area with margins (flakes at the edges read and write next to
the planes), the planes filled with the same random ground, obstacles,
pause and blue pixels; a random table with bits 0–7. COLOR00 is
redirected: every absolute `$DFF180` in the code of both loaded
libraries is patched to a word per set (the display is not touched).
memlib is loaded for `globalloc`. Compared after every call: the
memory area, the random table, the list table and the field blocks
(pointers as offsets), the COLOR00 word, d0 where there is a result,
kept registers. Cases: two fields on the same bitmap, 140 LDoSnow steps
on field 0 and 47 on field 1 (the table index wraps at 1024), 120
KillPoints (blue and clear pixels, row 0) and 20 LDoSnowActivates.
Checked to fail when the alternation, the stop in plane 1, the removal
at the bottom, the second side or one of the LDoSnowActivate tests is
changed. Not covered: the BUG(orig) cases above that write outside the
library's memory.
