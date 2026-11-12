# risoundlib (45): CludgeSound

Reflective Images. One command: `CludgeSound sound#,address` makes an
audiolib sound object from an IFF 8SVX file that is already in memory
(`!libs #audiolib,#ia0|#pd0`: a0 = the object of sound#). It frees the
object's old sample (memlib `globfree`), checks `8SVX` at +8 and `VHDR`
at +12, takes the sample rate, looks for the `BODY` chunk among the first
seven chunks, copies the sample into chip memory (memlib `globalloc`,
`MEMF_CHIP`) and fills the object: +0 the sample, +4 the period
3579545 / rate, +6 the length in words, +8/+12/+14 = 0 (no loop). The
result is -1, or 0 if it is no 8SVX file with a VHDR, has no BODY among
the first seven chunks or the memory is missing. init and finit do
nothing.

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

## BUG(orig), kept

* The old sample is freed before the file is checked, and the pointer
  stays in the object: a CludgeSound that fails leaves the object
  pointing to freed memory, and the next CludgeSound on it frees it again
  (memlib's "not found" error) (#831).
* The old sample is freed without looking at audiolib's `"cL"` mark
  (+14): after a DecodeSound of an 8SVX file in chip memory the sample is
  the file's own BODY, which memlib never allocated, and globfree raises
  "Unable to Free Memory" (TRAP #0) (#945; test
  `tests/bug-orig/rwe/risoundlib-cludgesound-frees-decoded`).
* The rate is the word at VHDR + the VHDR length (`ADD.l 4(a0),a0`): it is
  `samplesPerSec` only for the usual length 20 (#832). Rate 0 never
  returns (the subtraction loop) (#830).
* The chunk walk adds length + 8 without the pad byte of an odd chunk: a
  correct file with an odd chunk before BODY is not found (or walks off
  into its data); a file without the pad byte works (#828).
* The copy is `DBRA` on length − 1: ((length − 1) & $FFFF) + 1 bytes – an
  empty BODY copies 65536 bytes into a block of 8, a BODY over 65536
  bytes only the rest; the length in words is (length & $FFFF) / 2
  (#829).

## Test

Each run loads one library set (the C build or the original); the
scenario writes its observations as cases, compared with the expected
data `libs/tests/expected/rwe_risoundlib.txt` recorded from the
original. 8SVX files are built in memory; no audio. Observed: d0, the
object's fields and guards, the sample's bytes (the pointer itself is
only checked to be in chip memory on success and the old pointer on
failure), the file unchanged, the kept registers, memlib's memory all
back. Covered: nine rates (1, 2, 0x8000, 0xFFFF …), each call
freeing the previous sample; chunks before BODY, BODY as the 7th (found)
and 8th chunk (not), no `8SVX`, no `VHDR`, a VHDR of 24 bytes, an odd
chunk with and without the pad byte, odd and 1-byte BODYs, BODYs of 65536
and 70000 bytes, the stale pointer after a failure. Not covered: rate 0
and an empty BODY (see above); audiolib itself (only the object layout).
