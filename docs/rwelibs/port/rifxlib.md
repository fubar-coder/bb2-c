# rifxlib (66)

**What it does:** effects on planar bitmaps (Stephen McNamara,
Reflective Images, 1994–96). `ClearBitmap` and `FadeInBitmap` work line
by line with an alternating dither mask and a busy-wait delay; `ZoomX2`,
`ZoomX4` (optional x-only/y-only), `ZoomX8`, `ZoomyZoom` and `ReduceX2`
use lookup tables that init builds (1856 bytes through memlib
`globalloc`: reduce 256 bytes, x2 256 words, x4 256 longs, x8 16 longs;
`GetZoomTables` returns the address of the four table pointers);
`Derez`, `ZoomXY` (after `InitZoomXY`) and their `ScreenCopy` work on
lores bitmaps of 40 bytes per line; `ADDValue` = byte offset of x,y.

**Source / reference:** `rwelibsrc/FXLib/FX-Library13.bb`, BSS2
`RIFXLIB.OBJ` (the source builds to it byte for byte,
[README](../README.md) 3.1). C file: `libs/userlibs/rwe/rifxlib.c`.

**C:** every routine, with the original's loops: word arithmetic and the
DBRA counts are kept (`do … while (n-- != 0)` = n+1 turns, 65536 for
$FFFF), addresses are ULONGs as in the registers, the variables keep
their meaning across calls (e.g. `sbitdepth`, shared by `Derez` and
`ZoomXY`). `bit_add`, `dest_add` and the four `InitZoomXY` values are one
array, as they follow each other in the original, so the unchecked plane
stores run from one into the next the same way. memlib is called with
`BLITZ_ALIBJSR` (`TOK_globalloc`/`TOK_globfree`), 16 × 16 bit products
with `bl_mulu_w` (MULU.w). `ClearBitmap` and `FadeInBitmap` are plain C
functions as entries (the original keeps only a4-a6); the routines whose
original keeps every register (`MOVEM.l d0-d7/a0-a6`, the zooms,
`InitZoomXY`, `ZoomXY`, `ZoomyZoom`) are behind thunks that keep
d0/d1/a0/a1, `Derez` keeps d1/a0/a1, `ADDValue` a0/a1. The delay loop
counts a `volatile` variable (the compiler would drop an empty loop).

**Assembler:** none except the thunks (`BLITZ_THUNK`).

**BUG(orig)** (kept, commented in the code):

* `ClearBitmap` starts at end-4 (its comment says 2): every line pass is
  one word too low, the last word of the area stays and the last pass
  clears the word before the area (before the bitplane for offset 0)
  (#668);
* `ClearBitmap`, `FadeInBitmap`, `ZoomyZoom` read the depth as the word
  at 4 (`bm_Flags:bm_Depth`), right only while `bm_Flags` is 0 (#670);
* delay 0 in `ClearBitmap`/`FadeInBitmap` means 2^32 turns of the busy
  loop (#669);
* `Derez` is declared with a result (`!acommand{#long}`) but sets none:
  d0 is whatever the routine leaves there (the input for 0, the input's
  upper word and $FFFF for 1, the last long for 2, the last mask word
  swapped/the last source word for the others). The C version returns
  exactly that – it is what a Blitz program gets (#672);
* `Derez` and `ZoomXY` set only the low byte of `sbitdepth` (`MOVE.b`
  into `sbitdepth+1`): after a depth-0 bitmap the upper byte is $FF and
  stays (#675);
* `Derez` with a value ≥ 3 builds its mask table `myline` with 20 words
  (21 for values > 31) into the 18 words the source reserves: the 19th
  goes over `myflag` and the pad byte, the 20th (and 21st) over the first
  instruction(s) of `ZoomXY` (`TST.w d0`, `BEQ …`; checked in the CD
  binary: `myline` at $F84, `_AsmZoom` at $FAA of the file). After such a
  `Derez`, `ZoomXY` of the original runs a mask word as its first
  instruction (for 3: `MOVE.l (a2),d2`, the height comes from memory)
  (#671);
* `ZoomyZoom` ignores the bitmap's size: fixed 32 lines of 80 bytes, the
  left 40 bytes doubled in place (#676);
* the pointer arrays hold 16 longs each and the depth is not checked
  (#674): `FadeInBitmap` stores two per plane in each, so from 9 planes
  on it runs from `bit_add` into `dest_add` and from `dest_add` over the
  four variables behind it, from 11 planes on beyond the 36 longs of
  the array; `ClearBitmap`, `Derez` and `ZoomXY` (one per plane) reach
  the variables from 17 planes on and go beyond the array from 21 on.
  Beyond the array the original overwrites the code of `InitZoomXY`;
* finit frees the tables but keeps the pointers (#673).

Not bugs but limits the source names: `ZoomXY` and `Derez` (and
`ScreenCopy`) only for lores bitmaps (40 bytes per line).

**DIFF:** `myline` has room for 21 words, so `ZoomXY` stays intact after
a `Derez` ≥ 3 (code that corrupts code cannot be translated; the mask
words read back are the same). The overflow of the 36 pointer longs
(`FadeInBitmap` from 11 planes, the others from 21) lands on other
variables of the C build.

**Test** (`libs/tests/rwe/rifxlib.c`, snitch case "rwe rifxlib", with
memlib): every call on both sets with the same inputs – bitmaps in an
arena per set (same layout, the same pseudo-random contents before every
call, 64-byte gaps around the planes), the other registers markers;
compared are the whole arena afterwards, d0 where there is a result, and
the registers the original keeps (about 350 checks):

* the zooms and `GetZoomTables` before init (no tables: nothing
  happens);
* `GetZoomTables` after init: offsets of the four tables and their 1856
  bytes;
* `ADDValue` with x/y values including $8000/$FFFF (ASR.w) and garbage in
  the upper words;
* `ClearBitmap` and `FadeInBitmap`, both forms, with height/offset,
  depths 1–3, delay 1–3 (the word before the area included);
* `ZoomX2`, `ZoomX4` (both forms, `xy_select` 0–3 from -1(a2)), `ZoomX8`,
  `ReduceX2` with source/dest offsets (also odd), and with widths/heights
  that end them early (width < 8, -1, height 0, `ReduceX2` height 1);
* `ZoomyZoom` (also on a bitmap of another size);
* `InitZoomXY` + `ZoomXY` with 1,1 (screen copy) and factors up to 5,4,
  0 factors, garbage in the upper words;
* `Derez` 0, 1, 2 (two heights), 3, 4, 5, 7, 16, 17 (mask table), 18, 25,
  40 (bit by bit, 40 with the 21-word table), $FFFF0003 – after the
  `ZoomXY` tests (see above).

**Not covered:** delay 0 (2^32 turns), bitmaps with `bm_Flags` ≠ 0 (the
depth word gives thousands of planes), more than 8 planes, `ZoomXY`
after `Derez` ≥ 3 (the original's code is corrupt then).
