# rimapeditorlib (8): MapEd's helpers, MapObject

Reflective Images, 1994–97: the library of RWE's block map editor
MapEd. MapObject objects (block maps of words), helpers for shapes and
bitmaps, text output into a RastPort, and a parser of comma separated
arguments.

A MapObject (32 bytes, 5 by default): width.w (0), height.w (2),
bwidth.w (4), bheight.w (6), datasize.b (8, always 1 = words), mapnull.b
(9), data.l (10, width × height words), viewx.w (14), viewy.w (16),
name.l (18, a maxslib string), changed.w (22), the last address of
`MapCanDraw` (24), the rub-through map (28).

* `CreateMap map#,width,height,bwidth,bheight,name$` – frees the old
  map, allocates the data (`AllocMem`, cleared; the size `MULU` of the
  word width × 2 and the height, the 65535-byte fix of the source) and
  copies the name; the map becomes the used one.
* `MapDataAdr` / `MapDataAdr(x,y)`, `MapDataSize`, `MapObjWidth`,
  `MapObjheight`, `MapObjViewX`, `MapObjViewY`, `MapDataChanged` – the
  fields (inline); `MapDataChanged(newstate)` sets the state and returns
  the old one.
* `MapDataGrab address,datasize,mapnull` – fills the data from memory:
  bytes (datasize 0) or words, each + mapnull.
* `MapObjName$`, `SetMapObjName$ name$`.
* `RubThroughMap(map#)` sets the rub-through map of the used map,
  `RubThroughMap` returns it, `ClearRubThroughMap` clears it;
  `SetStencil array` sets a byte array.
* `MapCanDraw(shape#,x,y)` – −1 if x/y lie outside the map (or the
  rub-through map), if the map's word there equals shape# + 1 (with a
  rub-through map: that map's word there), or if the stencil byte of the
  map's word is set; else that value. `MapCanDrawAddr` returns the
  address of the map's word.
* `ShapeTo1616 src,dest` – the leftmost word of every row of src into
  dest's data, 16 rows per plane (missing rows 0, further rows skipped).
* `ShapeMaskFill shape#` – sets the first 32 bytes of the cookie.
* `ScanShapes(start,highest)` – from shape start up to highest or the
  first empty shape, the largest width, height and depth (signed;
  `ScanMaxWidth`, `ScanMaxheight`, `ScanMaxdepth`); returns the last
  number.
* `FillBitplane bitmap#,width,height` – a mesh in plane 0 (width/16
  words of $5555/$AAAA per row, alternating; a width below 16 gives
  65536 words per row, #895).
* `MapOutText`, `MapOutTextCenter`, `MapOutTextRight x,y,rastport,string`
  – `Move` to x (centred: − length/2, right: − (length − 1)), y + the
  font's baseline, then `Text`; `MapOutTextPen rastport,pen` – `SetAPen`.
* `SetWindowDraw window#,mode` – as rishapezlib's `SetWinDrawShapeZ`.
* `M_InitParse string`, `M_NextArg$` – the arguments separated by ",";
  after the last one (and before `M_InitParse`) "".

init opens graphics.library, finit closes it; the object's free routine
frees data and name and clears the object.

* **Source:** `rwelibsrc/MapEd/MapEd_Bit2.bb` (as TED shows it:
  `build/amiga/rwelibsrc/MapEd/MapEd_Bit2.bb`); `MapEd_Bit.bb` is older
  and does not compile (`docs/rwelibs/README.md` §3.1).
* **Reference:** the reference build of the source by blitz2 2.10
  (`refbuild`, `build/amiga/libs/userlibs/rwe/ref/rimapeditorlib.obj`); no
  binary exists (`docs/rwelibs/README.md` §3.3). Declarations equal
  (ctest `rwe_declarations_rimapeditorlib`).
* **C file:** `libs/userlibs/rwe/rimapeditorlib.c`; test
  `libs/tests/rwe/rimapeditorlib.c` (snitch case "rwe rimapeditorlib").

## Declarations

`MapDataAdr`'s first form has `!libs {8,#ua3}` (the literal library
number); `CreateMap` `{#rimapeditorlib,#ia3|#pd0,#rimapeditorlib,$0604}`
(d6 = the offset of the used map in the globals). The C file writes the
same words.

## C and assembler

C behind thunks (d0–d1/a0–a1 kept by the statements, d1/a0–a1 by the
functions; the original keeps less). The inline routines (`Dc $a000`:
`MapDataAdr` both forms, `MapDataSize`, `MapObjWidth/height`,
`MapObjViewX/Y`, `MapDataChanged` without argument, `RubThroughMap`
both forms, `ClearRubThroughMap`, `MapCanDrawAddr`) stay assembler with
the original instructions. a3 is left as the original leaves it:
`CreateMap` behind the fields written (24, or 10 on failure), the string
functions behind the characters. a2 stays unchanged (DIFF, the general
rule of `docs/libs/cmdlibs-port.md`: `CreateMap` and `SetMapObjName$`
pop the string length, `FillBitplane` uses a2 as its pointer). The
graphics calls get the original's register values with their upper
words (x of `Move` as the caller's long, `SetAPen`'s pen with the
rastport's upper word). The word results keep d0's upper word where the
original does (`MOVE.w`, `ADDQ.w`). `M_InitParse`'s buffer and the
pointer behind it are one struct, so that an overflow hits the pointer
as in the original. `ShapeTo1616` hides its source pointer from GCC's
loop optimisation (no `__mulsi3`).

## BUG(orig), kept

* `CreateMap` tests the long at 12 (the low word of the data pointer and
  viewx) instead of the data pointer to decide whether to free the old
  map: a map whose data lies at `$xxxx0000` with viewx 0 is not freed
  (lost); the fields 24/28 are not cleared then either (#894).
* `MapCanDraw` compares x/y signed: negative values pass and index far
  behind the data (#892); the stencil index is the map's word as a signed
  word; without `SetStencil` the stencil is at address 0 (#893).
* `MapDataGrab` of bytes keeps the upper byte of the previous value in
  the register (`MOVE.b`): a byte + mapnull beyond 255 carries into the
  following values (#898).
* The `DBRA` loops (`MapDataGrab`, `ShapeTo1616`, `FillBitplane`) run
  65536 times for a size of 0 (`FillBitplane` also for a width below 16)
  (#895); `ShapeTo1616` writes 16 words per
  plane regardless of the destination's size (#900).
* `M_InitParse` copies without limit into its 256-byte buffer (a longer
  string overwrites the pointer behind it) (#897).
* `MapObjName$` counts the length in a byte (`ADDQ.b`) (#899).
* finit closes graphics.library also if it did not open (#896).

Original behaviour that is not a defect:

* `CreateMap` of a map whose data cannot be allocated (also width or
  height 0) clears width, height, bwidth and bheight but leaves datasize
  1: nothing reads it then.
* The "strip leading spaces" of `M_NextArg$` is only a comment of the
  source, not something the command promises.

## Test

Each set has its own memlib/maxslib (names), MapObjects, Blitz shapes,
bitmap object, RastPort/BitMap (chip memory, topaz 8), window object and
globals (a5). Compared: the MapObjects (all fields; data and name by
content, the addresses at 24/28 relative), the used map, a3, the
strings, d0, the shapes' data, the bitmap and the RastPort after the
text commands, the kept registers; all memory back at the end. Covered:
every entry, the inline routines checked inline with the same length;
`CreateMap` new, again on the same object, with an empty name and with
`AllocMem` failing (replaced by `moveq #0,d0:rts` – a really failing
`AllocMem` runs the low memory handlers, which free system memory at
their own pace and made the memory check flaky); `MapDataGrab` of bytes
(with the carry) and words; 131 `MapCanDraw` calls inside and outside,
with and without a (smaller) rub-through map, stencil hits; `ScanShapes`
to highest, to an empty shape, start > highest, negative sizes;
`ShapeTo1616` of heights < 16, 16, > 16 and depths 1–4; `ShapeMaskFill`;
`FillBitplane`; the names (none, 300 characters: the byte length); the
text commands with empty strings; `SetWindowDraw`; `M_NextArg$` before
`M_InitParse` and over strings with empty arguments; the free routine;
init, finit. Checked to fail with a wrong stencil index in the C build.
Not covered: negative coordinates for `MapCanDraw`, sizes 0 of the
`DBRA` loops, `M_InitParse` of more than 255 characters, a map whose
data lies at `$xxxx0000`.
