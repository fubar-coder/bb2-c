# richunkylib (24): chunky bitmaps

Reflective Images (S. Matty, 14/8/95). An object library for chunky
(one byte per pixel) bitmaps, all fixed to 320 × 256 × 8:

* `ChunkyBitmap chunky#,width,height` – width × height bytes (memlib's
  `globalloc`, `MEMF_CLEAR`); data the object already holds is freed
  first.
* `ChunkyToPlanar chunky#,bitmap#` – chunky to planar into a bitmap of
  depth ≥ 8 (81920 pixels, the 8 planes contiguous from the first).
* `PlanarToChunky bitmap#,chunky#`, `ChunkyLoadBitMap filename$,chunky#`
  – no code (`RTS`).
* `ChunkyData(chunky#)` – the address of the data.
* `RaycastSky chunky#,x,height,column,&texture` – copies `height` bytes
  of a texture column (from `texture` + `column`) to the chunky column
  at `x`, both 320 bytes per row.

The object has 16 bytes (`!libfin` maximum 8, shift 4): width.w,
height.w, depth.w (8), an unused word, the data pointer, 4 unused
bytes. init and finit are empty; the object manager's free routine
(a3 = the object) frees the data.

* **Source:** `rwelibsrc/chunkylib.bb` (as TED shows it:
  `build/amiga/rwelibsrc/chunkylib.bb`).
* **Reference:** the reference build of the source by blitz2 2.10
  (`refbuild`, `build/amiga/libs/userlibs/rwe/ref/richunkylib.obj`); no
  binary of the library exists, and `chunkylib2` does not compile
  (`docs/rwelibs/README.md` §2). Declarations equal (ctest
  `rwe_declarations_richunkylib`).
* **C file:** `libs/userlibs/rwe/richunkylib.c`; test
  `libs/tests/rwe/richunkylib.c` (snitch case "rwe richunkylib").

## C and assembler

All C behind `BLITZ_THUNK`s (keep d1/a0/a1, d0 too for the empty
routines; the original changes d0/d1/d7/a0/a1 in ChunkyBitmap and the
free routine, d2/a0/a1 in RaycastSky). The calls of lmullib
(`lmultiply`) and memlib (`globalloc`, `globfree`) are `ALibJsr`s with
the original's registers. The C2P is the original's merge network
(the masks `$0F0F0F0F`, `$3333CCCC`, `$55555555`, 8 pixels per step, 32
pixels buffered and written as longs in the original's plane order) in
C.

## Fixed

Intended deviations from the original (`docs/libs/deviations.md`,
kind F):

* The free routine (ChunkyBitmap on an object with data, the end of the
  program) leaves CIA-A PRA bit 1 (the power LED / audio filter) as it
  is; the original toggled it with a `BCHG #1,$bfe001` left in (#886).
* ChunkyToPlanar tests the byte `bm_Depth` (below 8: nothing); the
  original compared the word `bm_Flags` * 256 + `bm_Depth` signed
  (`CMPI.w #8,4(a1)`), so a Flags byte of 1-127 let any depth pass (8
  planes written into a bitmap of fewer) and one with bit 7 set refused
  even a bitmap of depth 8 (#883).
* ChunkyBitmap frees the data the object holds when its data pointer is
  not 0; the original tested the long at offset 6 (`TST.l 6(a0)`: the
  unused word and the upper word of the data pointer), so data below
  `$10000` was never freed, and a ChunkyBitmap on such an object whose
  allocation failed kept the old pointer with width, height and depth 0
  (#885).

## BUG(orig), kept

* ChunkyToPlanar checks nothing but the depth and the data pointer: the
  sizes of both bitmaps are assumed (#884).
* RaycastSky checks nothing; x and column are added as signed words, the
  height is a `DBRA` count (0 copies 65536 rows) (#887).

Not a defect: the free routine extends the width twice and not the
height (`EXT.l d0` twice), so the size it computes has the caller's
upper word of d1 – but the product is unused (`globfree` needs no
size).

## Test

Each run loads one library set (the C build or the original); the
scenario writes its observations as cases, compared with the expected
data `libs/tests/expected/rwe_richunkylib.txt` recorded from the
original. The objects, a `struct BitMap` with 8 contiguous planes and a
guard, and a texture are built by the test. memlib is replaced by stubs
for `globalloc` and `globfree` (the real one ends the program with a
runtime error when an allocation fails; the stub can fail like
`AllocMem`, so the library's failure path is tested) that count the
blocks; lmullib is loaded. Observed after every call: the objects (the data
pointer as 0 / not 0), the chunky data, the planes with the guard, the
block count, CIA-A PRA bit 1, d0 where there is a result, kept
registers. Cases: ChunkyData without and with data, ChunkyBitmap new /
again with data (freed first) / with failing allocations / with the
unused word set but no data / on data below `$10000` (the address
`$1234`, whose frees the `globfree` stub counts instead of freeing it;
with and without a failing allocation, `[fixed #885]`), two rounds of random chunky data through
ChunkyToPlanar (depth words 8 and `$0108`; 7, `$8008` and no data do
nothing), 40 random RaycastSky calls (heights 1–256, marker upper
words, negative columns), PlanarToChunky, ChunkyLoadBitMap, the free
routine with and without data, init, finit. Not covered: RaycastSky
with height 0 or a negative x (writes outside the data).
