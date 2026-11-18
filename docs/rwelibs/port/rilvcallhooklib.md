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

All C behind `BLITZ_THUNK`s. The hook's thunk keeps d1/a0/a1, the C
code d2–d7/a2–a6 (the original keeps a2–a6/d2–d7; `TextFit` takes its
constraining extent in a3, which the libraries' safe form of the NDK
macro keeps, `cmake/ndk-calls.cmake`). The NDK's inline passes
`TextFit`'s length, width and height as words, zero-extended (the
original: the length a long with upper word 0, the width with `$FFFF`
above: a silent correction, below). The
original's unused load of `lvdm_DrawInfo->dri_Pens` is left out.

## Fixed

Intended deviations from the original (`docs/libs/deviations.md`,
kind F):

* The number of tree lines, indent / 16 − 1, is a word. The original
  computed it as a long with the upper word of the caller's d7
  (`MOVE.w indent,d7:ASR.w #4:SUBQ.l #1:BMI`): with a negative d7 no
  tree lines were drawn and the RastPort's line pattern stayed `$AAAA`;
  with an indent below 16 and an upper word of 1..`$8000` 65536 lines
  were drawn (`DBRA` on the low word) (#737; test
  `rilvcallhooklib-d7-upper-word`, `libs_emulator` cases `[fixed #737]`
  of `rwe_rilvcallhooklib`).
* The test for the short tree line of the last indent level reads the
  next node's indent at the indent offset. The original read it at the
  offset of the first field entry's *width* (`dispaddress` is already
  past the first entry): the line was short or long by an unrelated word
  of the next node, and an odd width was an address error on a 68000
  (#739; test `rilvcallhooklib-next-indent-offset`, `libs_emulator` cases
  `[fixed #739]` of `rwe_rilvcallhooklib`).
* A right-aligned BitMap ends one pixel before the field's right edge,
  as a right-aligned text does. The original drew it `ob_MinX` pixels
  further right: the alignment adds the offset a text extent needs, but
  the BitMap's extent already starts at `ob_MinX`; the field was filled
  in bpen instead, and with a BitMap pointer 0 that fill ran `ob_MinX`
  − 1 pixels past the field (#740; test `rilvcallhooklib-right-bitmap`,
  `libs_emulator` cases `[fixed #740]` of `rwe_rilvcallhooklib`). A
  BitMap as wide as the field (cut to it) starts one pixel left of the
  field, the mirror of the left alignment's 2 pixels past the right edge
  (both as in the original's placement of texts).
* A text pointer 0 in a text field is an empty text (the field is
  filled in bpen), as a BitMap pointer 0 draws no BitMap. The original
  read the string at address 0 (#742; test `rilvcallhooklib-text-null`;
  the `libs_emulator` scenario has no text pointer 0, it would read
  address 0 in the original's run).

## Silent correction (kind K)

* `TextFit` gets the constraining width as a word (upper word 0): the
  NDK's inline casts the argument to `UWORD`. The original's
  `MOVEQ #-3,d2` with `ADD.w`/`SUB.w` leaves `$FFFF` in the upper word.
  graphics.library takes the word, so it is not observable (#741,
  `docs/libs/deviations.md` 3a, kind K). The `libs_emulator` scenario
  observes the low word of d2 that `TextFit` gets (equal in both sets);
  the expected data have no form for a known difference without a fix,
  so the upper word is not a case.

## BUG(orig), kept

* The tree line pattern is written to `rp_LinePtrn` without the flags and
  count `SetDrPt` sets (#738).

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
none, the next node last or not and its words (the indent and the one
the original reads instead, #739) random, the
caller's d7 with upper words 0, `$FFFF`, `$8001`, `$0001`, `$8000`.
A message other than `LV_DRAW` (nothing drawn); one item of one text
field with graphics' `TextFit` patched (`SetFunction`, a stub that
records d0, d2 and d3 and goes on to `TextFit`); init/finit. Layouts that
would give graphics.library an empty rectangle (a RastPort without a
layer is not clipped) are skipped: items of 10 lines and more, BitMaps
lower than the item, no text or BitMap ending exactly at the field's
edge. The fixed bugs give `[orig]` / `[fixed #N]` cases: #737 those of
the items with an indent of 16 or more and the upper word `$FFFF` or
`$8000`, #739 the planes of the items whose last tree line changes,
#740 those of the items with a right-aligned BitMap (none of them starts
exactly at the field's left edge, which would give an empty rectangle).
Not covered: the original's 65536 tree lines (indent below 16 with an
upper word of 1..`$8000`: the scenario gives the original only upper
words without them), a text pointer 0, negative indents.
