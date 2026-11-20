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
* `STCLoad f$,a` – reads the file to `a` (all of it, whatever the
  buffer's size: the command has no size argument), unpacks it in place
  and returns `STCStart`; 0 if the file cannot be locked, examined,
  opened (e.g. a directory) or read whole, the FileInfoBlock not
  allocated, or the file is no S404 file whose packed data and trailer
  lie within it.

init opens dos.library (any version); finit closes it (the original's finit
is 0, #834).

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
in the header functions and d0–d7/a0/a1/a6 in STCLoad), except the
decruncher: it is assembler for speed (`sd_dec` in
`libs/userlibs/rwe/optidec.h`, shared with `ristcdecrunchlib`; perf
`bstc`), the original's source `optidec.S` v1.1 instruction by
instruction with the checks of #833, and `STCLoad`'s C calls it with
`JSR` (it keeps a2–a6 itself). d6 is the 32-bit bit buffer (`ADD.w
d6,d6` for a bit, `LSL.l`, `ROL.w #8`, `SWAP`), d7 the bit count as a
word, `DBF` loops on the low word, the getbits routine returns by `JMP
(a6)`. The code it reads (MSB first, words backwards from the trailer):

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


## Fixed (kind F, docs/libs/deviations.md 3a)

* finit closes dos.library. The original's finit is 0: dos.library was
  never closed, its open count one higher after every program run
  (#834).
* `STCLoad` unlocks no lock a file handle owns. The original opened the
  file with `OpenFromLock`, which takes the lock over, and unlocked it once
  more after `Close` (`BSR oopsfreefib`: FreeMem + UnLock), an UnLock of a
  lock the file system had already freed (on `RAM:` without visible
  effect; the test counts the UnLock calls of a lock a file handle owns:
  1 in the original, 0 now) (#835).
* `STCLoad` opens the file by its name (`Open`, `MODE_OLDFILE`) while it
  holds the lock of `Lock`/`Examine`, and unlocks that lock after the
  open: one path on every Kickstart. The original called `OpenFromLock`,
  a dos.library V36 function, on the dos.library it opened in any
  version; on Kickstart 1.x the offset lies behind the jump table and the
  program crashed at `STCLoad` (#1147; test
  `ristonecrackerlib-v36-calls` on the a500, a500-204 and a1200).
* `STCLoad` unpacks only if `Examine` succeeded, `Read` read the whole
  file, and the file is an S404 file whose packed data and trailer (12 +
  packed length + 6 bytes) lie within it; otherwise it returns 0, as the
  manual says ("If this command returns 0 then the file could not be
  loaded properly or was not a valid packed file"). The original checked
  none of it and unpacked whatever it had read with the offsets and
  lengths its words gave, returning the address (#836).
* The decruncher (`optidec.h`, shared with `ristcdecrunchlib`) no longer
  writes corrupt data below the destination; `STCLoad` returns 0 for
  them. The original wrote a match or literal run that starts just above
  the destination past its start. The checks cost nothing per token on
  the main path; reads are not checked (a known limit). Details:
  `docs/rwelibs/port/ristcdecrunchlib.md` (#833).

## DIFF

A literal run reads the shift count of its straddling byte from a table
behind the code, indexed by the bit count d7 (`MOVE.b $44C(pc,d7.w)`).
d7 leaves 0..15 only with a first bit count above 16 (the trailer's third
word; more than 16 far offset bits, the other way there, are refused,
#833); the original then reads whatever lies around the table (its own
code), the port takes the index modulo 16. Other files unpack the
same.

## Test

The test makes its own S404 files (`libs/tests/rwe/s404.h`, shared with
the test of `ristcdecrunchlib`): a random sequence of all the codes
above (literal bytes, literal runs, all match lengths including the 255
continuation, near/middle/far distances with 0..16 offset bits, first bit
buffers of 0..16 bits) is written as the bit stream, and the data it
stands for is worked out at the same time. 40 files (up to 20000 bytes,
code subsets and all codes) are written to `RAM:` and loaded with
`STCLoad` into a buffer prefilled per case. Each run loads one library
set (the C build or the original); the scenario writes its observations
as cases, compared with the expected data
`libs/tests/expected/rwe_ristonecrackerlib.txt` recorded from the
original: d0, the whole buffer, and as truth values that the data equal
what the test worked out, the start and the header; the three header
functions on each file and on other data
(0). Also a file that is no S404 file (the original unpacks it anyway, the C
build returns 0), a missing file
and a directory (the open fails), the number of UnLock calls of one
STCLoad that unlock a lock a file handle owns (dos.library's OpenFromLock
and UnLock patched: the lock an OpenFromLock took over is noted, the
UnLock calls of it counted), kept registers. Not covered:
corrupt data (the bug-orig tests of `ristcdecrunchlib` cover the shared
decruncher), a first bit count above 16 (the DIFF), a
16-bit far offset of $8000 or more (negative as a word: reads before the
destination).
