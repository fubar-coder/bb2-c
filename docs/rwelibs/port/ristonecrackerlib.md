# ristonecrackerlib (17): StoneCracker 4.04 files

Reflective Images. Four functions for files packed with StoneCracker
4.04 (`S404`): the header is `S404`, the security offset, the unpacked
length, the packed length (counted from +12), the packed words, and
behind them three words (the number of offset bits of far matches, the
first bit buffer, its number of bits). The data unpack backwards, from
address + offset + unpacked length down to address + offset.

* `STCLength(a)` – the unpacked length, 0 if `a` is no S404 file.
* `STCStart(a)` – `a` + offset (where the data are after unpacking), 0
  if no S404 file.
* `STCSafeLen(a)` – unpacked length + offset, the buffer size for
  unpacking in place; 0 if no S404 file.
* `STCLoad f$,a` – reads the file to `a` (all of it), unpacks it in
  place and returns `STCStart`; 0 if the file cannot be locked or opened
  (e.g. a directory) or the FileInfoBlock not allocated.

init opens dos.library (any version); finit is 0.

* **Source:** `rwelibsrc/stclib.bb` (as TED shows it:
  `build/amiga/rwelibsrc/stclib.bb`) with `rwelibsrc/optidec.bin`, the
  decruncher of StoneCracker 4.04 (third party code, `IncBin`).
* **Reference:** BSS2 `RISTONECRACKERLIB.OBJ` (= AB3
  `Ristonecrackerlib.obj`), newer than the source
  (`docs/rwelibs/README.md` §3.2); declarations equal (ctest
  `rwe_declarations_ristonecrackerlib`).
* **C file:** `libs/userlibs/rwe/ristonecrackerlib.c`; test
  `libs/tests/rwe/ristonecrackerlib.c` (snitch case "rwe
  ristonecrackerlib").

## The binary's state

Checked in the disassembly: `STCSafeLen` (`$2E4`) and `STCStart`
(`$2FA`) begin with `CMPI.L #"S404",(a0):BNE oops` (10 bytes each), the
check `STCLength` already has in the source; everything else is the
source's code, shifted by the 20 bytes, `optidec.bin` unchanged. The C
build has the checks: both return 0 for other data, as the BSS2 manual
says ("If this command returns 0 then 'address' does not point to a valid
STC packed file").

## C and assembler

All C behind `BLITZ_THUNK`s (keep d1/a0/a1; the original changes d0/a0
in the header functions and d0–d7/a0/a1/a6 in STCLoad). The decruncher
is C too, translated register by register like the decrunchers of
`libs/acidlibs/crunchlib.c`: d6 is the 32-bit bit buffer (`ADD.w d6,d6`
for a bit, `LSL.l` with the count modulo 64, `ROL.w #8`, `SWAP`), d7 the
bit count as a word, `DBF` loops on the low word, the targets of the
original's `JMP (a6)` (the continuation after its getbits routine) are
labels. The code it reads (MSB first, words backwards from the trailer):

| Bits | Meaning |
|---|---|
| `0` + 8 | a literal byte |
| `11` + 1 | match of 2–3 bytes |
| `101` + 2 | match of 4–7 bytes |
| `1001` + 4 (not 15) | match of 8–22 bytes |
| `1001 1111` + 5, then the bytes | 14–45 literal bytes |
| `1000` + 8, + 8 more while 255 | match of 23 + the sum of the bytes |
| then `01` + 5 / `00` + 9 / `1` + d2 bits | distance 1–32 / 33–544 / 545 + n |

`_stcdecrunch2` of the source (unpack to another address) is in no token
entry and never called: left out.

## BUG(orig), kept

* `STCLoad`: `OpenFromLock` takes over the lock, but after `Close` the
  lock is unlocked once more (`BSR oopsfreefib`: FreeMem + UnLock) – a
  second UnLock of a lock the file system has already freed (on `RAM:` in
  the test without visible effect).
* `STCLoad` does not check the header (any file is unpacked with what its
  words say), reads the whole file whatever the buffer's size, and does
  not check Examine's and Read's results.
* dos.library is opened by init and never closed (finit is 0).
* The decruncher checks nothing: corrupt data write below the
  destination (a match or literal run past the start) and read packed
  words below the file.

## DIFF

A literal run reads the shift count of its straddling byte from a table
behind the code, indexed by the bit count d7 (`MOVE.b $44C(pc,d7.w)`).
d7 leaves 0..15 only with a header of more than 16 far offset bits; the
original then reads whatever lies around the table (its own code), the C
build takes the index modulo 16. Files with 0..16 offset bits unpack the
same.

## Test

The test makes its own S404 files: a random sequence of all the codes
above (literal bytes, literal runs, all match lengths including the 255
continuation, near/middle/far distances with 0..16 offset bits, first bit
buffers of 0..16 bits) is written as the bit stream, and the data it
stands for is worked out at the same time. 40 files (up to 20000 bytes,
code subsets and all codes) are written to `RAM:` and loaded with
`STCLoad` into a buffer prefilled per case; compared with the original:
d0, the whole buffer, and on each set the expected data, the start and
the header; the three header functions on each file and on other data
(0). Also a file that is no S404 file (unpacked anyway), a missing file
and a directory (OpenFromLock fails), kept registers. Not covered:
corrupt data (writes anywhere), more than 16 offset bits (the DIFF), a
16-bit far offset of $8000 or more (negative as a word: reads before the
destination).
