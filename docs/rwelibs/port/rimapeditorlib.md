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
  word width × 2 and the height, the 65535-byte fix of the source; a
  width above 32767 is refused with runtime error checking, #1152) and
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
  65536 words per row, a height of 0 65536 rows; refused with runtime
  error checking, #895).
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
  but for the error checks the port adds
  (`rimapeditorlib.declarations-intended.txt`; ctest
  `rwe_declarations_rimapeditorlib`).
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
pointer behind it are one struct, in the original's order.
`ShapeTo1616` hides its source pointer from GCC's
loop optimisation (no `__mulsi3`).

## Fixed

* init clears the stencil of `SetStencil`, the results of `ScanShapes`
  and the parse position of `M_InitParse`, as at the first load; the
  original kept them, so a second run of a program from the editor (the
  library stays loaded) had the stencil of the run before (a freed
  array) before its `SetStencil`, the maxima of the run before before
  its `ScanShapes`, and from `M_NextArg$` before `M_InitParse` the rest
  of the run before's string instead of "" (#1133; the libs test's
  second run).
* `MapCanDraw` compares x/y with the width and height of the map (and of
  the rub-through map) unsigned; the original compared them signed
  (`BGE`), so negative values passed, the map's word was read far behind
  the data and its address kept for `MapCanDrawAddr` (#892; test
  `rimapeditorlib-candraw-negative-xy`).
* `CreateMap` frees the old map through the object's free routine, which
  tests the data pointer and the name itself; the original tested the
  long at 12 (the low word of the data pointer and viewx), so a map whose
  data lay at `$xxxx0000` with viewx 0 was not freed (lost) and the
  fields 24/28 were not cleared either (#894; test
  `rimapeditorlib-createmap-tst-offset12`).
* `MapDataGrab` of bytes zero-extends each byte; the original loaded it
  with `MOVE.b` into the register of the previous sum, whose upper byte
  stayed: a byte + mapnull beyond 255 carried into the following values
  (#898; test `rimapeditorlib-mapdatagrab-byte-carry`, the libs test's
  bytes 250–255 with mapnull 10).
* `MapObjName$` counts the length as a long; the original counted it in
  a byte (`ADDQ.b`), so a name of 256 characters or more came back with
  its length modulo 256 (#899; test
  `rimapeditorlib-mapobjname-byte-length`, the libs test's name of 300
  characters).
* `M_InitParse` copies at most 255 characters and a 0 into its 256-byte
  buffer (a longer string is cut); the original copied without a limit,
  so a string of 256 characters or more overwrote the pointer behind
  the buffer, and from 260 characters the code of `M_NextArg$` behind it
  (#897; test `rimapeditorlib-initparse-overflow`).
* `MapDataGrab` fills nothing into a map without data (its `CreateMap`
  could not allocate them, also for a width or height of 0; D-018); the
  original's `DBRA` loops took the width and height 0 of such a map as
  65536 each and wrote from address 0 over all memory (#895, the
  `MapDataGrab` part; test `rimapeditorlib-mapdatagrab-no-data`).

### Error checks

The original has no error check area; the port adds one. The release
code of the checked commands is unchanged (D-005: invalid arguments
and commands before their setup command).

* `MapCanDraw` refuses a call before any `SetStencil` ("MapCanDraw
  needs SetStencil"; the original reads the stencil at address 0) and,
  where it looks up the stencil (a position inside the map and the
  rub-through map whose word is not the value it compares with), a map
  word of `$8000` or more ("Map value above 32767 for the stencil"; the
  original uses it as a signed index, up to 32 KB before the stencil)
  (#893; test `rimapeditorlib-candraw-no-stencil`).
* `MapCanDraw` and `MapDataGrab` refuse a used map without data – after
  `Free MapObject`, or a `CreateMap` that could not allocate it
  ("MapObject has no map data"; under #893, test
  `rimapeditorlib-candraw-no-map`; `MapDataGrab`'s release code fills
  nothing into such a map since #895, so it has no bug-orig test).
* `MapDataAdr(x,y)` refuses a position outside the map ("Map position
  outside the map"; x and y compared unsigned with the width and the
  height, as `MapCanDraw` does; the inline routine has no range check:
  x at or beyond the width gives a word of a later row, y at or beyond
  the height an address behind the map) (under #892; test
  `rimapeditorlib-mapdataadr-outside`).
* `FillBitplane` refuses a width below 16 ("FillBitplane width must be
  at least 16"; the release code writes 65536 words per row) and a
  height of 0 ("FillBitplane height must be at least 1"; 65536 rows)
  (#895; test `rimapeditorlib-dbra-size0`).
* `ShapeTo1616` refuses a source shape of height or depth 0
  ("ShapeTo1616 source of height or depth 0"; the release code copies
  65536 rows or planes into the destination; an invalid shape object,
  Blitz's own commands make none) (#895).
* `ShapeTo1616` refuses a destination that is not 16 × 16 with at least
  the source's depth ("ShapeTo1616 needs a 16 x 16 destination as deep
  as the source"; the release code writes 16 words per plane of the
  source, one per row, into the destination's data whatever its size:
  fewer planes or rows overflow it, a wider one gets the words packed as
  if it were 16 wide) (#900; test
  `rimapeditorlib-shapeto1616-dest-unchecked`).
* `CreateMap` refuses a width above 32767 ("Map width above 32767"; the
  release code sizes the data as (width × 2 as a word) × height
  (`ASL.w`, `MULU.w`), so the upper bit is lost (40000: 14464 bytes per
  row instead of 80000); `MapDataSize`, `MapDataAdr(x,y)` and the free
  routine compute the same word, but `MapDataGrab` fills width × height
  words and `MapCanDraw` reads at x × 2 as a long, up to 64 KB per row
  behind the data) (#1152; test `rimapeditorlib-createmap-width-word`).

## Kept on purpose (not fixed, decision)

* BUG(orig) (#896): finit closes graphics.library also if it did not open.

Original behaviour that is not a defect:

* `CreateMap` of a map whose data cannot be allocated (also width or
  height 0) clears width, height, bwidth and bheight but leaves datasize
  1: nothing reads it then.
* The "strip leading spaces" of `M_NextArg$` is only a comment of the
  source, not something the command promises.

## Test

Each run loads one library set (the C build or the original); the
scenario writes its observations as cases, compared with the expected
data `libs/tests/expected/rwe_rimapeditorlib.txt` recorded from the
original. The set has its memlib/maxslib (names), MapObjects, Blitz
shapes, bitmap object, RastPort/BitMap (chip memory, topaz 8), window
object and globals (a5). Observed: the MapObjects (all fields; data and name by
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
