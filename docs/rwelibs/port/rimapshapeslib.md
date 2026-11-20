# rimapshapeslib (6): map shapes and groups

Reflective Images, 1994–96 (history in the source: 08/07/96
`FreeMapShapes` and the check for empty shapes, 22/12/96 the group block
allocated). Map shapes are Blitz shapes seen as a `struct BitMap`, for
MapEd's block maps; they are blitted into a window's RastPort.

A map shape (64 bytes; map shape n at n × 64 in the block
`SetNumMapShapes` allocates): width.w, then a `struct BitMap`:
BytesPerRow = the shape's ebwidth, Rows = height, the depth word (=
Flags/Depth), pad, 8 plane pointers – the planes of the Blitz shape, not
copied. Width 0 = empty.

* `ShapeToMapShape shape#,mapshape#` – fills the map shape from the
  shape (plane n = data + n × onebpmem).
* `BlitMapShape window#,mapshape#,x,y` – `BltBitMapRastPort` from 0,0
  of the map shape (minterm $C0) to x,y of the window's RastPort, the
  whole shape; nothing for an empty one.
* `BlitMapShapeSize window#,mapshape#,x,y,width,height` – the same with
  the given size, the height at most the shape's (a signed compare).
* `RemapMapShape mapshape#,botcol,topcol` – does nothing.
* `SetNumMapShapes num` – frees the old block and allocates num × 64
  bytes (`AllocVec`, cleared).
* `MapShapeAddr(mapshape#)` – the map shape's address.
* `FreeMapShapes first,last` – clears the widths from first to last
  (nothing if last < first as signed words; the addresses are then
  compared signed as well).
* `ResetGroup blockwidth,blockheight` – empties the group, returns its
  address. The group: a 512-byte block (allocated by init) of members
  {map shape address.l, dx.w, dy.w}, ended by an address 0.
* `AddGroupMember mapshape#,dx,dy` – a member at dx × blockwidth,
  dy × blockheight (`MULU.w`, the low words).
* `BlitGroup window#,x,y` – blits every member at x + dx, y + dy
  (words).

init allocates the group block; finit frees the map shapes and the
group.

* **Source:** `rwelibsrc/MapEd/MapShape2.bb` (as TED shows it:
  `build/amiga/rwelibsrc/MapEd/MapShape2.bb`); `MapShape.bb` is the
  older subset.
* **Reference:** the reference build of the source by blitz2 2.10
  (`refbuild`, `build/amiga/libs/userlibs/rwe/ref/rimapshapeslib.obj`); no
  binary exists (`docs/rwelibs/README.md` §3.3). Declarations equal
  but for the intended differences of
  `libs/userlibs/rwe/rimapshapeslib.declarations-intended.txt` (ctest
  `rwe_declarations_rimapshapeslib`).
* **C file:** `libs/userlibs/rwe/rimapshapeslib.c`; test
  `libs/tests/rwe/rimapshapeslib.c` (snitch case "rwe rimapshapeslib").

## Declarations

`RemapMapShape` has `!libs {#chipbaselib,#la2,254,$5600}`: the second
pair is no valid library/register code (probably a mistyped
`#graphicslib,#la6`); the C file emits the same words.

## C and assembler

C behind thunks (d0–d1/a0–a1 kept by the statements, d1/a0–a1 by the
functions; the original keeps less), but the three blit entries
`BlitMapShape`, `BlitMapShapeSize` and `BlitGroup` (with the shared
`_blitmapshape2`), which are the original's assembler (`BLITZ_ASM_ROUTINE`)
for speed: `BlitMapShape` is a map editor's call per tile, and the C
version took about 530 cycles more per call than the original (9461
against 8931 for a 16 × 16 map shape of 2 planes into a window, a600);
as assembler 8931. They change d0, d1, a0, a1 and the low words of d4–d6,
as the original. `RemapMapShape` is an empty C function. The
`BltBitMapRastPort` calls get what the original passes in
the registers, upper words included: x and y (and width/height of
`BlitMapShapeSize`) are the caller's longs; width, height and minterm
are loaded with `MOVE.w` into the caller's d4/d5/d6 – in `BlitGroup` d5
holds the member's address before, so its upper word is that address's.

## BUG(orig), kept

* The map shape number is not checked; before `SetNumMapShapes` (or
  after one that failed or allocated 0 shapes) the map shapes lie at
  address 0 (#906).

## Fixed

Intended deviations, `docs/libs/deviations.md` (3a), kind F:

* init sets the next-member pointer to its group block (cleared, so an
  empty group) and the block size to 0, as at the first load. The
  original set only the group's address: an `AddGroupMember` before the
  first `ResetGroup` wrote the member and the end mark to addresses 0–11,
  over the ExecBase pointer (#901; test
  `rwe/rimapshapeslib-addgroupmember-before-resetgroup`), and in a second
  run of a program from the editor (the library stays loaded) into the
  run before's freed group, with the run before's block size (#1134; the
  libs test's second run).
* Without the group block (init's `AllocVec` failed) `ResetGroup` writes
  nothing and returns 0, `AddGroupMember` and `BlitGroup` do nothing
  (D-018: fail silently; one test per call). The original did not check
  the `AllocVec`: `ResetGroup` cleared the long at address 0,
  `AddGroupMember` wrote its members from 0 on (over the ExecBase
  pointer) and `BlitGroup` read the group there (#905; test
  `rwe/rimapshapeslib-init-allocvec-unchecked`, inject allocfail).
* `BlitGroup`'s `!libs` has `#graphicslib,#la6`, as `BlitMapShape`'s and
  `BlitMapShapeSize`'s: the compiler loads graphics.library into a6. The
  original's named the window only, so `BlitGroup` jumped to offset −606
  of whatever the program had left in a6 – it worked only after a
  command that left graphics.library there (#902; test
  `rwe/rimapshapeslib-blitgroup-caller-a6`). A declaration change: one
  library/register pair more.
* With runtime error checking `ShapeToMapShape` refuses a shape of depth
  0 or of more than 8 planes: "Illegal Shape Depth" (shapeslib's text;
  `shapetomapshapechk`, in an error check area the original does not
  have). The release code is unchanged (an invalid object, D-005): depth
  0 writes 65536 plane pointers (`DBRA`), depths 9–13 write into the
  unused bytes 42–63 of the 64-byte map shape, from depth 14 on the
  pointers run into the next map shape (#903; tests
  `rwe/rimapshapeslib-depth0-dbra`, `rwe/rimapshapeslib-depth-above-8`).
* The group holds at most 63 members (504 bytes and the end mark):
  `AddGroupMember` adds a member only below the group's end, which init
  keeps (an unsigned compare; the end is 0 without a group block, #905),
  and ignores further ones; with runtime error checking it refuses them:
  "Group holds at most 63 members" (`addgroupmemberchk`). The original
  did not count: the end mark of the 64th member was written behind the
  512-byte block, each further member 8 bytes more (#904; w7-decisions
  Q5 (a): refused in the release code plus the message; tests
  `rwe/rimapshapeslib-group-no-limit`, `rwe/rimapshapeslib-group-full-check`).

Not a defect: `RemapMapShape` does nothing (the `ReMap` call is
commented out in the source – a command left unfinished, not a fault in
what it does).

## Test

Each run loads one library set (the C build or the original); the
scenario writes its observations as cases, compared with the expected
data `libs/tests/expected/rwe_rimapshapeslib.txt` recorded from the
original. Blitz shapes (random data in chip memory), a window object
whose Window/RastPort/BitMap are built by the test (no layer), the map
shapes and group of the library. Observed: the map shapes (all fields, the
plane pointers relative to the shapes' data), the group (member
addresses relative to map shape 0, the offsets), the results relative to
the blocks, the bitmap after every blit (and that a blit changed it, an
empty map shape not), the kept registers. Covered: every entry: shapes
of depth 1–3, widths 16–48, odd heights; blits at several positions,
`BlitMapShapeSize` with a smaller and a clamped height; groups with an
empty member, three block sizes, an empty group; `FreeMapShapes` with a
range, last < first, one shape, first = −1 (the word compare passes, the
address compare not); a second `SetNumMapShapes`; init, finit; a second run with the library
still loaded (the freed group of the run before reserved by the test):
init, `SetNumMapShapes`, `AddGroupMember` before `ResetGroup`. Checked
to fail with a wrong blit width in the C build. Not covered: the bugs
above, blits that leave the bitmap or have a size ≤ 0 (without a layer
nothing clips them).
