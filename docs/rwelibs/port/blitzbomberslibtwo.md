# blitzbomberslibtwo (31): Blitz Bombers 2 space invaders

Reflective Images. Four drawing routines of the space invaders game of
Blitz Bombers 2, plain CPU memory accesses with fixed sizes (sprite
images of 16-byte rows, a 320 pixel wide bitmap, a 236-line playfield):

* `KillInvader sprad,x,y,offset` – clears an invader (8 rows: the words
  at +0 and +8 of each 16-byte row) in four sprite images: the images are
  the longs at `sprad + (x>>2)*16` and 48 bytes further, the rows start
  at `(x&3)*2 + y*224` in them and `offset` behind that.
* `DrawStarField table,bitplane` – a table of stars (words speed, bit,
  byte offset; a negative speed ends it): each star's bit is cleared at
  its offset, the offset advanced by the speed (from line 236 on it wraps
  back by 204 lines) and the bit set there.
* `DrawBullet bitm,x,y` / `EraseBullet bitm,x,y` – a 1×3 pixel bullet in
  planes 0–3 of a bitmap object (40 bytes per row): colour 4 / colour 0.

No init, no finit (`!libheader {#blitzbomberslibtwo,0,0,0,0}`).

* **Source:** `rwelibsrc/blitzbomberslib/blitzbombersTwo00.bb` (as TED
  shows it: `build/amiga/rwelibsrc/blitzbomberslib/blitzbombersTwo00.bb`).
* **Reference:** the reference build of the source by blitz2 2.10
  (`refbuild`, `build/amiga/libs/userlibs/rwe/ref/blitzbomberslibtwo.obj`);
  no binary exists (`docs/rwelibs/README.md` §3.3). Declarations equal
  (ctest `rwe_declarations_blitzbomberslibtwo`). Number 31 (ours, RWE's
  `RESERVEDLIB_8`, next to `#blitzbomberslib` 30).
* **C file:** `libs/userlibs/rwe/blitzbomberslibtwo.c`; test
  `libs/tests/rwe/blitzbomberslibtwo.c` (snitch case "rwe
  blitzbomberslibtwo").

## C and assembler

All C behind `BLITZ_THUNK`s that keep d0/d1/a0/a1 (the original changes
d0–d3/a0–a3; a3 is the string pointer of the Blitz program, which the C
build keeps). The address arithmetic is the original's: word results
(`LSR.w`/`LSL.w`, `ADD.w`), word offsets added with their sign
(`ADDA.w`, `x(an,dn.w)`), `MULU.w`.

## Original behaviour, not defects

* Nothing is checked; the sizes are constants of the game.
* `DrawStarField` takes the bit numbers modulo 8 (`BCLR`/`BSET` on
  memory address a byte): the table gives the bit within the star's
  byte.

## Test

Each run loads one library set (the C build or the original); the
scenario writes its observations as cases, compared with the expected
data `libs/tests/expected/rwe_blitzbomberslibtwo.txt` recorded from the
original. `KillInvader` 40
times with x 0..63, y 0..20 and offsets −256..511 on random images
behind a table of pointers (the pointers relative to the
memory); `DrawStarField` over 60 frames with 50 stars (speeds 0..240,
bits 0..15) that wrap; `DrawBullet`/`EraseBullet` 100 times anywhere on
four 320×256 planes. After every call the whole memory is
observed, and the kept registers. Not covered: offsets that are negative
as words.
