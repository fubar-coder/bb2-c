# rishapeslib (46): shapes in memory, the LE shapes format

Reflective Images. Shapes that are already in memory become shapes
(`CludgeShapes`, `CludgeShapesToAddress`, `LECludgeShapes`). There is
also a shapes file format of its own ("LE": `FORM`/`LESH` with `SHHD`,
an optional `CMAP` and `SDAT`, optionally ByteRun1-packed by RIPackLib),
which `LESaveShapes` writes and `LELoadShapes` reads. A file that is not
an LE file goes to shapeslib's `LoadShapes`. The other entries:
`SafeToBlit`, `LEInitShape`, `LEScanShapes` with `LEMaxShapeWidth/Height/Depth`,
`LECompressShapes`, `LEAutoCookie`. init clears the compression flag;
finit does nothing.

* **Source:** `rwelibsrc/shapeslib/RIShapesLib2.bb` (31/8/97, BB22
  format; as TED shows it: `build/amiga/rwelibsrc/shapeslib/RIShapesLib2.bb`).
* **Reference:** the reference build of that source (`REFERENCE
  refbuild`, `build/amiga/libs/userlibs/rwe/ref/rishapeslib.obj`). The CD
  binaries (BUM9B … BSS2, AB3, all the same file) are an older state
  without `LEAutoCookie` and `CludgeShapesToAddress`
  (`docs/rwelibs/README.md` §3.2). The declarations are equal (ctest
  `rwe_declarations_rishapeslib`).
* **C file:** `libs/userlibs/rwe/rishapeslib.c`; test
  `libs/tests/rwe/rishapeslib.c` (snitch case "rwe rishapeslib").

## Declarations

The source writes some `!libs` flags without `#` (`#ia3|pd1`,
`#ia0|pd3`, `#doslib,la6`). The reference build gives them the values of
the constants (pd1, pd3, la6), so the C file uses the normal flags.
`LELoadShapes` form 3 (`shape#,filename$,palette#`) asks for the palette
of **d3**, although that form has only three arguments (d0–d2), as the
source's comment "This little bastard won't work below" warns.

## C and assembler

Everything is C behind thunks: d0/d1/a0/a1 are kept for the statements,
d1/a0/a1 for the functions. The original's register arithmetic is
reproduced. This covers `MULU.w`, `DIVU #3` with its overflow
(`bl_mulu_w`/`bl_divu_w`), word additions (`ADDQ #8,d0`, `ADD.w` into a
long size), `ADDA.w` of a sign-extended word (the last shape of the
one-shape forms), DBRA on words (65536 iterations for a count of 0) and
signed or unsigned compares.

The variables are statics. `bytespershape` and `centry` carry over from
one call to the next, as in the original. `MOVE.w #0,bytespershape`
clears only the upper word of the long.

Calls into other libraries (ALibJsr):

* memlib `globalloc`/`globfree`.
* RIPackLib (73): `PackPBM` (token `#ripacklib<<7+1`, d0 = source,
  d1 = destination, d2 = length, d3 = 12 as the original leaves it,
  through `BL_JSR_D4_`) and `UnPackPBM` (`+8`, d0–d3).
* shapeslib `LoadShapes` form 1 (`#shapeslib<<7+10`). This call is
  inline assembler, because the callee takes the first shape in a3 (the
  register fixed for GCC) and the last one in a2. The original passes
  them exchanged (below).

Further assembler:

* **dos Write with the flags it leaves.** In `_saveCMAP`,
  `MOVE.l paletteptr,a0` is a MOVEA and sets no flags. The `BNE` behind
  it therefore tests the Z flag that dos `Write` of the file header left.
  `write_z` calls Write and records that flag with `seq`.
* **The stack imbalances.** The entries that call `readshapes`
  (LELoadShapes, LECludgeShapes) record the stack pointer in their
  thunk (`rs_sp`). A cludged shape (`"cL"`) in the target range runs a
  `MOVE.l (a7)+,a0` without its push:
  * The first such pop yields readshapes' return address (the C
    analogue of the original's return address into the library code), and
    the routine then returns to the entry's caller (the C caller returns
    at once).
  * Further pops yield the longs of the Blitz program's stack. The final
    RTS then jumps there (`rs_return`).

  An allocation that fails in the shape loop or in the cookie returns
  with a0 still pushed. The original returns to that data address;
  `rs_crash` jumps there.

## BUG(orig), kept

* **Shape numbers shifted as object offsets (`ASR #5`):**
  * `SafeToBlit` compares shape/32 with the maximum.
  * `LESaveShapes` stores (s2>>5)−(s1>>5) as the count, with the upper
    word of d1 uncleared.
  * The shapes handed on to shapeslib's `LoadShapes` are s>>5.

  The loader reads count + 1 shapes (`SUB.l #1:BMI`), limited by the
  last shape. So a file of shapes below 32 loads one shape.
* **CludgeShapes:**
  * An empty shape (width or height 0) advances by 32 + the low word of
    the size of the shape before it, which may come from an earlier call.
  * A size of 0 copies 65536 bytes.
  * shape# is divided by 32 and never used.
* **The cookie of a freed shape is never freed** (LELoadShapes,
  LECludgeShapes, LEInitShape). `MOVE.l 18(a2),a1:BEQ` after
  `CLR 30(a2)` tests the CLR, so the branch is always taken.
* **Cludged shapes reloaded:** see the stack imbalances above.
  LELoadShapes then neither frees its buffers nor closes the file.
* **LELoadShapes:**
  * Form 3 uses the palette of d3.
  * A non-LE file goes to `LoadShapes` with a3 = the last shape and
    a2 = the first. One shape is loaded into the last shape.
  * A failing chunk allocation runs the caller's cleanup and returns
    there. The caller then frees the header buffer and closes the file a
    second time if packedsize is 0, and otherwise uses packedsize as the
    chunk.
  * The result in d0 is `packedsize` of the last packed shape.
* **read_palette:**
  * The allocation is not checked; a failure writes to address 0.
  * The colour count is `DIVU #3` of the chunk size; on overflow the
    register is unchanged.
  * 0 colours decode 65536.
  * A CMAP size that is not a multiple of 3 makes LECludgeShapes miss
    SDAT.
* **LESaveShapes:**
  * Whether the CMAP is written depends on Write's Z flag (above). In
    the emulator (OS 3.2) it is written.
  * The FORM length is 8 too large and contains the CMAP size of the
    last save that had a palette.
  * The shape header buffer is allocated with the upper word of the d0
    that `_saveCMAP` leaves, and freed with size 14 instead of 12.
  * A palette of 0 colours copies 65536.
  * When the pack buffer is missing, two runtime errors follow (TRAP #0:
    "Not enough memory to pack shapes", "Shape Compression Failed!"),
    then CreateCookie on the shape, whose RTS returns from
    `_writeshapes`.
  * PackPBM's failure runs UnPackPBM with d3 = 12 (RIPackLib's
    BUG(orig)).
* **LECludgeShapes:**
  * The SHHD chunk is skipped as 20 bytes, whatever its length.
  * Memory below 2 MB that is not compressed is linked, not copied
    (`"cL"`), unless copymemflag is negative. The form without
    copymemflag links.
* **LEInitShape:** the data and the cookie are cleared chip memory; the
  cookie is not computed.

## Test

Each set has its own memlib, shapeslib and RIPackLib (originals: the
CD's binaries; C set: this build), its own 70 shape objects, its own
palettes and bitmap object, and its own files in RAM:.

Compared:

* the results;
* the shape objects: every field but the pointers; the data and cookie
  bytes; for linked shapes, the offset of the data in the source buffer;
* the palettes;
* the saved files, byte for byte;
* the kept registers;
* memlib's memory, all back at the end.

Checks of the expected effect make sure the scenarios do what they say
(one shape loaded, packed shapes loaded, "cL", LoadShapes into the last
shape, the stale-size skip).

Covered:

* LEInitShape on fresh and on used shapes.
* LEScanShapes and LEMax*.
* SafeToBlit: limits, negative coordinates, no cookie, shape/32 against
  the maximum.
* LESaveShapes: both forms, with a palette, packed (packable data and
  data PackPBM fails on), count 0 and 2, the upper word in the count.
* LELoadShapes: all four forms; into used shapes; a missing file; FORM
  without LESH; a shapeslib file.
* LECludgeShapes: from fast memory; from chip memory, copied (flag −1),
  linked, and packed; with and without a palette; a CMAP of 7 bytes; no
  FORM.
* CludgeShapes and CludgeShapesToAddress: with and without LEAutoCookie,
  empty shapes, 0 shapes, used and linked targets.
* init and finit.

Not covered: the stack imbalances and failing allocations (they jump
into data), failing chunk allocations, a palette of 0 colours, the pack
buffer's runtime error.
