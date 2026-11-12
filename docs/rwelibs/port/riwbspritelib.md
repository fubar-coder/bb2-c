# riwbspritelib (10): sprites on the Workbench screen

Reflective Images, 1994. Hardware sprites on Workbench (Intuition)
screens through the OS 3 sprite functions of graphics.library:

* `GetWBSprite(x,y,shape[,spritechannel])` – converts a Blitz shape
  (copied into an `AllocBitMap` bitmap, then `AllocSpriteDataA`) and
  takes a sprite channel for it (`GetExtSpriteA`; channel -1 = any);
  returns the channel. The shape is the item of parameter 2 (`#ia0|pd2`).
* `FreeWBSprite sprite_channel` – `FreeSprite`, `FreeSpriteData`.
* `MoveWBSprite(viewport,x,y,sprite_channel)` – `MoveSprite`.
* `ChangeWBSprite(viewport,sprite_channel,shape#)` – a new image at the
  same position (`ChangeExtSpriteA`); -1, 0 if it failed.
* `WBSpriteX(sprite_channel)`, `WBSpriteY(sprite_channel)` – the
  position.

init opens graphics.library (any version; the functions need 39), finit
frees every sprite of the table and closes it. All routines are
Amiga-mode only (`+1`).

* **Source:** `rwelibsrc/wbspritelib/WBSprite-library2.bb` (as TED shows
  it: `build/amiga/rwelibsrc/wbspritelib/WBSprite-library2.bb`).
* **Reference:** the reference build of the source by blitz2 2.10
  (`refbuild`, `build/amiga/libs/userlibs/rwe/ref/riwbspritelib.obj`); no
  binary of the library exists (`docs/rwelibs/README.md` §2).
  Declarations equal (ctest `rwe_declarations_riwbspritelib`).
* **C file:** `libs/userlibs/rwe/riwbspritelib.c`; test
  `libs/tests/rwe/riwbspritelib.c` (snitch case "rwe riwbspritelib").

## C and assembler

All C behind `BLITZ_THUNK`s (keep d1/a0/a1; the original changes
d0–d7/a0–a2/a6). The library's variables, the tag lists (the
`GetExtSpriteA` list has no `TAG_DONE`: the empty list behind it ends
it), the sprite table, the library name and base are one packed struct
in the original's order, since the routines index the table without
bounds. Three graphics calls are inline assembler: `MoveSprite` and
`FreeSpriteData` (MoveWBSprite and FreeWBSprite return the d0 they
leave) and `ChangeExtSpriteA` (its tag list in a3, a fixed register of
the C code: saved around the call). The shape copy has a compiler
barrier so that GCC does not compute the end pointer with `__mulsi3`.

## BUG(orig), kept

* No channel is checked: GetWBSprite stores at table + channel × 4 (a
  word index); FreeWBSprite takes the channel modulo 16 for the table
  (entries 8–15 are the name and the base) but passes the whole register
  to `FreeSprite`; MoveWBSprite, ChangeWBSprite and WBSpriteX/Y index the
  table with the channel as is (#916) (without a sprite WBSpriteX/Y read
  the words at addresses 6 and 8, #922).
* GetWBSprite: when `GetExtSpriteA` fails, the result is still the
  channel if the table has a sprite there (an earlier one of the
  library), else 0 (for channel -1 the entry before the table, the end
  of the tag list: 0). Results: no graphics.library or a shape narrower
  than 16: 0; no shape data or a failing `AllocBitMap`: -1 (#920).
* The shape is copied width/16 words per row into the bitmap, without
  its `BytesPerRow` or the shape's `ebwidth` (#918), with `DBRA` counts
  (height or depth 0: 65536 rows/planes) (#917).
* ChangeWBSprite does not free the new sprite data if
  `ChangeExtSpriteA` fails (#915).
* MoveWBSprite returns the d0 `MoveSprite` leaves (declared as a byte
  result) (#921).
* finit does not clear the library base: a second finit closes
  graphics.library again (#919).

## Test

The library takes real hardware sprites and shows them on the
Workbench screen, so a run loads one set only: the whole scenario runs
in it (finit frees every sprite); each step's registers and a snapshot
are observed as cases and compared with the expected data step by step. Snapshot: d0 (not for MoveWBSprite), the data
block (found by its "graphics.library" string; pointers as offsets or 0
/ not 0), each sprite's `ExtSprite` fields and image data, the change of
graphics.library's open count. Afterwards the free hardware sprites
(`GetSprite` 1–7) must be those of the start. Shapes are built in
memory (16, 20 and 32 wide, 1 and 2 planes, narrow, without data).
Steps: WBSpriteX without a sprite, init, GetWBSprite on a free channel,
on any (-1), on its own channel again (BUG(orig) #920: the channel), on the
pointer's channel 0, 32 wide (refused on this screen), narrow, no data,
20 wide; WBSpriteX/Y; MoveWBSprite on two sprites and without one;
ChangeWBSprite (2 and 1 planes, no sprite, narrow, no data);
FreeWBSprite with and without a sprite; finit with sprites left.
Checked to fail when the row count of the copy, the position of a
changed sprite or the channel of GetWBSprite(-1) is changed. Not
covered: channels outside 0–7, a second finit, failing `AllocBitMap`,
`AllocSpriteDataA` or `ChangeExtSpriteA`, height or depth 0.
