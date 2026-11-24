# riwbspritelib (10): sprites on the Workbench screen

Reflective Images, 1994. Hardware sprites on Workbench (Intuition)
screens through the OS 3 sprite functions of graphics.library (below
V39 through those of the `SimpleSprite`, fixed #1151):

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

init opens graphics.library (any version) and chooses the sprite
functions by its version, finit frees every sprite of the table and
closes it. All routines are
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
bounds. Two graphics calls are inline assembler: `MoveSprite` and
`FreeSpriteData` (FreeWBSprite, a statement, returns the d0
`FreeSpriteData` leaves). `ChangeExtSpriteA` takes its tag list in
a3, a fixed register of the C code: the libraries' safe form of the NDK
macro keeps it (`cmake/ndk-calls.cmake`). The shape copy has a compiler
barrier so that GCC does not compute the end pointer with `__mulsi3`.

## Fixed

* ChangeWBSprite keeps a3 (#1028). The original passes its tag list to
  `ChangeExtSpriteA` in a3 (`LEA dudtaglist(pc),a3`) and returns without
  restoring it: as a function in a string expression the rest of the
  expression was written over the library's tag list.
* GetWBSprite returns -1 for every failure (no graphics.library, no
  shape data, a shape narrower than 16, a failing `AllocBitMap`,
  `AllocSpriteDataA` or `GetExtSpriteA`) and the channel 0–7 on success
  (#920, W7 decision Q4, test `rwe/riwbspritelib-getwbsprite-fail-returns-channel`).
  The original decided by the channel's table entry: when
  `GetExtSpriteA` or `AllocSpriteDataA` failed, the result was still the
  channel if the library held a sprite there, else 0; no graphics.library
  or a narrow shape gave 0 (which cannot be told from channel 0), no
  shape data or a failing `AllocBitMap` -1.
* MoveWBSprite returns -1 when it moved the sprite and 0 without one
  (#921, W7 decision Q8, test `rwe/riwbspritelib-movewbsprite-result`);
  the original, declared with a byte result, returned the d0
  `MoveSprite` leaves (a void function).
* ChangeWBSprite frees the new sprite data when `ChangeExtSpriteA`
  fails (#915; `libs_emulator` cases `[fixed #915]`, the step "ChangeWBSprite
  (change fails)" with graphics.library's function replaced); the
  original lost them, chip memory of graphics.library until reboot.
* The shape is copied into the bitmap row by row with the shape's
  `ebwidth` and the bitmap's `BytesPerRow` (#918; `libs_emulator` cases
  `[fixed #918]`, the image of the 20-wide sprite); the sprite keeps
  the shape's width rounded down to 16. The original copied width/16
  words per row back to back: a shape whose width is no multiple of 16
  has one word more per row, so its image was sheared and its next
  planes read from inside plane 0.
* With runtime error checking every command refuses a channel outside
  0–7 (-1 to 7 for GetWBSprite): "Sprite channel must be 0 to 7",
  "Sprite channel must be -1 to 7" (#916, test
  `rwe/riwbspritelib-channel-unchecked`; the original has no error check
  area). The release code is unchanged (an invalid argument, D-005):
  GetWBSprite stores at table + channel × 4 (a word index); FreeWBSprite
  takes the channel modulo 16 for the table (entries 8–15 are the name
  and the base) but passes the whole register to `FreeSprite`;
  MoveWBSprite, ChangeWBSprite and WBSpriteX/Y index the table with the
  channel as is. `GetWBSprite(10,10,0,8)` returned 8 (the "grap" of the
  name in entry 8) before #920's fix, -1 now.
* With runtime error checking GetWBSprite and ChangeWBSprite refuse a
  shape with data and a height of 0 ("Illegal Shape Height") or a depth
  of 0 or above 8 ("Illegal Shape Depth"), an invalid object that
  Blitz's shape commands do not make (#917, test
  `rwe/riwbspritelib-copy-dbra-zero`). The release code is unchanged
  (D-005): the copy counts height − 1 and depth − 1 with `DBRA` (65536
  rows or planes); for height 0 `AllocBitMap` fails first (-1), for
  depth 0 it does not return.
* Below graphics.library V39 (Kickstart 1.x and 2.x) the library has a
  path of its own (#1151, W7 Kickstart decision (b); tests
  `rwe/riwbspritelib-v39-calls`, `-v39-calls-kick204`,
  `-v39-shape-too-big`, `-v39-shape-too-big-kick204`, and
  `libs_emulator_riwbspritelib_a500_204`): GetWBSprite makes a
  `SimpleSprite` (in the block of an `ExtSprite`, one word wide) with
  its image in chip memory (the control words, per row the words of
  planes 0 and 1, the end words) and takes the channel with
  `GetSprite`; ChangeWBSprite gives the sprite a new image with
  `ChangeSprite` (at its position) and frees the old one; FreeWBSprite
  and finit free both blocks after `FreeSprite`. The four routines go
  through a table of function pointers (`struct sprfuncs`) that init
  chooses once by the version; from V39 on they are the original's.
  MoveWBSprite, WBSpriteX and WBSpriteY are the same on both paths. A
  sprite below V39 is 16 pixels wide (the shape's width rounded down to
  16, as from V39 on) with 2 planes: another shape gives -1
  (GetWBSprite) or 0 (ChangeWBSprite), with runtime error checking
  "Shape too big for a sprite below Kickstart 3.0". As from V39 on, a
  new sprite is not shown before MoveWBSprite or ChangeWBSprite.
  The original opened graphics.library in any version and called its V39
  functions behind the end of the jump table: a crash at the first
  GetWBSprite. Measured (2026-11-20): the same `MoveSprite` units on
  Kickstart 1.3, 2.04 and 3.1 (on the hires Workbench x halved, y as
  is); the control words of 1.3 and 2.04 place the sprite one lores
  pixel to the right of those of 3.1 on the a600 (graphics.library's own
  difference); a 32-pixel shape is refused on 3.1 with ECS (a600) as on
  the A1200's Workbench, so 16 pixels below V39 lose nothing there.
* With runtime error checking WBSpriteX and WBSpriteY refuse a channel
  without a sprite of the library: "No sprite on this channel" (#922,
  test `rwe/riwbspritelib-wbspritexy-no-sprite`). The release code is
  unchanged (D-005): it returns the word at offset 6 or 8 of the entry
  without testing it, for an empty entry the words at the addresses 6
  and 8 (the libs test's case "WBSpriteX (no sprite)").

## Kept on purpose (not fixed, decision)

* BUG(orig) (#919): finit does not clear the library base: a second finit closes
  graphics.library again.

## Test

The library takes real hardware sprites and shows them on the
Workbench screen, so a run loads one set only (on the A1200, the V39
path; the path below V39 as `libs_emulator_riwbspritelib_a500_204`, the
same scenario on the a500-204 with a 68020 and Zorro III memory, the C
build only, against `libs/tests/expected-a500-204/`, every case
`[fixed #1151]`): the whole scenario runs
in it (finit frees every sprite); each step's registers and a snapshot
are observed as cases and compared with the expected data
`libs/tests/expected/rwe_riwbspritelib.txt` (recorded from the original)
step by step. Snapshot: d0 (not for MoveWBSprite), the data
block (found by its "graphics.library" string; pointers as offsets or 0
/ not 0), each sprite's `ExtSprite` fields and image data, the change of
graphics.library's open count. Afterwards the free hardware sprites
(`GetSprite` 1–7) must be those of the start. Shapes are built in
memory (16, 20 and 32 wide, 1 and 2 planes, narrow, without data).
Steps: WBSpriteX without a sprite, init, GetWBSprite on a free channel,
on any (-1), on its own channel again (-1; the original's channel, fixed
#920), on the pointer's channel 0, 32 wide (refused on this screen),
narrow, no data, 20 wide (the left 16 pixels of each row; the
original's image sheared, fixed #918); WBSpriteX/Y; MoveWBSprite on two sprites and without one;
ChangeWBSprite (2 and 1 planes, no sprite, narrow, no data,
`ChangeExtSpriteA` failing: graphics.library's function replaced for
that call, `FreeSpriteData` counted);
FreeWBSprite with and without a sprite; finit with sprites left.
Checked to fail when the row count of the copy, the position of a
changed sprite or the channel of GetWBSprite(-1) is changed. Not
covered: channels outside 0–7, a second finit, failing `AllocBitMap`,
`AllocSpriteDataA`, height or depth 0 (the error checks: Blitz tests).
