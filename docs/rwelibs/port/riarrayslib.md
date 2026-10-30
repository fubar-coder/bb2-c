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
original instructions; the C build's code equals the reference build's
byte for byte. init and finit are empty C functions.

## BUG(orig), kept

* The loops store first and compare after (`CMP.l a0,a1:BNE`): an end
  that is not reached exactly – an array size that is not a multiple of
  the element size, or no element at all – runs on through memory.

## Test

The a2 stack is built in memory (type word, end address). Both inline
routines are checked to be inline with the same length and their code
is called as a subroutine. Compared: the array and the bytes around it,
d0–d2/a0–a2 afterwards (addresses relative to the buffers), the kept
registers. Covered: byte, word and long arrays of 1–37 elements, odd
start addresses for bytes, values with garbage above the element size,
both forms, a type word with garbage in its upper byte, the types that
do nothing (0, 4, 5, 7, 8, byte/word/long only in the upper byte), init,
finit. Not covered: an end that is not reached exactly (the bug).
