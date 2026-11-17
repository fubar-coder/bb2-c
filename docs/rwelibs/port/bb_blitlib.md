# bb_blitlib (19): Blitz Bombers blitter routines

Reflective Images (Stephen McNamara). The custom blitter code of Blitz
Bombers: 12 Blitz mode commands that program the blitter registers
directly (no `OwnBlitter`), on bitmaps, shapes and images in chip
memory:

* `SpecialBlock top,bottom,x,y` / `SpecialBlock top,bottom,offset` – a
  16×16 block: the top shape over the bottom shape through top's cookie
  into the used bitmap (`D = A ? B : C`).
* `SpecialBlockBitmap top,bitmap,x,y` / `…,offset` – a shape of any size
  over the same area of another bitmap into the used bitmap.
* `TBlock shape,x,y` – a 16×16 shape through its cookie.
* `PredatorBlit(shape,x,y)` – the "predator" effect: inside the shifted
  cookie the bitmap is replaced by the line below, one pixel further;
  returns the cookie address.
* `BPredatorBlit buffer,shape,x,y` – the same, buffered: the background
  is saved into a record of a `Buffer` object as bblitlib's `BBlit`
  does (so `UnBuffer` restores it), with one line less ("BB Cludge").
* `SetupMapTwo ebwidth` – the bytes per line (default 40) that
  `SpecialBlock`, `TBlock` and the predator blits assume.
* `CreateShape start,count,chipmem,like` – `count` shape objects like
  `like`, their data one after another at `chipmem`, no cookie, `"cL"`.
* `SpecialBlockToShape top,bottom,dest`, `BlockToShape shape,dest` – the
  block into a shape / a shape's data copied (error checks: the
  destination has data and the same size; `BlockToShape`: the shape fits
  one blit).
* `BColourBlit buffer,shape,x,y,colour` – buffered: the shifted cookie in
  one colour (per plane `D = A | C` or `~A & C`).
* `TBlockImage image_addr,image,x,y` / `BTBlockImage
  buffer,image_addr,x,y,image` – 16×16 images of 6 planes (224 bytes:
  mask, then the planes) through their mask, the second buffered and
  shifted.

init and finit are `RTS`.

* **Source:** `rwelibsrc/blitzbomberslib/BB_BlitLib4.bb` (30/1/96; as TED
  shows it: `build/amiga/rwelibsrc/blitzbomberslib/BB_BlitLib4.bb`).
* **Reference:** the reference build of the source by blitz2 2.10
  (`refbuild`, `build/amiga/libs/userlibs/rwe/ref/bb_blitlib.obj`); no
  binary exists (`docs/rwelibs/README.md` §3.3). Declarations equal
  (ctest `rwe_declarations_bb_blitlib`) but for the error checks the
  fixes add (`bb_blitlib.declarations-intended.txt`). Number 19 (the literal of the
  source, = `#bb_blitlib`).
* **C file:** `libs/userlibs/rwe/bb_blitlib.c`; test
  `libs/tests/rwe/bb_blitlib.c` (snitch case "rwe bb_blitlib").

## C and assembler

No entry has a thunk, as the register rule allows
(`docs/libs/deviations.md`, "Register preservation"): a statement keeps
a4–a7, `PredatorBlit` (a function) also a3. Every command is faster
than the original (perf program `bbblit`, `tests/perf/budgets.txt`).
The C entries save no register (`BLITZ_ENTRYPOINT`, a6 kept by
`BLITZ_EP_KEEP_A6`) and change d0–d7/a0–a2; their plane loops are the
original's loops as asm blocks (in C, GCC tested the counter and
branched twice per plane instead of one `DBF`), and they write the
blitter registers through `$DFF000` in an address register, 4 cycles
less per write than the original's absolute addresses. `SetupMapTwo`
writes its three words through an address register, `CreateShape` its
objects with `MOVE (a2)+` and 0 and "cL" from registers. The buffered
blits (`BPredatorBlit`, `BColourBlit`, `BTBlockImage`) are assembler
routines that follow the original's code with the fixes: in C, GCC had
too few address registers for the record, the planes, the bitmap, the
shape and the chip base and kept values on the stack. They write every
register through the chip base in a1 and use a3 (free in a statement)
instead of the original's saved a4/a5; they change d0–d7/a0–a3. The
original changes up to d0–d7/a0–a3 (the routines differ) and keeps
a4–a6, so a caller can rely on nothing more than the rule names. The
blitter gets the same registers in the same order as in the original
(volatile writes, `BL_CUSTOMW`/`BL_CUSTOML`, or the assembler's moves).
Pointers
that the original does not reload per plane (B and C of `SpecialBlock`,
B of `SpecialBlockBitmap`, `BTBlockImage`) go on where the blitter left
them, as in the original. The record of the buffered blits is written by
the CPU like the original's (the save blit writes into it). The address
arithmetic is the original's (`MULU.w`/`MULS.w`, `ASR.w`, word sums).
`ADD.l ebwidth-2(pc)` adds the long `pad:ebwidth`, i.e. `ebwidth` (pad
is 0). The error checks are C in the error check area; they keep d0,
which the original changes (low word: +26 of a shape). The checks of
`CreateShape` (#428, #1077) and of the blits that take a shape (#429)
are the port's.

## Fixed

Intended deviations from the original (`docs/libs/deviations.md`,
kind F):

* `BTBlockImage` adds image × 224 to `image_addr` as a long, as
  `TBlockImage` does; the original added it as a word (`ADD.w` to an
  address register), so images from 147 on were read before
  `image_addr` (#427; bug-orig test
  `bb_blitlib-btblockimage-word-offset`). No added work: `ADD.l`
  instead of `ADD.w`, once per call.
* `BPredatorBlit`'s B channel reads the line below with the bitmap's
  bytes per row, as the offset and the modulos of the routine do; the
  original added `SetupMapTwo`'s width (`ADD.l ebwidth-2(pc)`), so on a
  bitmap of another width B read the wrong part of the background
  (#426; bug-orig test `bb_blitlib-bpredatorblit-b-ebwidth`). The width
  is loaded once per call and kept in a register: cheaper per plane
  than the load of `ebwidth` before.
* `CreateShape` has an error check: with runtime error checking a count
  of 0 is the error "CreateShape count must be at least 1!"; the
  original has no check, and its `DBRA` loop creates 65536 shapes, 2 MB
  over the memory behind the shape table (#428; bug-orig test
  `bb_blitlib-createshape-count0`). The release code is unchanged: the
  count is an argument the programmer controls.
* `CreateShape`'s error check also refuses a count that runs past the
  shape table: `start_shape + count` above Maximum Shapes is the error
  "CreateShape start+count above Maximum Shapes!". The original writes
  the objects from Maximum Shapes on, 32 bytes each, over the memory
  behind the table (#1077; bug-orig test
  `bb_blitlib-createshape-past-max`); the compiler checks only
  `start_shape`. The check gets the table (`#ba0`) and the maximum
  (`#md4`) from the declaration, so each call loads a0 and d4 also
  without runtime error checking (24 cycles; a set-up command).
* `SpecialBlock`, `SpecialBlockBitmap`, `TBlock`, `PredatorBlit`,
  `BPredatorBlit` and `BColourBlit` have an error check, and
  `SpecialBlockToShape`'s check tests the top shape's depth: with
  runtime error checking a shape of depth 0 (one never defined) is the
  error "Uninitialized Shape". The original has no such check, and the
  plane loops (`DBRA` on depth − 1) run 65536 times over the "plane
  pointers" behind the bitmap object (#429; bug-orig test
  `bb_blitlib-depth0-dbra`). The release code is unchanged, so no work
  is added per call or plane; a bitmap of depth 0 cannot be made with
  runtime error checking (bitmaplib's checks).
* `BlockToShape` copies the whole data: one blit of height × depth
  lines of `ebwidth`/2 words. The original blitted one word per line
  (`ADDQ.w #1`), so of a shape wider than 16 pixels only the first
  2/`ebwidth` of the data, the rest of the destination kept its old
  content (#425; bug-orig test `bb_blitlib-blocktoshape-one-word`). Its
  error check also refuses a shape one blit cannot hold, more than 1024
  lines (height × depth) or wider than 64 words, with "Shape too large
  for BlockToShape!"; the original blitted the size word's low bits
  (bug-orig test `bb_blitlib-blocktoshape-too-large`). The size is
  computed before the blitter wait; the call is faster than the
  original's (perf `bbblit.blocktoshape`).

## Original behaviour that is not a defect

* `SpecialBlock`, `TBlock`, `PredatorBlit` assume 16×16 shapes and
  bitmaps of `SetupMapTwo`'s width: the map format they are written for.
* `BPredatorBlit` stores the blit size one line less in the record (so
  `UnBuffer` restores one line less than the slot holds): deliberate
  (`;!!! BB Cludge!` in the source).
* `BColourBlit` sets the B modulos it does not use (B is off); its colour
  is shifted as a byte (`LSR.b`): planes from the 9th on would get colour
  bit 0, but there are at most 8.

## Test

Each run loads one library set (the C build or the original); the
scenario writes its observations as cases, compared with the expected
data `libs/tests/expected/rwe_bb_blitlib.txt` recorded from the
original. One chip memory block with random contents: three 6-plane bitmaps (40 and 48 bytes per line, 64 lines), the
data and cookies of the shapes (16×16×6, 32×20×5, 32×12×4 with handles,
destinations), four images, the memory of a `Buffer`. Every call runs
with the blitter owned (`OwnBlitter`) and waited for (`WaitBlit`), then
the bitmaps, shape data and images are observed byte for byte, the
buffer records (pointers relative to the block, the saved data
the blit wrote), the shape objects `CreateShape` made, d0 of
`PredatorBlit`, the registers the rule keeps (`KEEPS_STATEMENT`, `PredatorBlit`
`KEEPS_FUNCTION`). All 12 routines, both forms of
`SpecialBlock`/`SpecialBlockBitmap`, all shifts, `SetupMapTwo` 48 and back
to 40, colours with bits above the depth and above bit 7, `CreateShape`
with the like shape among the created ones, the original's two error
checks with good shapes. Not covered: the runtime errors of the checks,
the port's checks (#428, #429, #1077; the original has no entries to compare
them with: the bug-orig tests `bb_blitlib-createshape-count0` and
`bb_blitlib-depth0-dbra`, `bb_blitlib-createshape-past-max` show their
runtime errors), depth/count 0,
blits outside the bitmaps.
