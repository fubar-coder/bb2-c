# risoundlib (45): CludgeSound

Reflective Images. One command: `CludgeSound sound#,address` makes an
audiolib sound object from an IFF 8SVX file that is already in memory
(`!libs #audiolib,#ia0|#pd0`: a0 = the object of sound#). It checks
`8SVX` at +8 and `VHDR` at +12, takes the sample rate, looks for the
`BODY` chunk among the first seven chunks of the FORM, frees the
object's old sample (memlib `globfree`), copies the sample into chip memory (memlib
`globalloc`, `MEMF_CHIP`) and fills the object: +0 the sample, +4 the
period 3579545 / rate, +6 the length in words, +8/+12/+14 = 0 (no loop).
The result is -1, or 0 if it is no 8SVX file with a VHDR, its rate is 0,
it has no BODY among the first seven chunks of the FORM or the memory is
missing. init and finit do nothing.

* **Source:** `rwelibsrc/soundlib/risoundlib.bb` (as TED shows it:
  `build/amiga/rwelibsrc/soundlib/risoundlib.bb`).
* **Reference:** BSS2 `RISOUNDLIB.OBJ` (= AB3 `Risoundlib.obj`), newer
  than the source (`docs/rwelibs/README.md` §3.2); declarations equal
  (ctest `rwe_declarations_risoundlib`).
* **C file:** `libs/userlibs/rwe/risoundlib.c`; test
  `libs/tests/rwe/risoundlib.c` (snitch case "rwe risoundlib").

## The binary's state

The binary is the source plus two variables `sloop.l` and `srep.w`
behind `sdata` (6 zero bytes, the first step of `risoundlib2.bb`'s loop
support). Checked in the disassembly: the code is the source's
instruction for instruction (the variables at hunk `$1D2`–`$1E9`, the
trailing `MOVEQ #0,d0:RTS` at `$1EA`); nothing refers to the 6 bytes
(no relocation, no address). The variables of the original (`saddr`,
`sper`, `slen`, `sram`, `sdata`) are written before they are read in
every call, so the C build keeps them in locals; there is no data block
whose layout could matter.

## C and assembler

All C behind one `BLITZ_THUNK` (keeps d1/a0/a1; the original changes
d0–d2/a0/a1 and what memlib changes, on success a0 is the object again).
init/finit are empty C functions. memlib is called with `BLITZ_ALIBJSR`
(`TOK_globfree` with d0.w = the old length in words over sound#, as the
original; `TOK_globalloc`). The period is the original's subtraction loop
with its signed compare.

## Fixed

Intended deviations from the original (`docs/libs/deviations.md`,
kind F):

* The old sample is not freed when audiolib's `"cL"` mark (+14) is set.
  After a DecodeSound of an 8SVX file in chip memory the sample is the
  file's own BODY, which memlib never allocated; the original passed it
  to globfree, which raised "Unable to Free Memory" (TRAP #0) (#945; test
  `tests/bug-orig/rwe/risoundlib-cludgesound-frees-decoded`).
* The rate is `samplesPerSec`, the word at offset 12 of the VHDR data.
  The original took the word at VHDR + the VHDR length (`ADD.l
  4(a0),a0`), which is that word only for the usual length 20 (#832;
  test `tests/bug-orig/rwe/risoundlib-vhdr-rate-offset`).
* A rate of 0 gives the result 0; the original's subtraction loop for
  the period never ended (#830; tests
  `tests/bug-orig/rwe/risoundlib-rate-zero-hangs`,
  `risoundlib2-rate-zero-hangs`).
* The chunk walk skips the IFF pad byte behind a chunk of odd length, and
  a chunk that does not fit into the FORM (by the FORM's length) ends it
  with the result 0. The original added length + 8: behind an odd chunk
  it read id and length one byte off, missed BODY of a correct file or
  walked off into the file's data or past it; a file without the pad
  bytes (not valid IFF) worked, and no longer loads (#828; test
  `tests/bug-orig/rwe/risoundlib-chunk-walk-no-pad`).
* The old sample is freed after all checks of the file, and the
  object's pointer is cleared with it: a CludgeSound that fails on the
  file leaves the sound as it was (one that finds no chip memory leaves
  it without a sample). The original freed it first and kept the
  pointer: the object pointed to freed memory, and the next CludgeSound
  on it freed it again (memlib's "Unable to Free Memory") (#831; test
  `tests/bug-orig/rwe/risoundlib-stale-sample-pointer`).
* The whole BODY is copied, and the length in words is length / 2. An
  empty BODY and one of 131072 bytes or more (more than the word at +6
  describes; audiolib's `LoadSound` and `DecodeSound` refuse those as
  "Sound too long") give the result 0. The original copied with `DBRA`
  on length − 1, ((length − 1) & $FFFF) + 1 bytes – an empty BODY 65536
  bytes into a block of 8, a BODY over 65536 bytes only the rest – and
  took (length & $FFFF) / 2 as the length in words (#829; test
  `tests/bug-orig/rwe/risoundlib-copy-length-word`).

## Test

Each run loads one library set (the C build or the original); the
scenario writes its observations as cases, compared with the expected
data `libs/tests/expected/rwe_risoundlib.txt` recorded from the
original. The cases are in `libs/tests/rwe/risoundcludge.h`, which
risoundlib2's test runs as well (the same routine). 8SVX files are built
in memory; no audio. Observed: d0, the object's fields and guards, all
of the sample's bytes (the pointer itself is only checked to be in chip
memory on success and the old pointer on failure), what the call did to
the task's memory balance (the old sample freed, the new one
allocated), the file unchanged, the kept registers, memlib's memory all
back. Covered: nine rates (1, 2, 0x8000, 0xFFFF …), each call
freeing the previous sample; chunks before BODY, BODY as the 7th (found)
and 8th chunk (not), no `8SVX`, no `VHDR`, a VHDR of 24 bytes, an odd
chunk with and without the pad byte, odd and 1-byte BODYs, BODYs of
65536, 70000, 131071 and 131072 bytes, the old sample freed by a failure;
in the C build only: rate 0 (the original never returns) and an empty
BODY (the original overwrites memory). Not covered: audiolib itself
(only the object layout).
