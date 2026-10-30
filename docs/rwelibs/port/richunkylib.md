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

## BUG(orig), kept

* ChunkyBitmap decides whether the object holds data by `TST.l 6(a0)`:
  the unused word and the upper word of the data pointer. Data below
  `$10000` is never freed, and a ChunkyBitmap on such an object whose
  allocation fails keeps the old pointer.
* The free routine extends the width twice and not the height (`EXT.l
  d0` twice): the size it computes has the caller's upper word of d1
  (unused, `globfree` needs no size).
* The free routine toggles CIA-A PRA bit 1 (`BCHG #1,$bfe001`, the power
  LED / audio filter), a debug flash left in.
* ChunkyToPlanar checks nothing but the depth word (signed, `bm_Flags`
  in its upper byte) and the data pointer: the sizes of both bitmaps are
  assumed.
* RaycastSky checks nothing; x and column are added as signed words, the
  height is a `DBRA` count (0 copies 65536 rows).

## Test

The objects, a `struct BitMap` with 8 contiguous planes and a guard, and
a texture are built per set. memlib is replaced by stubs for `globalloc`
and `globfree` (the real one ends the program with a runtime error when
an allocation fails; the stub can fail like `AllocMem`, so the
library's failure path is tested) that count the blocks of each set;
lmullib is loaded. Compared after every call: the objects (the data
pointer as 0 / not 0), the chunky data, the planes with the guard, the
block count, CIA-A PRA bit 1, d0 where there is a result, kept
registers. Cases: ChunkyData without and with data, ChunkyBitmap new /
again with data (freed first) / with failing allocations / with the
unused word set but no data, two rounds of random chunky data through
ChunkyToPlanar (depth words 8 and `$0108`; 7, `$8008` and no data do
nothing), 40 random RaycastSky calls (heights 1–256, marker upper
words, negative columns), PlanarToChunky, ChunkyLoadBitMap, the free
routine with and without data, init, finit. Not covered: RaycastSky
with height 0 or a negative x (writes outside the data).
