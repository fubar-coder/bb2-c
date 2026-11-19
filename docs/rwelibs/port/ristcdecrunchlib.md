# ristcdecrunchlib (18): STCDecrunch

Reflective Images. One command with two forms for StoneCracker 4.04
data (`S404`, the format of `ristonecrackerlib`) in memory:

* `STCDecrunch(source)` – unpacks the data at `source` in place, to
  `source` + the security offset, and returns that address (`a0` after
  the decruncher); 0 if `source` is no S404 file or its data are corrupt
  (they would write below the destination, #833).
* `STCDecrunch(source,dest)` – unpacks to `dest` (without the offset)
  and returns 1 (the decruncher leaves `d0` = 1, `MOVEQ #1,d0` for its
  `ADDX`); 0 if no S404 file or corrupt data.

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

Assembler for speed (perf `bstc`): the two entries and the decruncher
`sd_dec` of `libs/userlibs/rwe/optidec.h` (shared with
`ristonecrackerlib`, see its notes for the code it reads), the
original's source `optidec.S` v1.1 instruction by instruction with the
checks of #833. As in the original, the entries change d0–d7/a0/a1 and
keep a2–a6; form 2 runs on into the decruncher (no `BSR`). In C the
decruncher took 1.8 times the original's cycles (helpers inlined, `LSL`
as an assembler statement), so it is assembler (the libraries'
performance rule: assembler where C stays behind).

The decruncher is faster than the original on real data: the 101xx
match (4..7 bytes, the most frequent code in text) reads its 2 bits one
at a time like the 11x match, not through the getbits routine; the
result is the same bit for bit (in the main loop the bit count stays
0..15). On `test_C1.pack410` (perf `bstc.c1pack410`) 11358874 cycles
against the original's 11836874 (0.96), a call on a file of one byte
(`bstc.onebyte`) 467.0 against 504.7 (0.93).

## Fixed (kind F, docs/libs/deviations.md 3a)

* Corrupt data no longer write below the destination (#833). The
  original decruncher tests only at the top of its loop whether the
  output has reached the destination: a match or a literal run that
  starts just above it wrote its whole length past the start into the
  memory below. The checks cost nothing per token on the main path: the
  loop's one test compares with destination + 276 instead of the
  destination, so no token but a long match of more than one length
  byte (more than 277 bytes) can reach below it there, and such a match
  is checked; the last bytes go through a second copy of the loop that
  checks each literal run and match against the room left (so does a
  file whose first bit count, the trailer's third word, is above 16).
  Once per call far offset bits above 16 and an odd packed length are
  refused. Such data give 0 and leave everything outside destination ..
  destination + unpacked length untouched. Not checked, a known limit:
  reads. A stream that runs out reads words below the packed data (the
  header, then the memory below the file), and a match may read up to
  544 + 65535 bytes above the output written so far; such data unpack
  wrong, nothing is written outside the destination.

Form 2 does not know the size of `dest`; that is no defect (the caller
provides a `dest` for the unpacked length, which the header gives).

## DIFF

The run table index of the decruncher (`optidec.h`, as in
`ristonecrackerlib`): with a first bit count above 16 (the trailer's
third word) the original reads its own code as the table, the port
takes the index modulo 16 (more than 16 far offset bits, the other way
there, are refused, #833).

## Test

The S404 files of `libs/tests/rwe/s404.h` (as for `ristonecrackerlib`:
random sequences of every code, 0..16 far offset bits, first bit buffers
of 0..16 bits), 40 files up to 20000 bytes. Each run loads one library
set (the C build or the original); the scenario writes its observations
as cases, compared with the expected data
`libs/tests/expected/rwe_ristcdecrunchlib.txt` recorded from the
original. Form 1 unpacks a copy of each in a prefilled buffer in place,
form 2 from the file into another prefilled buffer at 0 or 1..199 bytes
in; observed: d0 (relative to the buffer in form 1), both whole buffers,
and as truth values that the data equal what the test's encoder started
from, d0 = start / 1, the bytes around the data.
Both forms with data that are no S404 file (0, nothing written), kept
registers. Not covered: corrupt data, a first bit count above 16 (the
DIFF), a far offset that is negative as a word. The bug-orig tests
`ristcdecrunchlib-optidec-no-checks` (a forged match of 3 bytes for 2)
and `ristcdecrunchlib-optidec-real-data` (ancient's real S404 files
`test_C1.pack410` and `test_C1.pack410_0`, from
`cmake/fetch-originals.cmake`: to another address, in place and with
`STCLoad` they give `test_C1.raw`; five corrupt copies made by the test
with 66000 guard bytes below the destination) cover the checks.
