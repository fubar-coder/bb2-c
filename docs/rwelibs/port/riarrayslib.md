# riarrayslib (7): FillArray

Reflective Images, 1994. One statement, `FillArray array(),value` /
`FillArray array()`: every element of a byte, word or long array is set
to the value (the second form: 0). The array argument is
`#unknown|#array|#arrayend`: d0 = the first element; the compiler's a2
stack holds the type word and, on top of it, the address behind the last
element. The code pops both (a2 − 6) and compares only the low byte of
the type: 1 byte, 2 word, 3 long; any other type (quick, float, string
…) does nothing. init and finit do nothing.

* **Source:** `rwelibsrc/ArraysLib.bb` (as TED shows it:
  `build/amiga/rwelibsrc/ArraysLib.bb`).
* **Reference:** the reference build of the source by blitz2 2.10
  (`refbuild`, `build/amiga/libs/userlibs/rwe/ref/riarrayslib.obj`); no
  binary exists (`docs/rwelibs/README.md` §3.3). Declarations equal
  (ctest `rwe_declarations_riarrayslib`).
* **C file:** `libs/userlibs/rwe/riarrayslib.c`; test
  `libs/tests/rwe/riarrayslib.c` (snitch case "rwe riarrayslib").

## C and assembler

Both forms are inline routines (`Dc $a000,len`: the compiler copies the
code to the call), so they stay assembler (`BLITZ_INLINE`), with the
original instructions and one compare more (#882, below); otherwise the
C build's code equals the reference build's byte for byte. init and
finit are empty C functions.

## Fixed

Original bugs fixed in the port (`docs/libs/deviations.md`, 3a,
kind F):

* The loops store first and compare after (`CMP.l a0,a1:BNE`): an array
  without elements (the end at the start) ran on through memory. One
  `CMP.l a0,a1:BLS` before the type dispatch does nothing for an end at
  or below the start: 4 bytes and one compare per call, nothing per
  element (the loops are unchanged). An array size that is not a
  multiple of the element size cannot occur (only byte, word and long
  arrays are filled) and is not tested (#882).

## Test

Each run loads one library set (the C build or the original); the
scenario writes its observations as cases, compared with the expected
data `libs/tests/expected/rwe_riarrayslib.txt` recorded from the
original. The a2 stack is built in memory (type word, end address). Both
inline routines are observed to be inline, with their length, and their
code is called as a subroutine. Observed: the array and the bytes around
it, d0–d2/a0–a2 afterwards (addresses relative to the buffers), the kept
registers. Covered: byte, word and long arrays of 1–37 elements, odd
start addresses for bytes, values with garbage above the element size,
both forms, a type word with garbage in its upper byte, the types that
do nothing (0, 4, 5, 7, 8, byte/word/long only in the upper byte), init,
finit; the inline lengths are `[orig]`/`[fixed #882]` pairs. Not
covered: an array without elements (the original runs through memory;
the bug-orig test `riarrayslib-end-not-reached` shows it).
