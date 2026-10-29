# ritooltypeslib (ToolTypes, library 59)

**What it does:** the icon of a file and its tool types through
icon.library. `GetIconObject file$` opens one icon at a time
(GetDiskObjectNew) and points its `do_ToolTypes` to the library's own
array of 100 entries; `FindToolValue`, `FindToolNumber`, `FindToolType
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

**BUG(orig)** (kept):

* GetIconObject copies the icon's tool types without a limit into the
  100 entries (and reads from address 0 if the icon has none);
  NewToolType appends without a limit.
* SetToolValue matches a name that is a prefix of the entry (`FL`
  replaces `FLAG`) and skips the entry's own `(` (`OFF` replaces
  `(OFF)=1`); the new string is built with do-while loops (an empty name
  copies its 0 and then whatever follows).
* SetToolValue/NewToolType/IconDefTool: if AddFreeList fails, `MOVE.l
  d0,a1` frees address 0 (d0 is AddFreeList's 0), the block is lost.
* FindToolType tt$,1 sets the bracket check flag, which only a bracket
  search clears: after a hit without brackets, a name beginning with `(`
  or a call before GetIconObject the next FindToolType tt$ also looks for
  `(tt$)`. The bracket hit returns 1, not -1; a name over 253 characters
  overflows the 256-byte buffer.
* FindToolValue stops at a `)` also inside the value; it and
  FindToolNumber write no terminating 0 (invisible, see
  `docs/libs/cmdlibs-port.md`).
* IconDefTool rewinds a3 by the length of its argument + 1 and leaves it
  there.
* SetIconType without an icon returns its argument.
* PutIconObject ignores PutDiskObject's result and forgets the icon
  without FreeDiskObject (lost).
* ShapeToIcon shape,shape uses the library's own Image (PlanePick 0) as
  selected image and sets the gadget flags to 6, dropping the others.
* IconToShape fills the shape before allocating its data (no memory:
  size without data); copies `(width >> 4) + 1` words per row, one more
  than the shape's data has when the width is a multiple of 16; calls
  MakeCookie with a3 = the shape only if it was in use before
  (freeashape), otherwise with the caller's a3 (a cookie for whatever a3
  points to); with two shapes the second gets the cookie of the first
  if it is fresh (a3 still the first).

**DIFF:** ShapeToIcon shape,shape and IconToShape leave a3 unchanged (the
original uses it as a work register and leaves it on its Image or the
shape); a2 unchanged.

**Test** (`libs/tests/rwe/ritooltypeslib.c`, snitch case "rwe
ritooltypeslib"): the test writes `RAM:rwett.info` (icon.library's
default tool icon with its own seven tool types); both sets (each with
its own memlib, shapeslib and the libraries they need, string area,
argument stack, shapes, output files) work on it with the same inputs.
Compared: d0, a3 and the string area, the kept registers, the open icon
(tool types, default tool, type, gadget size and flags, both images), the
shapes of IconToShape (fields, data, cookie) and the written icons (read
back). Cases: every command and form without an icon; GetIconObject
(twice, a missing file); FindToolValue for every entry, a `)` in the
value and missing names; FindToolNumber over the array and beyond;
MatchToolValue; FindToolType with the stale bracket flag, with/without
bracket check, a name with `(`; SetToolValue (replace, prefix match, the
skipped `(`, status 0, empty value, not found); NewToolType (with/without
value and status); ClearToolTypes; SetIconHit; IconRender; SetIconType
0-6 and $103; IconDefTool (a3 rewound, empty string); ShapeToIcon both
forms; IconToShape both forms with shapes in use and fresh (the fresh
case with a3 on a decoy shape, which gets the cookie); PutIconObject both
forms; FreeIconObject; finit with an icon open; init again. Not covered:
icon.library missing, failing AllocMem/AddFreeList, more than 100 tool
types, IconToShape with a width that is a multiple of 16 (writes behind
the data) or when the icon has no image for it.
