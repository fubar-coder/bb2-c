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
  destination has data and the same size).
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
  (ctest `rwe_declarations_bb_blitlib`). Number 19 (the literal of the
  source, = `#bb_blitlib`).
* **C file:** `libs/userlibs/rwe/bb_blitlib.c`; test
  `libs/tests/rwe/bb_blitlib.c` (snitch case "rwe bb_blitlib").

## C and assembler

All C behind `BLITZ_THUNK`s that keep d0/d1/a0/a1 (`PredatorBlit`:
d1/a0/a1); the original changes up to d0–d7/a0–a3. The blitter is
programmed with volatile register writes (`BL_CUSTOMW`/`BL_CUSTOML`,
`BL_BLITWAIT`), the same registers in the same order, absolute or
through the chip base in a1 where the original uses either. Pointers
that the original does not reload per plane (B and C of `SpecialBlock`,
B of `SpecialBlockBitmap`, `BTBlockImage`) go on where the blitter left
them, as in the original. The record of the buffered blits is written by
the CPU like the original's (the save blit writes into it). The address
arithmetic is the original's (`MULU.w`/`MULS.w`, `ASR.w`, word sums).
`ADD.l ebwidth-2(pc)` adds the long `pad:ebwidth`, i.e. `ebwidth` (pad
is 0). The error checks are C in the error check area; they keep d0,
which the original changes (low word: +26 of a shape).

## BUG(orig), kept

* Nothing is checked but the two error checks; shapes or bitmaps of
  depth 0 loop 65536 times (DBRA) (#429).
* `BlockToShape` copies one word per line (height × depth words) (#425).
* `BPredatorBlit`'s B channel reads the line `SetupMapTwo`'s width
  further, not the bitmap's (#426).
* `BTBlockImage` adds image × 224 as a word (from image 147 on before
  `image_addr`) (#427).
* `CreateShape` with count 0 creates 65536 shapes (#428).

Original behaviour that is not a defect:

* `SpecialBlock`, `TBlock`, `PredatorBlit` assume 16×16 shapes and
  bitmaps of `SetupMapTwo`'s width: the map format they are written for.
* `BPredatorBlit` stores the blit size one line less in the record (so
  `UnBuffer` restores one line less than the slot holds): deliberate
  (`;!!! BB Cludge!` in the source).
* `BColourBlit` sets the B modulos it does not use (B is off); its colour
  is shifted as a byte (`LSR.b`): planes from the 9th on would get colour
  bit 0, but there are at most 8.

## Test

One chip memory block per set with the same contents at the same
offsets: three 6-plane bitmaps (40 and 48 bytes per line, 64 lines), the
data and cookies of the shapes (16×16×6, 32×20×5, 32×12×4 with handles,
destinations), four images, the memory of a `Buffer`. Every call runs
with the blitter owned (`OwnBlitter`) and waited for (`WaitBlit`), then
the bitmaps, shape data and images of both sets are compared byte for
byte, the buffer records (pointers relative to the block, the saved data
the blit wrote), the shape objects `CreateShape` made, d0 of
`PredatorBlit`, the kept registers. All 12 routines, both forms of
`SpecialBlock`/`SpecialBlockBitmap`, all shifts, `SetupMapTwo` 48 and back
to 40, colours with bits above the depth and above bit 7, `CreateShape`
with the like shape among the created ones, the error checks with good
shapes. Not covered: the runtime errors of the checks, depth/count 0,
blits outside the bitmaps.
