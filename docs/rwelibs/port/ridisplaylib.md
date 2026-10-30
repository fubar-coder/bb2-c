# ridisplaylib (29): InitView, ViewBitmap

Reflective Images ("RI DisplayLib"). The start of a display library
that was to build its own copper list; two commands:

* `InitView(height,depth,numsprites,numcolours)` – allocates a copper
  list in chip memory, writes its first four moves and returns its
  address.
* `ViewBitmap(bitmap)` – no code (`RTS`, d0 unchanged).

init opens graphics.library 39 (the base is not used otherwise), finit
closes it and frees the last list. The list is never installed: the
library does not touch the display.

* **Source:** `rwelibsrc/mydisplaylib.bb` (as TED shows it:
  `build/amiga/rwelibsrc/mydisplaylib.bb`).
* **Reference:** the reference build of the source by blitz2 2.10
  (`refbuild`, `build/amiga/libs/userlibs/rwe/ref/ridisplaylib.obj`); no
  binary of the library exists (`docs/rwelibs/README.md` §2).
  Declarations equal (ctest `rwe_declarations_ridisplaylib`).
* **C file:** `libs/userlibs/rwe/ridisplaylib.c`; test
  `libs/tests/rwe/ridisplaylib.c` (snitch case "rwe ridisplaylib").

## C and assembler

All C behind `BLITZ_THUNK`s (keep d1/a0/a1, and d0 for init, finit and
ViewBitmap; the original changes d0/d1/a0/a1/a6 in init and finit,
d0–d6/a0/a1/a6 in InitView). The variables `viewsize` and `cop` are a
struct in the original's order (the test finds them by the address
InitView returns). The words of the copper list are read from the
absolute addresses the original reads (below), as volatile words.

## BUG(orig), kept

* InitView loses the colour count (`MOVE.w d0,d3` overwrites it with
  the height before it is read): the size is 16 × numsprites + 16 ×
  depth + 8 × height + 38, computed as a word and sign-extended.
* `!cmove{diwstrt,$2c81}` assembles to `MOVE.w diwstrt,(a0)+` /
  `MOVE.w $2c81,(a0)+`: the register name is its offset (`diwstrt` =
  `$8E`) and the value has no `#`, so both are read as absolute
  addresses: the list gets the words at `$8E`, `$2C81`, `$90`, `$2CC1`, `$92`,
  `$38`, `$94`, `$D0` – low memory and the vector table, two of them at
  odd addresses (an address error on a 68000).
* AllocMem is not checked: without chip memory the eight words go to
  address 0 (over ExecBase at 4).
* Every InitView allocates a new list and forgets the old one; finit
  frees only the last, and only if graphics.library was opened
  (`FreeMem(0,size)` if the last allocation failed).

## Test

Compared after every call: d0 (InitView: as "is the list the library
keeps"), `viewsize` and `cop`, the list's eight words (each equal to the
word at its address, BUG(orig), and equal between the sets for the
even ones), the change of graphics.library's open count, the test task's
memory balance (rwemem.h), kept registers. Cases: init, five InitViews (marker upper
words, colour counts that change nothing), ViewBitmap, finit with a
list, finit again (nothing), init and finit without a list. The lists
InitView leaks are freed by the test. Not covered: InitView without
chip memory or with a size that is negative as a word (writes to
address 0).
