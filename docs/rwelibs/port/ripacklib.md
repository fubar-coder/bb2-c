# ripacklib (userlib 73): ByteRun1, ILBM and Pack-Ice in memory

`libs/userlibs/rwe/ripacklib.c`, the port of `rwelibsrc/packlib/Pack-Library8.bb`
("Reflective Images PACK-library", v1.2 of 21/3/1995) to C. Reference:
`BUM10A/BLITZLIBS/RILIBS/RIPACKLIB.OBJ`, which the source builds to with
blitz2 2.10 ([README](../README.md) 3.1; `Pack-Library9` is newer than any
binary, `8_Mc` equals OL's other build). The declarations equal the
reference (ctest `rwe_declarations_ripacklib`, including the `error2 = 1`
of both LoadIFF forms); the code is C, not byte-identical.

## What it does

* `PackPBM(source,dest,length)` – ByteRun1 (runs and literal stretches of
  at most 128 bytes); the packed length, or -1 if it would not be
  smaller than `length` ("never writes a file bigger than the source").
* `UnPackPBM source,dest,length,destlength` – ByteRun1 back (`-128` is
  skipped).
* `DeIce(source,dest)` – the Pack-Ice 2.40 decruncher ("ICE!", packed and
  unpacked length, a bit stream read from the end; literal runs, strings
  with 2–1033 bytes and three offset classes, the Atari picture mode that
  converts bitplanes); -1, or 0 if there is no "ICE!".
* `UnpackIFF(source,bitmap#[,numlines,xy_offset])` – finds BMHD (and from
  there BODY) by searching the memory in steps of 2 bytes and unpacks the
  BODY into the planes of the bitmap object (`#ia1|#pd1`), from
  `xy_offset` bytes into each plane, `numlines` lines (< 0: all); 0 if a
  chunk is missing, the bitmap is too small or not deep enough, or plane
  0 is missing.
* `ILBMPalette(source,palette#[,offset])` – the CMAP into a palette
  object (allocating its colour block with memlib when it has none).
* `ILBMGrab source,bitmap#,palette#` – UnpackIFF and ILBMPalette.
* `LoadIFF(filename$,bitmap[,palette])` – the file into a free bank of
  RIAmosFuncLib (from bank 0 on), UnpackIFF and ILBMPalette from there,
  the bank erased.
* `ChunkHeader(a$)` – the first long of a string (a chunk id).

init and finit are empty (RTS).

## C and assembler

Everything is C, no assembler. The originals of the packers and
unpackers save all registers but d0 (`MOVEM.l d1-d7/a0-a6` /
`d0-d7/a0-a6`); the thunks keep d1/a0/a1, GCC the rest. The register
arithmetic that decides the results is reproduced: byte counters and
DBRA on words (ByteRun1, the row counter of UnpackIFF as a signed byte),
signed address comparisons (`CMP.l`/`BLT`/`BGT`), `DIVU #3` (with its
overflow case, `bl_divu_w`) and `MULU.w` of the palette code, the
sign-extended word offsets of the ICE strings, the X flag of the ICE bit
buffer (a marker bit shifted in with ADDX), the order and size of the
memory accesses (ICE's header is read byte by byte, so it may be at an
odd address).

## Calls into other libraries

* `ILBMPalette`: memlib `globalloc`/`globfree` (`TOK_globalloc`,
  `TOK_globfree`; MEMF_PUBLIC|MEMF_CLEAR).
* `LoadIFF`: **RIAmosFuncLib** (75), tokens `(LIB_riamosfunclib << 7) +
  index+1` with the index in the token list of the CD's
  `BSS2 RIAMOSFUNCLIB.OBJ`: `NextBank` (`$A596`, d0 = 0 → bank or
  -1; the original calls form 0 with d0 = 51, which in the original
  RIAmosFuncLib starts at d0; the C version calls form 1, the form with
  the start bank, with 0, #770; form 0 of the C RIAmosFuncLib searches
  from bank 0, #497), `BLoad` form 1 (`$A58F`, d0 = name, d1 = bank → d0; called with
  `BL_JSR_D4_`/`BL_TOKVAL(…, 1)`), `Start` (`$A591`, d0 = bank → address),
  `Erase` (`$A593`, d0 = bank). The original also loads d2 = 0 and
  d4 = -1 for BLoad, which form 1 does not read; the C version passes
  d2 = d3 = 0.

## Fixed

Intended deviations from the original (`docs/libs/deviations.md`,
kind F):

* **PackPBM** returns -1 when the packed data would not be smaller than
  the source; dest holds the packed bytes written up to that point. The
  original had no RTS there: it fell into `UnPackPBM` with the registers
  of the caller and d0 = -1, unpacked from address -1 (then 0, 1, …)
  into dest, as far as the caller's d3 + dest allowed, and returned -1
  (#772).
* **UnPackPBM** tests both limits before each access (unsigned) and stops
  a literal run at the end of the source: nothing at or behind
  source+length is read, nothing at or behind dest+destlength written.
  The original tested after the access and signed: it read the byte at
  source+length, wrote one byte past destlength and read a literal run
  past the source's end (#778).
* **LoadIFF** takes the first free bank of RIAmosFuncLib from bank 0 on,
  as RWE's own V2.5 of the library does (`MOVEQ #0,D0 : ALibJsr
  NextBank,0`). The original asked for one from bank 51 on; with
  RIAmosFuncLib's default of 50 banks there is none, and LoadIFF returned
  0 unless the program had raised the number with `BankLimit` (#770).
* **LoadIFF** returns the result of UnpackIFF, and 0 when it was asked
  for the palette and ILBMPalette fails (no CMAP); the bank is erased in
  every case. The original returned -1 whenever the file loaded (#771).
* **UnpackIFF** refuses a BMHD with nPlanes 0 (0, nothing written). The
  original checked nPlanes only against the bitmap's depth, which 0
  passes; `SUBQ.b #1` made the depth 255, and 256 plane pointers were
  stored into its table of 8, over its variables and the code behind it
  (#777).
* **UnpackIFF** takes the bytes of a row as ILBM stores them, padded to
  whole words (`((width + 15) / 16) * 2`), and tests that width against
  the bitmap's bytes per row. The original took width/8 (`ASR.w #3`):
  for every picture whose width is no multiple of 16 the BODY was read
  out of step from the second row on (#775).
* **UnpackIFF** counts the bytes of a row in a word. The original counted
  in a signed byte (`TST.b d5`): rows of 129 to 256 bytes (pictures
  1032 to 2047 pixels wide) were "done" before anything was read, and
  longer rows were read out of step (#774).

## BUG(orig), kept

* `UnpackIFF` treats every BODY as ByteRun1 (the compression byte of the
  BMHD is not read) (#773), does not stop a run at the end of
  a row (it runs into the next row or behind the plane) (#776).
* The chunk searches step by 2 bytes from the given address: BMHD within
  5120 bytes, BODY within 5120 bytes from the BMHD on (a word counter),
  CMAP within 5122 bytes (#766).
* `ILBMPalette` without offset always frees the palette's colour block
  (if it has one) and allocates a new one of the CMAP's size; with an
  offset into a palette that is too small it does the same and ignores
  the offset (#769). The size passed for the freed block comes from the
  object's colour count; if the new allocation fails, the object keeps
  the freed block (#768).
* `ILBMPalette` with an offset on a palette that has no colour block yet
  allocates the block for the CMAP's colours (12 bytes each + 16) and still
  adds offset * 12 to the destination (only the killmem path resets the
  offset): the last `offset` colours lie behind the block, from offset 2 on
  outside it (an offset of 4 with a 4-colour CMAP: 36 bytes behind the 64
  byte block). Test `ripacklib-ilbmpalette-offset-empty-palette` (inject
  guard). (#991)
* `DeIce`: the picture flag of Pack-Ice loads the default count `$0f9f`
  into d7, the register of the bit buffer; the next bit (the "own count"
  flag) and the first 6 bits of the count then come from `$9f`: the
  default is never used, and the count is at least `$3C00` – 15361 or
  more groups of 8 bytes are converted, ending at the end of the data
  (123 KB and more below it for short data) (#767).

## Test

`libs/tests/rwe/ripacklib.c` (snitch "rwe ripacklib"): each run loads
one library set (the C build or the CD original) with memlib; the
scenario writes its observations as cases (d0, output buffers, palette
and bitmap objects, the kept registers), compared with the expected data
`libs/tests/expected/rwe_ripacklib.txt` recorded from the original. The data is
built in the test: plain data with noise, runs and repeats; ByteRun1 by
PackPBM and by a small packer of the test; ILBM files with BMHD, CMAP
and BODY; Pack-Ice data from an encoder written against the decruncher
(the output must equal the plain data – a check of the encoder). Covered: PackPBM on 0…4000 bytes and runs around 127/128,
round trips through UnPackPBM, the failure path (with d3 0, 4, -8),
UnPackPBM with no-ops and short source/destination limits; DeIce with
literal runs of every length class, strings of every length and offset
class including repeats, data at an odd address, no "ICE!", the
picture mode (on 132 KB below the data); UnpackIFF both forms with all
line counts (all, fewer, more than the bitmap, 0), xy offset, a too
shallow or narrow bitmap, no plane 0, rows of 6, 128 and 130 bytes (the
BODY rows word-aligned, as ILBM stores them), a run past the row, no BMHD, no BODY; ILBMPalette into an empty palette, with
offsets that fit or not, without offset, a CMAP of 10 bytes, no CMAP;
ILBMGrab; LoadIFF both forms with a good file, a file that does not load,
no free bank, a file that is no ILBM; ChunkHeader; memlib's memory all
back.

RIAmosFuncLib is loaded too ([riamosfunclib.md](riamosfunclib.md)): its C
build with the C set, the AmiBlitz3 binary (the port's reference; its
token list is the one of the BSS2 binary the original was built against)
with the original, initialised after memlib, the ALibJsr targets
fixed up with `amosfunc_fixjsrs` (`libs/tests/rwe/amosfunc.h`: BLoad is
Amiga mode only). LoadIFF reads an ILBM of the test from `RAM:`; besides
bitmap and palette bank 51 afterwards is observed (free, NextBank(51)).
The first LoadIFF runs with RIAmosFuncLib's default of 50 banks (the
original finds no bank from 51 on and returns 0, fixed #770); then the
test raises the limit to 60.

Not covered: corrupt ICE data, PackPBM's failure with a large d3,
nPlanes 0, DIVU overflows (CMAP chunks of 196 KB), a failing memlib
allocation.
