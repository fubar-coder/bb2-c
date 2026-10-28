# Command libraries: source (bblibsrc) vs. CD binary (acidlibs)

State: comparison of the **declaration parts** (header, token entries with
kind/forms/args/libs/name/help text, libfin) of the 58 command libraries in
`BLITZ2/ACIDLIBS` of the CD (BB2.1; identical to `BUM9A/BLITZ2/ACIDLIBS` and
`BUM10B/BLITZ2/ACIDLIBS`, all under `archive/cd/ultimate-blitz-basic-2.1/`) with the sources in
`archive/aminet/amibl2src/bblibsrc/`.
The code itself is **not** compared. Tool (bb2 only, not taken over):

```
python3 -I libs-c/tools/srcdecl.py report cd/blitz2/acidlibs <bblibsrc>
python3 -I libs-c/tools/srcdecl.py compare cd/blitz2/acidlibs <bblibsrc>/gtlib
```

## How the libs were built

The sources are normal BB2 programs that were assembled with the Blitz
assembler; the declaration macros (`!libheader`, `!astatement`,
`!args`, `!libs`, `!subs`, `!name`, `!nullsub`, `!libfin` …) are in
`EXTRAS/BLITZLIBS/LIBMACS.BB` of the CD (identical in `BUM9B|BUM10A/BLITZLIBS/
LIBMACS.BB2`, there also `.res`), the lib numbers in `libnums.bb`
(`INCLUDE "blitzlibs:libnums.bb"` at the end of libmacs). The macros generate
only `Dc` data in exactly the format that `biglib.parse_lib` reads. That is why
`srcdecl.py` evaluates the macros on the host (no Amiberry needed):

* Constants from libmacs.bb, libnums.bb (later definition wins), then
  `#x=…` of the source.
* BB2 peculiarity: `#` is optional in assembler expressions (`la6`,
  `shapeslib`, `#word|push` yield the constants) - confirmed in the binary.
* Labels = set code offset, `0` = unset (like `biglib.without_code`).
* Hand-written entry header `Dc 3 LSL 8+3,0,0` (windowslib WaitEvent)
  is recognised.

Cross-check: 24 of 25 system lib sources yield "identical"; exception
debuglib (binary has one more nullsub, `last` different - known, see
libs/acidlibs/debuglib.c: binary newer than source).

## Result

* **52 identical**, **1 identical except for the lib number** (lib 1),
  **5 with deviations**, **0 without source**.
* The lib number always comes from `#name` + libnums.bb, except where stated.

| No | Source | Entries src/bin | Result |
|---:|---|---|---|
| 1 | dhandlerlib | 2/2 | identical except for lib no.: `#dhandlerlib` is not defined in libnums.bb, binary has 1 |
| 74 | crunchlib (`#romuluscrunchlib`) | 4/4 | identical |
| 76 | banklib | 13/13 | identical |
| 78 | datelib | 11/11 | identical |
| 80 | asllib (`#myasllib`) | 6/6 | identical |
| 82 | medlib | 18/18 | identical |
| 88 | speaklib | 7/7 | identical |
| 90 | seriallib | 13/13 | identical |
| 92 | arexxlib | 16/16 | identical |
| 94 | animlib | 10/10 | identical |
| 96 | trackerlib | 9/9 | identical |
| 98 | blitzkeyslib | 7/7 | identical |
| 100 | scrolllib | 7/7 | identical |
| 104 | collslib | 12/12 | identical |
| 106 | cliargslib | 4/4 | identical |
| 108 | mouselib | 9/9 | identical |
| 110 | sortlib | 4/4 | identical |
| 112 | spriteslib | 14/14 | identical |
| 114 | iffmakelib | 5/5 | identical |
| 116 | audiolib | 18/18 | identical |
| 117 | blitzoslib | 32/32 | identical |
| 118 | shapetrixlib | 4/4 | identical |
| 119 | 2dlib (`#sis2dlib`) | 17/17 | identical |
| 120 | vallib | 16/16 | identical |
| 121 | polylib (`#chunkylib=121` in the source) | 8/8 | identical |
| 122 | stencillib | 7/7 | identical |
| 124 | fileiolib | 25/25 | identical |
| 128 | linklistlib | 18/15 | **source newer**: SetItem, CountList, CountItem additionally at the end |
| 130 | rawkeylib | 5/5 | identical |
| 132 | brexxlib | 23/23 | identical |
| 134 | intuifontlib | 8/7 | **source newer**: token `Text` additionally at the end |
| 136 | menuslib | 17/17 | identical |
| 138 | windowslib | 75/75 | identical |
| 140 | gadgetslib | 39/39 | identical |
| 141 | gtlib (`#mygadtoolslib`) | 37/40 | **different state**, see below |
| 142 | fontlib | 15/15 | identical |
| 143 | displaylib | 25/25 | identical |
| 144 | blitzcoplib | 20/19 | **source newer**: token `Help` additionally at the end |
| 146 | bblitlib | 11/11 | identical |
| 148 | qblitlib | 10/10 | identical |
| 150 | blitlib | 10/10 | identical |
| 152 | freqlib | 1/1 | identical |
| 154 | shapeslib | 21/21 | identical |
| 156 | palettelib | 33/33 | **Args**: Red/Green/Blue source `!args{2}` (#word), binary 3 (#long) |
| 157 | fadelib (`libheader{157`) | 6/6 | identical |
| 158 | screenslib | 20/20 | identical |
| 160 | bitmaplib | 11/11 | identical |
| 161 | ilbmifflib | 9/9 | identical |
| 162 | iffiolib | 9/9 | identical |
| 164 | inputoutputlib | 12/12 | identical |
| 180 | memacclib | 4/4 | identical |
| 185 | stringfunclib | 28/28 | identical |
| 186 | editlib | 3/3 | identical |
| 187 | printlib | 7/7 | identical |
| 190 | gameiolib | 5/5 | identical |
| 192 | mathtranslib (2nd definition =192) | 16/16 | identical |
| 193 | environlib | 9/9 | identical |
| 194 | utillib | 6/6 | identical |

## Details on the deviations

* **141 gtlib** - source and binary are separate further developments
  (source comments up to "November 1994 gtsetattrs fixed by Steve"):
  * source only: `GTMakeCycleTag` (token 31, "Simon was here!");
  * binary only: `DetachGTList`, `GTGimmeCludge`, `GTSetHighlight`,
    `GTFreeGadget` (tokens 31-34);
  * `GTList` help: source "A Gadg**T**ools GadgetList", binary "A GadTools …";
  * `AttachGTList`: binary has an error check routine (subs err1 set),
    source does not;
  * `GTSetAttrs`: source `!astatement`, binary kind $303 (statement *and*
    function, type #long - like windowslib WaitEvent).
  * `oldgtlib` is older (without GTStatus/GTArrowSize/GTMakeCycleTag;
    AttachGTList without intuitionlib base in a6 - "presumed a6 had gtlib").
* **156 palettelib**: only the argument types of Red/Green/Blue (binary
  takes #long). `palettelib.aga` is an older AGA draft (18 entries,
  without AGARGB/AGAPalRGB/AGARed…).
* **128/134/144**: source has new tokens at the end (token numbers of the
  others stay the same) - so the source is newer than BB2.1.

## Further source files (not on the CD, or variants)

* Variants of the same number: `sis2dlib.bb` (older than 2dlib: without
  ClipBlitMode/BitMapWindow/BitMapOrigin, Line/ClipBlit with fewer forms),
  `olddisplaylib`, `displaylibfuk` (older displaylib states), `oldgtlib`,
  `palettelib.aga`.
* `renderlib` = polylib except for `#renderlib` (libnums: 87, comment
  `; #renderlib=121`): declarations identical to lib 121.
* Not in acidlibs: alglib (#algebralib=9), cd32lib (113), chunkylib (72 from
  libnums; 29 entries, ChunkyCLS… - the tokens of 121 come from polylib),
  ctrlclib and xprlib (`#ctrlclib`/`#xprlib` not in libnums).
* `archive/aminet/amibl2src/bb2src/linklistlib` is byte-identical to bblibsrc/linklistlib.
* The RWE sources (`RWELibSourc.lha.dir/Libs`) contain none of the
  acidlibs numbers.

## Version hints

All files in the archive carry the same date (14.09.1995), so no
hint. Comments in the sources go up to "bum8" (animlib, 2dlib,
utillib, blitzoslib) or "March 1995" (fileiolib); bum9a/bum10b ship
the same acidlibs as the 2.1 CD. The sources are therefore roughly the
2.1 state, for linklistlib/intuifontlib/blitzcoplib/gtlib somewhat later.

## Open points

* Code parts are not compared; for the port, as with the
  system libs: the binary is the reference (bb2 `tools/disasm.py`, not taken over).
* Lib 1 (dhandlerlib): which number definition it was built with is
  unclear (libnums.bb does not know `#dhandlerlib`).
