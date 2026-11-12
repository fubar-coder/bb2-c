# ricopperfxlib (CopperFX, library 11)

**What it does:** custom copper lists for the Blitz mode display
library (displaylib 143). `CopperReset coplist#,startline[,ccoffset]`
takes the custom copper space of a coplist (the long at coplist+28),
then `DoColSplit` writes an AGA colour gradient (24 bit, one colour
register, per line a WAIT, BPLCON3 bank/high nibbles, colour, BPLCON3
low nibbles, colour), `RedoColSplit` rewrites the colours of such a
split in place, `CopperMove`/`CopperWait`/`CopperSkip`/`CopperCommand`/
`CopperAGACol`/`CopperEnd` append single instructions. `CopperResetAdr`
takes a custom space address directly, `CopperInfoBlock` returns the
address of the state block, `GetCCOffset` the current offset in
instructions, `ColSplitBplcon3` the low byte of BPLCON3 for the splits
(border blanking).

**Source and reference:** `rwelibsrc/Copperlib/CopperFX3.bb`
(Reflective Images, 8/3/95), reference BSS2 `RICOPPERFXLIB.OBJ`
(1856 bytes) = the blitz2 2.10 build of the source. The C build has the
same declarations (`rwe_declarations_ricopperfxlib`); the code is not
byte-identical.

**C:** everything (`libs/userlibs/rwe/ricopperfxlib.c`). The routines
only write copper instructions into memory, nothing touches the custom
chips, so no assembler is needed. The word arithmetic of the original
(`ASR.w`, `ROL.b`, `ADD.w`, `DBRA` on the low word, `ADDQ.b` of the line
counter) is kept with `UWORD`/`UBYTE` operands, `DIVS`/`MULS` through
`bl_divs_w`/`bl_muls_w` (a divisor of 0 raises the divide-by-zero
exception like the original). The state block keeps the original's
layout (20 bytes; `CopperInfoBlock` hands out its address), including
`lo_bplcon3` as the first byte of a word (`MOVE.b d0,lo_bplcon3` on a
`Dc.w`). Every entry is a thunk that keeps those of d0/d1/a0/a1 that are
not the result (the original keeps a1 nearly always and d0/d1 in several
statements); the original's changes of d2/d6/d7/a0/a2 are not
reproduced.

**BUG(orig)** (kept):

* `GetCCOffset` tests the coplist pointer, not the custom space: after
  `CopperResetAdr` (coplist 0) it returns -1 although the commands work (#593).
* `DoColSplit` is a command with a word result, but d0 is the red step
  at the end (its `cols_adr` argument without a custom space);
  `CopperAGACol` (long result) returns 1234 without custom space,
  otherwise its `col#` argument (#592, #590).
* `DoColSplit`/`RedoColSplit` do not check numlines (0: DIVS by zero;
  negative: the DBRA counter as a word, up to 65536 lines; the step
  `(1<<14)/numlines` is a word quotient); the `$FFE1FFFE` wait (line
  256) is written whenever the low byte of the line is 0, also for line
  0 (#589, #588).
* `RedoColSplit`: `ASL.w #2` of the ccoffset argument shifts only the
  low word, the upper word is added unchanged (#594).
* `CopperWait`: x becomes `((x >> 1) & $FE) + 6` as a word, the +6 can
  carry into the line byte (#591).

**Test** (`libs/tests/rwe/ricopperfxlib.c`, snitch case "rwe
ricopperfxlib"): each run loads one library set (the C build or the CD
original); the scenario writes its observations as cases, compared with
the expected data `libs/tests/expected/rwe_ricopperfxlib.txt` recorded
from the original. Every command (both forms of `CopperReset`) is called
with markers in the other registers and garbage in the upper words of
word arguments; observed are d0 of the functions, the whole custom space
(an 8 KB pattern-filled buffer), the state block (pointers relative to
the buffer and coplist) and the kept registers. Cases: before any reset (nothing written), after
`CopperReset` with and without ccoffset, splits across line 256 and from
line 0, falling, rising and out-of-range colour values, `ColSplitBplcon3`,
40 random splits with `RedoColSplit` over them, `CopperResetAdr` with
the `GetCCOffset` -1, init/finit. Not covered: the lists running in the
copper (nothing is displayed), numlines 0 or negative, a ccoffset above
$FFFF for `RedoColSplit`.
