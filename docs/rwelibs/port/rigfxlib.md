# rigfxlib (userlib 65): palette and bitmap helpers

`libs/userlibs/rwe/rigfxlib.c`, the port of `rwelibsrc/GFXLib/GFX-Library2.bb`
(21/01/96) to C. Reference: `BSS2/REDBLITZLIBS/LIBRARIES/USERLIBS/RIGFXLIB.OBJ`,
which the source builds to with blitz2 2.10 ([README](../README.md) 3.1).
The declarations equal the reference (ctest `rwe_declarations_rigfxlib`);
the code is C, not byte-identical.

## What it does

Palette objects of palettelib (`!libs #palettelib,#ia0|#pd0`: a0 = the
object, +0.l the data, +4.w the number of colours; the data: numcols.w,
a word, then 12 bytes per colour, r.l g.l b.l with the value in the top
byte) and bitmap objects of bitmaplib (a struct BitMap, +62.w non-zero
when the planes belong to the object):

* `PaletteInfo palette#` chooses the palette for `PalRed`/`PalGreen`/
  `PalBlue(c)` (4 bit) and `AGAPalRed`/`AGAPalGreen`/`AGAPalBlue(c)`
  (8 bit), and is the source of `PalAdjust`.
* `PalAdjust palette#,ratio.q[,start,end]` – palette# = PaletteInfo's
  palette × ratio (8.8 fixed point from the Quick, saturated at 255).
* `FillPalette`/`AGAFillPalette palette#,r,g,b[,start,end]`.
* `CopyColour(srcpal#,destpal#,srccol#,destcol#)` – -1 or 0.
* `CPUCls bitmap#` – clears all planes with the CPU (blocks of 40 bytes).
* `SaveCMAP(palette#,filename)` – writes an ILBM with BMHD and CMAP
  (Amiga mode only); the file name is a `#long` (an address).
* `MergeBitplanes src1,src2,dest,planes` – dest gets the plane pointers
  of src1 and then of src2 (the sources need word 62 non-zero, dest zero;
  dest's word 62 is set to 1). Nothing happens if that or the number of
  planes does not fit.

No init or finit (`!libheader {#rigfxlib,0,0,0,0}`); the original's
`_libfinit` (an RTS without a reference) is not ported.

## C and assembler

All routines are C behind thunks that keep d1/a0/a1 (statements also
d0) – at least what the original keeps; the original's CPUCls leaves
a1–a3 zero (the source registers of its MOVEM blocks), the C version
keeps them. The register arithmetic of the original is reproduced where
it decides the result: `MULU.w #12` with the low word of the product as
a signed index (`0(a0,d0.w)`, `ADDA.w`) for the colour functions and
PalAdjust, `ADD.l` of the unsigned product for FillPalette and
CopyColour, DBRA counts on words, `TST.l` of the whole `end` register in
FillPalette but `CMP.w` against start, `DIVU #40` with its overflow case
in CPUCls (`bl_divu_w`).

Assembler remains only in the entry of `CPUCls`: the `TST.l 8(a0):BEQ
'oops` of the original jumps to `MOVEM.l (a7)+,a4-a6:RTS` before the
matching `MOVEM.l a4-a6,-(a7)` has run (BUG(orig) below) – a stack
imbalance C cannot express. The clearing itself is C.

## Calls into other libraries

* `SaveCMAP`: memlib `globalloc`/`globfree` (`TOK_globalloc`/`TOK_globfree`,
  d0 = size, d1 = MEMF_CLEAR / a1 = memory) and **RIAmosFuncLib** (75)
  `BSave` form 1 (d0 = name, d1 = start, d2 = length → d0), token
  `(LIB_riamosfunclib << 7) + 16` (`$A590`, index 15 in the token list of
  the CD's `RIAMOSFUNCLIB.OBJ`), called with `BL_JSR_D4_` (blitzlib.h) and
  `BL_TOKVAL(…, 1)` because BLITZ_ALIBJSR passes no d2. SaveCMAP returns
  BSave's result.

## BUG(orig), kept

* `CPUCls` on a bitmap without plane 0 (`8(a0)` = 0) pops the return
  address and two more longs into a4–a6 and returns to whatever follows
  on the stack – a crash.
* `CPUCls` clears with MOVEM blocks of 40 bytes, `DBRA` on blocks − 1: a
  bitplane of less than 40 bytes gives −1, i.e. 65536 blocks – 2.6 MB
  below the plane are cleared. A rest of an odd number of bytes leaves
  the first byte of the plane. The depth is the word at +4 (Flags<<8 |
  Depth); depth 0 clears 65536 planes.
* `SaveCMAP` writes no pad byte behind a CMAP of odd length (odd number
  of colours); the BMHD chunk is 20 zero bytes; a palette of 0 colours
  copies 65536 colours into the 48 byte block.
* `CopyColour` checks the colour numbers signed (`BGE`): negative colour
  numbers pass and index up to 786 KB behind the palette.
* `PalAdjust` and the colour functions use the palette of the last
  `PaletteInfo` without a check (before it: address 0); `PalAdjust`
  without start/end takes the number of colours of that palette, not of
  the target; colour numbers from 2731 on index before the palette (12·c
  as a signed word); `PalAdjust` with end = start − 1 runs 65536·3 channels.
* `FillPalette … ,start,0` fills up to the last colour (end 0 = all), but
  an `end` with a zero low word and a non-zero upper word is end 0 itself.

The source's history says "Fixed SaveCMAP … size was set to
(NUMCOLS*3)-48"; the reference has the fixed version.

## Test

`libs/tests/rwe/rigfxlib.c` (snitch "rwe rigfxlib"): the C build and the
CD original with memlib, side by side; word arguments carry markers in
their upper word. Palettes and bitmaps lie in one buffer of 72000 bytes
per set, filled the same, so reads and writes outside the palettes are
compared too (the whole buffer after every call), as are d0 and the
registers the original keeps. RIAmosFuncLib is not loaded (not ported,
not in the test directory): a stub (`sysload_stubs`) takes the place of
`BSave` in both sets, records name, length and file data and returns a
chosen value. Covered: all colour functions (in range, beyond the
palette, the negative word index, before PaletteInfo), PalAdjust both
forms with nine ratios (saturation, negative, fractions) and ranges past
the palette and before it, both fill commands with all forms of start/end
(the `TST.l` case, end < start), CopyColour in and out of range, CPUCls
with whole blocks, a word rest, an odd rest and odd plane addresses,
SaveCMAP with odd/even colour counts, BSave failing, no data, memlib's
memory all back, MergeBitplanes with all counts (0 = all, too many,
negative), the flag checks and a negative word sum.

Not covered: the crash and the 65536-block/-plane cases of CPUCls, the
DIVU overflow (planes of 2.6 MB), negative colours in CopyColour,
PalAdjust with end < start, SaveCMAP with 0 colours (they write far
outside any buffer); the real BSave.
