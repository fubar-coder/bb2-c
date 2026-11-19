# rishapeslib (46): shapes in memory, the LE shapes format

Reflective Images. Shapes that are already in memory become shapes
(`CludgeShapes`, `CludgeShapesToAddress`, `LECludgeShapes`). There is
also a shapes file format of its own ("LE": `FORM`/`LESH` with `SHHD`,
an optional `CMAP` and `SDAT`, optionally ByteRun1-packed by RIPackLib),
which `LESaveShapes` writes and `LELoadShapes` reads. A file that is not
an LE file goes to shapeslib's `LoadShapes`. The other entries:
`SafeToBlit`, `LEInitShape`, `LEScanShapes` with `LEMaxShapeWidth/Height/Depth`,
`LECompressShapes`, `LEAutoCookie`. init clears the compression flag
and the maxima of `LEScanShapes` and sets `LEAutoCookie` on; finit does
nothing.

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
source's comment "This little bastard won't work below" warns. No
compiled program reaches it: the compiler takes the first form whose
argument count fits, form 1 (`shape#,shape#,filename$`), as the manual
says ("you cannot use the following form of the command"). The libs
test calls its routine directly.

The shape numbers in d0/d1 (`#pd0`, `#pd1`) arrive shifted by 5: the
compiler shifts a `#pd` register by the object size before it adds it
to the `#ia` register. `SafeToBlit`, `LESaveShapes` and `LELoadShapes`
shift them back with `ASR #5`.

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
* shapeslib `LoadShapes` form 1 (`#shapeslib<<7+10`), with the shape
  numbers in d0/d1. This call is inline assembler, because the callee
  takes the first shape in a3 (the register fixed for GCC) and the last
  one in a2. The original leaves them the other way round (a2 = the
  first, a3 = the last); compiled code links the ALibJsr through a stub
  that loads LoadShapes' own `!libs` first (a3, a2 from d0, d1), so this
  does not matter (#813, as editlib #130 in `docs/libs/cmdlibs-port.md`).

Further assembler:

* **dos Write with the flags it leaves.** In `_saveCMAP`,
  `MOVE.l paletteptr,a0` is a MOVEA and sets no flags. The `BNE` behind
  it therefore tests the Z flag that dos `Write` of the file header left.
  `write_z` calls Write and records that flag with `seq`.
* **The stack imbalance.** An allocation that fails in readshapes'
  shape loop or in the cookie returns with a0 still pushed. The original
  returns to that data address; `rs_crash` jumps there.

## BUG(orig), kept

* **LESaveShapes' shape count:** s2 − s1 is a word subtraction, and the
  upper word of d1 goes into the long count. Compiled code leaves one
  there from a long expression (`a.l=$50009 : LESaveShapes 0,a,…`
  writes the count $50009). The loader reads count + 1 shapes
  (`SUB.l #1:BMI`), limited by the last shape (#815).
* **A failing allocation in readshapes** returns with a0 pushed (#806;
  see the stack imbalance above).
* **LELoadShapes:** a failing chunk allocation runs the caller's cleanup
  and returns there. The caller then frees the header buffer and closes
  the file a second time if packedsize is 0, and otherwise uses
  packedsize as the chunk (#819; not a bug in practice: a failing globalloc never returns, memlib ends the program with "Out of Memory!").
* **read_palette:**
  * The allocation is not checked; a failure writes to address 0 (#821; not a bug in practice: a failing globalloc never returns, memlib ends the program with "Out of Memory!").
  * The colour count is `DIVU #3` of the chunk size; on overflow the
    register is unchanged (#820).
  * 0 colours decode 65536 (#820).
  * A CMAP size that is not a multiple of 3 makes LECludgeShapes miss
    SDAT (#811).
* **LESaveShapes:**
  * Whether the CMAP is written depends on Write's Z flag (above). In
    the emulator (OS 3.2) it is written (#814).
  * The FORM length is 8 too large and contains the CMAP size of the
    last save that had a palette (#816).
  * The shape header buffer is allocated with the upper word of the d0
    that `_saveCMAP` leaves, and freed with size 14 instead of 12 (#817).
  * A palette of 0 colours copies 65536 (#823).
  * When the pack buffer is missing, two runtime errors follow (TRAP #0:
    "Not enough memory to pack shapes", "Shape Compression Failed!"),
    then CreateCookie on the shape, whose RTS returns from
    `_writeshapes` (#818).
  * PackPBM's failure runs UnPackPBM with d3 = 12 (RIPackLib's
    BUG(orig), #772).
* **LECludgeShapes:** the SHHD chunk is skipped as 20 bytes, whatever its
  length (#811).

Original behaviour that is no defect:

* CludgeShapes: shape# is divided by 32 and never used.
* SafeToBlit compares d0 >> 5 with the maximum, LESaveShapes counts
  (d1 >> 5) − (d0 >> 5), LELoadShapes hands d0 >> 5 and d1 >> 5 on to
  `LoadShapes`: the shape numbers, as compiled code passes them shifted
  (above; #822, #815 as titled). Blitz programs run against the
  original show it: `SafeToBlit(40,0,0)` with 30 shapes is 0,
  `LESaveShapes 0,9` writes the count 9 and `LELoadShapes 20,29` of it
  loads ten shapes.
* LELoadShapes of a non-LE file: `SaveShapes 0,2` then
  `LELoadShapes 10,12` loads the three shapes into 10–12 (the stub,
  above; #813).
* LELoadShapes form 3 takes the palette of d3, but cannot be compiled
  (Declarations; #812).
* LELoadShapes, a statement, leaves `packedsize` of the last packed
  shape in d0 (a statement has no result).
* LECludgeShapes links memory below 2 MB that is not compressed instead
  of copying it (`"cL"`), unless copymemflag is negative; the form
  without copymemflag links. That is intended.
* LEInitShape: the data and the cookie are cleared chip memory; the
  cookie is not computed (a shape of cleared data has an empty cookie).

## Fixed

Intended deviations from the original (`docs/libs/deviations.md`,
kind F):

* init sets `LEAutoCookie` on and clears the maxima of `LEScanShapes`
  besides the compression flag. The original's init clears only the
  compression flag, so a second run of a program with the library still
  loaded (from the editor) started with the run before's `LEAutoCookie`
  setting (off: `CludgeShapes` made no cookies) and its
  `LEMaxShapeWidth/Height/Depth` (#1109).
* CludgeShapes takes a shape in memory whose size (plane size × depth)
  is 0 as empty (nothing allocated or copied, the next shape 32 bytes
  on), and copies a size above 64 KB whole. The original's copy is a
  DBRA on the low word of the size: a size of 0 copied 65536 bytes into
  a block of none, a larger size only its low word (#808, test
  `rishapeslib-cludgeshapes-size0`).
* CludgeShapes advances by 32 behind an empty shape in memory (width or
  height 0). The original cleared the size, a long, with `MOVE.w #0`
  (its upper word only), so an empty shape advanced by 32 + the low word
  of the size of the shape before it, also of an earlier call, and the
  shapes behind it were read from the wrong place (#809, test
  `rishapeslib-cludgeshapes-empty-stale-size`).
* LELoadShapes, LECludgeShapes and LEInitShape free the cookie of the
  shape they replace (always memlib's memory, also for a shape linked
  into memory). The original's `MOVE.l 18(a2),a1:BEQ` after
  `CLR 30(a2)` tests the Z flag of the CLR (a MOVEA sets no flags), so
  the cookie, one plane of chip memory, was lost on every reload (#810,
  test `rishapeslib-cookie-not-freed`).
* LELoadShapes and LECludgeShapes replace a shape in use that is
  cludged (`"cL"`) like any other. The original's readshapes saves a0
  (the read pointer) around the free of a used shape but skips the push
  for a cludged one, not the pop: the first such shape took readshapes'
  return address as its data pointer and the routine returned to its
  caller's caller (LELoadShapes then neither freed its buffers nor closed
  the file); a second one returned into the Blitz program's stack, a
  crash (#807, test `rishapeslib-cludged-shape-pop`).

## Test

Each run loads one library set (the C build or the original); the
scenario writes its observations as cases, compared with the expected
data `libs/tests/expected/rwe_rishapeslib.txt` recorded from the
original. The set has its own memlib, shapeslib and RIPackLib (original
set: the CD's binaries; C set: this build); the test has 70 shape
objects, palettes, a bitmap object and files in RAM:. It passes the
registers as compiled code does: the shape numbers of `SafeToBlit`,
`LESaveShapes` and `LELoadShapes` shifted by 5 (the compiler shifts a
`#pd` register by the object size before it adds it to the `#ia`
register), and `LELoadShapes` reaches shapeslib's `LoadShapes` through a
stub that loads the callee's registers from its `!libs` first, as the
compiler links every ALibJsr call.

Observed:

* the results;
* the shape objects: every field but the pointers; the data and cookie
  bytes; for linked shapes, the offset of the data in the source buffer;
* the palettes;
* the saved files, byte for byte;
* the kept registers;
* memlib's memory, all back at the end.

Cases of the expected effect (the truth value of a condition, observed
like any other) show that the scenarios do what they say (four shapes
loaded, packed shapes loaded, "cL", LoadShapes into the first shape, the
stale-size skip).

Covered:

* LEInitShape on fresh and on used shapes.
* LEScanShapes and LEMax*.
* SafeToBlit: limits, negative coordinates, no cookie, shapes beyond
  the maximum.
* LESaveShapes: both forms, with a palette, packed (packable data and
  data PackPBM fails on), count 3 and 64, an upper word of d1 in the count.
* LELoadShapes: all four forms; into used shapes; a missing file; FORM
  without LESH; a shapeslib file.
* LECludgeShapes: from fast memory; from chip memory, copied (flag −1),
  linked, and packed; with and without a palette; a CMAP of 7 bytes; no
  FORM.
* CludgeShapes and CludgeShapesToAddress: with and without LEAutoCookie,
  empty shapes, 0 shapes, used and linked targets.
* init and finit; a second run (finit and init again, as from the
  editor with the library still loaded): LEMax* and LEAutoCookie before
  anything else.

Not covered: failing allocations (they jump
into data), failing chunk allocations, a palette of 0 colours, the pack
buffer's runtime error.
