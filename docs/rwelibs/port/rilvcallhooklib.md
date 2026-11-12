# rilvcallhooklib (27): a render hook for GadTools listviews

Red When Excited. One function, `RetrieveCallHook`: the address of a
hook function for `GTLV_CallBack` (`h_Entry`); it keeps graphics.library's
base (`!libs #graphicslib,#la6`) for the hook. init and finit are empty.

The hook (a0 = Hook, a1 = LVDrawMsg, a2 = node) returns `LVCB_UNKNOWN`
for other messages than `LV_DRAW`, else draws the item and returns
`LVCB_OK`. `h_Data` points to the field table:

| Entry | Words |
|---|---|
| first (14 bytes) | offset of the indent word in the node (0: none), offset of the colour table pointer |
| fields (14 bytes each) | width, offset of the text / BitMap pointer, alignment (1 right, else left), isbitmap, 6 bytes unused |
| end | width 0 |

The colour table: apen/bpen of the normal (and normal disabled) state,
then apen/bpen of the others. The item is drawn in `lvdm_Bounds`: the
indent cleared in pen 0 with dotted tree lines in pen 1 every 16 pixels
(the last one short if the next node ends the list or has a smaller
"indent"), then the fields left to right (text cut with `TextFit`, or a
BitMap cut to the field; left or right aligned, centred vertically; the
rest of the field in bpen), the last field cut at the right edge of the
bounds; disabled items (states 2 and 8) are ghosted with the pattern
`$4444/$1111` in apen. The hook keeps its state in static variables (not
reentrant, as the original).

* **Source:** `rwelibsrc/lvcallhook/LVCallHook3.bb` (as TED shows it:
  `build/amiga/rwelibsrc/lvcallhook/LVCallHook3.bb`).
* **Reference:** the reference build of the source by blitz2 2.10
  (`refbuild`, `build/amiga/libs/userlibs/rwe/ref/rilvcallhooklib.obj`):
  the source is newer than the only binary, AmiBlitz3's
  `RILVCallHook.obj` (no indent, no ghosting, 8-byte field entries;
  `docs/rwelibs/README.md` §3.2). Declarations equal (ctest
  `rwe_declarations_rilvcallhooklib`).
* **C file:** `libs/userlibs/rwe/rilvcallhooklib.c`; test
  `libs/tests/rwe/rilvcallhooklib.c` (snitch case "rwe rilvcallhooklib").

## C and assembler

All C behind `BLITZ_THUNK`s. The hook's thunk keeps d1/a0/a1 and a3
(the original keeps a2–a6/d2–d7; `TextFit` takes its constraining extent
in a3, which the C code, built with a3 fixed, does not save itself). The
hook takes the caller's d7 as a parameter: its upper word decides on the
tree lines (second bug below). The graphics calls get the registers the
original passes where their upper words can matter (`TextFit`'s width
with `$FFFF` above, its length as a long). The original's unused load of
`lvdm_DrawInfo->dri_Pens` is left out.

## BUG(orig), kept

* The test for the short tree line of the last indent level reads the
  next node's indent at the offset of the first field entry's *width*
  (`dispaddress` is already past the first entry), not at the indent
  offset; an odd width is an address error on a 68000 (#739).
* The number of tree lines, indent / 16 − 1, is computed as a long with
  the upper word of the caller's d7 (`MOVE.w indent,d7:ASR.w #4:SUBQ.l
  #1:BMI`): with a negative d7 no tree lines are drawn and the RastPort's
  line pattern stays `$AAAA`; with an indent below 16 and an upper word
  of 1..`$8000` 65536 lines are drawn (`DBRA` on the low word) (#737).
* `TextFit` gets the constraining width with `$FFFF` in the upper word
  (`MOVEQ #-3,d2` and word arithmetic) (#741).
* A right-aligned BitMap is drawn `ob_MinX` too far right: the alignment
  adds the offset a text extent needs, but the BitMap's extent already
  starts at `ob_MinX` (#740).
* The tree line pattern is written to `rp_LinePtrn` without the flags and
  count `SetDrPt` sets (#738).
* A text pointer 0 in a text field is read as a string at address 0
  (#742).

The hook needs `RetrieveCallHook` to have been called (the graphics base
is 0 before); that is the documented usage, not a defect.

## Test

Each run loads one library set (the C build or the original); the
scenario writes its observations as cases, compared with the expected
data `libs/tests/expected/rwe_rilvcallhooklib.txt` recorded from the
original. `RetrieveCallHook` with a6 = graphics.library gives the hook;
it is called like GadTools does with a RastPort on a BitMap (2 planes,
1280 × 48, cleared to a pattern before each call, topaz 8). Observed
(after `WaitBlit`: the hook returns while its last blit may
still run): d0, both planes, the RastPort without the BitMap pointer,
kept registers. 150 random items: bounds, states 0/1/2/8/4, 1–4 fields of
texts and BitMaps (left/right, BitMaps wider/higher than the field
cut, a BitMap pointer 0), the last field cut, indents of 1–60 pixels or
none, the next node last or not and its "indent" word random, the
caller's d7 with upper words 0, `$FFFF`, `$8001`, `$0001`, `$8000`.
A message other than `LV_DRAW` (nothing drawn), init/finit. Layouts that
would give graphics.library an empty rectangle (a RastPort without a
layer is not clipped) are skipped: items of 10 lines and more, BitMaps
lower than the item, no text or BitMap ending exactly at the field's
edge. Checked to fail when the width-offset bug or the d7 bug is "fixed".
Not covered: the 65536 tree lines, a text pointer 0, negative indents.
