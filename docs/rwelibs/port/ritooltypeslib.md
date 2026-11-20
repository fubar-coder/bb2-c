# ritooltypeslib (ToolTypes, library 59)

**What it does:** the icon of a file and its tool types through
icon.library. `GetIconObject file$` opens one icon at a time
(GetDiskObjectNew; below icon.library V36, on Kickstart 1.x,
GetDiskObject) and points its `do_ToolTypes` to the library's own
array (allocated, grown by NewToolType); `FindToolValue`, `FindToolNumber`, `FindToolType
[,bracket_check]` (also looks for `(name)`), `MatchToolValue` read it,
`SetToolValue`/`NewToolType [,status]` replace/append entries (status 0
= `(name=value)`, switched off), `ClearToolTypes` empties it;
`SetIconHit`, `IconRender`, `SetIconType`, `IconDefTool` change the
icon; `ShapeToIcon`/`IconToShape` exchange its images with shapeslib's
shapes; `PutIconObject file$[,type]` writes it, `FreeIconObject` frees
it. New strings are AllocMem'd and hung on the icon's FreeList
(AddFreeList at DiskObject+78). All commands are Amiga mode only.

**Source and reference:** `rwelibsrc/tooltypeslib/ToolTypes4.bb`
(Reflective Images, Stephen McNamara, 5/4/95), reference BSS2
`RITOOLTYPESLIB.OBJ` (3564 bytes) = the blitz2 2.10 build of the
source. The C build has the same declarations
(`rwe_declarations_ritooltypeslib`); the code is not byte-identical.
Library number `LIB_ritooltypes` (59) is in the bb2-2.10 libnums.

**C:** everything (`libs/userlibs/rwe/ritooltypeslib.c`): icon.library
and exec through the NDK inlines (base `iconlibbase`, `ICON_BASE_NAME`),
memlib globalloc/globfree and shapeslib `MakeCookie` through
`BLITZ_ALIBJSR`. String lengths are read below a2 (a2 stays unchanged,
DIFF as in all C ports); FindToolValue and FindToolNumber write their
result at a3 (`blitz_string`). `MakeCookie` takes its shape in a3: the C
code sets a3 for that call to what the original has there and restores
it afterwards. Every entry is a thunk that keeps those of d0/d1/a0/a1
that are not the result.

**Fixed** (intended deviations from the original, kind F in
`docs/libs/deviations.md`):

* SetIconType without an open icon returns 0; the original returned its
  argument, so `SetIconType 3` gave 3, success for a program that tests
  the result (#852; bug-orig test
  `ritooltypeslib-seticontype-noicon-returns-arg`).
* FindToolType tt$ (one argument) never searches `(tt$)`; the bracket
  check is a parameter of the call. The original kept it in a flag that
  FindToolType tt$,1 set and only the end of the bracket search cleared:
  after such a call that returned earlier (a hit without brackets, a name
  beginning with `(`, no icon, no memory) the next FindToolType tt$ also
  found the switched-off `(tt$)` and returned 1 (#843; bug-orig test
  `ritooltypeslib-findtooltype-stale-bracket-flag`).
* FindToolValue copies the whole value; a `)` at its end is dropped only
  for a name given with `(` (the only name icon.library finds a
  switched-off `(name=value)` with). The original stopped at the first
  `)`, also inside a value: `PAREN=a)b` gave `a` (#844; bug-orig test
  `ritooltypeslib-findtoolvalue-stops-at-paren`).
* SetToolValue replaces the entry of the name: after the name (its own
  `(` skipped, as before) the entry must end or go on with `=` or `)`.
  The original took the first entry that only began with the name (`FL`
  replaced `FLAG`); a program that used an abbreviation of a name on
  purpose now gets 0 instead of a replaced entry (#853; bug-orig test
  `ritooltypeslib-settoolvalue-prefix-match`).
* SetToolValue/NewToolType with an empty name make the entry `=value`
  (`=` and the value; with status 0 `(=value)`). The original copied the
  name with a do-while loop: its 0 first, then whatever followed it up
  to the next 0, so the entry began with 0 and read back as an empty
  string (#841; bug-orig test `ritooltypeslib-empty-name-copies-on`).
* SetToolValue/NewToolType/IconDefTool free their new block when
  AddFreeList fails and return 0. The original's `MOVE.l d0,a1` freed
  address 0 (d0 is AddFreeList's 0) and lost the block (#840; bug-orig
  test `ritooltypeslib-addfreelist-fail-frees-0`, through inject).
* The bracket search of FindToolType tt$,1 builds `(tt$)` in a buffer of
  the name's length + 3 bytes. The original's buffer had 256 bytes; a
  name over 253 characters wrote behind it (#842; bug-orig test
  `ritooltypeslib-findtooltype-name-overflow`, inject guard).
* The tool type array has no fixed size: GetIconObject allocates it for
  the icon's tool types and the terminator, NewToolType replaces a full
  one by one of twice the size (+ 2); FreeIconObject, PutIconObject (when
  it closes the icon) and finit free it. Without memory GetIconObject
  and NewToolType return 0 and change nothing. The original had a static
  array of 100 entries and counted nowhere: an icon with more than 100
  tool types, or a 100th NewToolType, wrote behind it (#855, data from
  outside, the user's decision of 2026-11-17; bug-orig tests
  `ritooltypeslib-tooltype-array-unbounded` with an icon of 101 tool
  types, and `ritooltypeslib-tooltype-array-nomem`).
* PutIconObject returns 0 when PutDiskObject fails, and the icon stays
  open: the program can write it again or free it with FreeIconObject.
  The original ignored the result, returned -1 and closed the icon also
  then (#851, the user's decision of 2026-11-17; bug-orig test
  `ritooltypeslib-puticonobject-result-ignored`).
* PutIconObject frees the written icon (FreeDiskObject) as
  FreeIconObject does; the original forgot it, and its memory stayed
  allocated until a reset (#850; bug-orig test
  `ritooltypeslib-puticonobject-leaks-icon`, inject track).
* IconToShape copies `(width + 15) >> 4` words per row, the words it
  allocates for the shape's data. The original copied `(width >> 4) +
  1`, one word per row and plane more when the width is a multiple of 16,
  behind the data (#847; bug-orig test
  `ritooltypeslib-icontoshape-extra-word`, inject guard).
* ShapeToIcon shape,shape gives the library's own Image for the selected
  image PlanePick `(1 << depth) - 1` (all planes of the shape) and
  PlaneOnOff 0; the original left PlanePick 0, no plane picked (#854;
  bug-orig test `ritooltypeslib-shapetoicon-planepick-0`). The gadget
  flags 6 (GADGHIMAGE, the others dropped) stay: RIToolTypesLib.guide,
  node IconRender, documents them.
* ShapeToIcon (both forms) gives the icon's own Image (GadgetRender)
  PlanePick `(1 << depth) - 1` of the shape and PlaneOnOff 0; the
  original kept the icon's values, so a shape of another depth than the
  icon's image had planes picked that it does not have (Intuition reads
  them behind the shape's data) or planes that were not drawn (#1083;
  bug-orig test `ritooltypeslib-shapetoicon-keeps-planepick`).
* GetIconObject reads the icon with `GetDiskObjectNew` from icon.library
  V36 on and with `GetDiskObject` below (Workbench 1.x has icon.library
  V34). The original called `GetDiskObjectNew` there too, behind
  icon.library's jump table: a crash at the first GetIconObject on
  Kickstart 1.x. The difference stays: on Kickstart 1.x a file without
  an icon gives 0, from V36 on `GetDiskObjectNew` gives a default icon
  for it (RIToolTypesLib.guide describes the GetDiskObject behaviour:
  "will return either FALSE for failure") (#1149; bug-orig test
  `ritooltypeslib-v36-calls`, a500, a500-204, a1200).

**BUG(orig)** (kept):

* GetIconObject reads the icon's tool types from address 0 if the icon
  has none (#845, closed as not planned: not observable - the long at
  address 0 is 0 on Kickstart 1.3, 2.04 and 3.1, so the count stops at
  once and the icon has no tool types, as `ritooltypeslib-v36-calls`
  shows with such an icon; it is an Enforcer hit only).
* IconDefTool rewinds a3 by the length of its argument + 1 and leaves it
  there. This is no defect (#846, closed): it gives back the space of the
  string argument, which the compiler does not take back after a library
  function with a numeric result. Without it
  `"ab"+Str$(IconDefTool("tool"))+"cd"` gives `abtool` instead of `ab-1cd`.
* IconToShape fills the shape before allocating its data (no memory:
  size without data) (#848; not a bug in practice: a failing globalloc never returns, memlib ends the program with "Out of Memory!").

Not a defect in compiled programs: IconToShape calls MakeCookie with a3 =
the shape only if it was in use before (freeashape), otherwise with the
caller's a3; with two shapes the second call has a3 still on the first
shape. The compiler links each ALibJsr call through a stub that loads the
callee's registers from its declaration first (as for editlib, #130,
`docs/libs/cmdlibs-port.md`): MakeCookie's a3 is `Shape(d0)`, the shape
number IconToShape passes, so every shape gets its own cookie (#849). The
library test calls MakeCookie without that stub, so its case "IconToShape
(fresh, a3 = decoy)" shows the cookie on the decoy.

Original behaviour that is no defect: SetToolValue skips the entry's own
`(` (`OFF` replaces `(OFF)=1`, the switched-off form of the same entry);
FindToolType's bracket hit (a switched-off entry) returns 1, not -1 (it
tells the two apart); FindToolValue and FindToolNumber write no
terminating 0 (invisible: string results need none, see #257 and
`docs/libs/cmdlibs-port.md`).

**DIFF:** ShapeToIcon shape,shape and IconToShape leave a3 unchanged (the
original uses it as a work register and leaves it on its Image or the
shape); a2 unchanged.

**Test** (`libs/tests/rwe/ritooltypeslib.c`, snitch case "rwe
ritooltypeslib"): each run loads one library set (the C build or the CD
original); the scenario writes its observations as cases, compared with
the expected data `libs/tests/expected/rwe_ritooltypeslib.txt` recorded
from the original. The test writes `RAM:rwett.info` (icon.library's
default tool icon with its own seven tool types); the set (with its
memlib, shapeslib and the libraries they need) works on it, with the
test's string area, argument stack, shapes and output files.
Observed: d0, a3 and the string area, the kept registers, the open icon
(tool types, default tool, type, gadget size and flags, both images), the
shapes of IconToShape (fields, data, cookie) and the written icons (read
back). Cases: every command and form without an icon; GetIconObject
(twice, a missing file); FindToolValue for every entry, a `)` in the
value and missing names; FindToolNumber over the array and beyond;
MatchToolValue; FindToolType after a bracket check that returned early, with/without
bracket check, a name with `(`; SetToolValue (replace, "FL" and the entry `FLAG` (fixed #853: not found), the
skipped `(`, status 0, empty value, not found); NewToolType (with/without
value and status); ClearToolTypes; SetIconHit; IconRender; SetIconType
0-6 and $103; IconDefTool (a3 rewound, empty string); ShapeToIcon both
forms; IconToShape both forms with shapes in use and fresh (the fresh
case with a3 on a decoy shape, which gets the cookie); PutIconObject both
forms; FreeIconObject; finit with an icon open; init again. Not covered:
icon.library missing, failing AllocMem/AddFreeList, more than 100 tool
types, IconToShape with a width that is a multiple of 16 (writes behind
the data) or when the icon has no image for it.
