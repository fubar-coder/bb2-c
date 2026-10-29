# ristcdecrunchlib (18): STCDecrunch

Reflective Images. One command with two forms for StoneCracker 4.04
data (`S404`, the format of `ristonecrackerlib`) in memory:

* `STCDecrunch(source)` – unpacks the data at `source` in place, to
  `source` + the security offset, and returns that address (`a0` after
  the decruncher); 0 if `source` is no S404 file.
* `STCDecrunch(source,dest)` – unpacks to `dest` (without the offset)
  and returns 1 (the decruncher leaves `d0` = 1, `MOVEQ #1,d0` for its
  `ADDX`); 0 if no S404 file.

init and finit are `!nullsub{0,0,0}`.

* **Source:** `rwelibsrc/stcdecrunchlib.bb` (as TED shows it:
  `build/amiga/rwelibsrc/stcdecrunchlib.bb`) with `rwelibsrc/optidec.bin`,
  the decruncher of StoneCracker 4.04 (third party code, `IncBin`).
* **Reference:** the reference build of the source by blitz2 2.10
  (`refbuild`, `build/amiga/libs/userlibs/rwe/ref/ristcdecrunchlib.obj`):
  the source is newer than the BSS2 binary `RISTCDECRUNCHLIB.OBJ`
  (`docs/rwelibs/README.md` §3.2). Declarations equal (ctest
  `rwe_declarations_ristcdecrunchlib`).
* **C file:** `libs/userlibs/rwe/ristcdecrunchlib.c`; test
  `libs/tests/rwe/ristcdecrunchlib.c` (snitch case "rwe
  ristcdecrunchlib").

## Source and binary

The BSS2 binary has the routines of `stclib`: form 1 returns the
unpacked length, form 2 does not check the header. The source (the
state ported) checks the header in both forms and returns the address
of the data in form 1. Programs written for the BSS2 binary that use the
result of form 1 as a length do not work with this library.

## C and assembler

All C behind two `BLITZ_THUNK`s (keep d1/a0/a1; the original changes
d0–d7/a0/a1 and keeps a2–a6). The decruncher is `dostc` of
`libs/userlibs/rwe/optidec.h`, the translation shared with
`ristonecrackerlib` (see its notes for the code it reads).

## BUG(orig), kept

* The decruncher checks nothing: corrupt data write below the
  destination and read packed words below the file; form 2 does not know
  the size of `dest`.

## DIFF

The run table index of the decruncher (`optidec.h`, as in
`ristonecrackerlib`): with more than 16 far offset bits the original
reads its own code as the table, the C build takes the index modulo 16.

## Test

The S404 files of `libs/tests/rwe/s404.h` (as for `ristonecrackerlib`:
random sequences of every code, 0..16 far offset bits, first bit buffers
of 0..16 bits), 40 files up to 20000 bytes. Form 1 unpacks a copy of
each in a prefilled buffer in place, form 2 from the file into another
prefilled buffer at 0 or 1..199 bytes in; compared with the reference
build: d0 (relative to the buffer in form 1), both whole buffers, and on
each set the expected data, d0 = start / 1, the bytes around the data.
Both forms with data that are no S404 file (0, nothing written), kept
registers. Not covered: corrupt data, more than 16 far offset bits (the
DIFF), a far offset that is negative as a word.
