# riencryptlib (52): Encrypt, Decrypt, GetWheel

Reflective Images (Stephen McNamara), 1994/96. A simple cipher with three
byte "wheels": `Encrypt memadr,length[,wheel1,wheel2,wheel3]` adds the
three wheels to every byte, stepping them like an odometer before each
byte; `Decrypt memadr,length,wheel1,wheel2,wheel3[,mode]` undoes it from
the end; `GetWheel(n)` returns the state of wheel 1..3 after the last
call (-1 for other numbers). Without wheels, Encrypt takes random ones
(vallib `Rnd`); `mode = 1` makes Decrypt take the start wheels and step
them forward by the length itself.

* **Source:** `rwelibsrc/Encryptlib/Encrypt-Library.bb` (as TED shows it:
  `build/amiga/rwelibsrc/Encryptlib/Encrypt-Library.bb`).
* **Reference:** BSS2 `RIENCRYPTLIB.OBJ`; declarations equal (ctest
  `rwe_declarations_riencryptlib`).
* **C file:** `libs/userlibs/rwe/riencryptlib.c`; test
  `libs/tests/rwe/riencryptlib.c` (snitch case "rwe riencryptlib").

## C and assembler

All routines are C behind `BLITZ_THUNK`; no other assembler. `Rnd` is
called with `BLITZ_ALIBJSR` (token `#vallib<<7 + 7`, its first form). The
loop counters are unsigned with a sign test, to keep the original's
`SUBQ.l #1` / `BPL` counting without signed overflow. Like the original,
Encrypt and Decrypt return with d0's low byte = the last byte written
(upper bytes = memadr; memadr itself when nothing is written); the
thunks keep a1 (GetWheel also d1), which the original changes in
places – more than the original, never less.

## Fixed

Original bugs fixed in the port (`docs/libs/deviations.md`, 3a,
kind F):

* The loops counted with `SUBQ.l #1` before a `BPL` loop whose first pass
  is unconditional: a length of 0 processed one byte (Decrypt the byte at
  memadr-1, in front of the block), and so did a length of 2^31 and
  above. Such a length now does nothing; the wheels GetWheel returns stay
  as they were (#630).
* Decrypt mode 1 added the length's bytes 0..2 to the wheels separately,
  without the carries between them: whenever wheel 1 + the low byte of the
  length overflowed (or wheel 2 + the next byte), the end state was one
  step off and the data was not decrypted. The length is added to the
  wheels as the 24-bit number wheel3:wheel2:wheel1, the odometer Encrypt
  counts (#629). The help text documents Decrypt with five arguments;
  `mode` is only in the declaration.

Not a defect (kept): `ALibJsr Rnd` calls the first form of `Rnd`
(without a range, Quick 0..1): the ranges 256, 65535 and -1 that Encrypt
loads into d0 have no effect; each wheel is the low byte of the second of
two calls - random bytes either way.

## Test

Each run loads one library set (the C build or the original); the
scenario writes its observations as cases, compared with the expected
data `libs/tests/expected/rwe_riencryptlib.txt` recorded from the
original. Every form: Encrypt with wheels, Decrypt with five and six
arguments (mode 0, 1, $101, 2), GetWheel 0..4, $101 and a long with upper
bits; lengths 0 (`[orig]`/`[fixed #630]` pairs), 1, 5, 7, 300, 700
with wheels that carry into wheel 2 and 3 and mode 1 cases that overflow (`[orig]`/`[fixed #629]` pairs); buffer with guards, d0, wheels,
kept registers. Encrypt without wheels takes its wheels from vallib's
`Rnd`, which is hardware noise without a seed; vallib is not loaded and
its `Rnd` is a stub (`sysload_stubs`) that answers a fixed sequence, so
the random wheels and what follows are defined and observed: the wheels
from GetWheel, and Decrypt with them restores the data.
