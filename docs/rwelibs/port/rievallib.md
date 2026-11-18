# rievallib (121): ConvToPostFix, PFEvaluate, PFRegisters, PFErrorType, PFErrorText, PFIndirectAddr

Reflective Images (RI Eval Lib V2.5). Integer expressions in strings:
`ConvToPostFix string$,buffer` converts an infix expression (`2+6`) to
postfix (`2 6 + `) in `buffer`; `PFEvaluate(string$)` evaluates a
postfix string. Operands: decimal, `$` hex, `%` binary numbers, the
registers `d0`–`d7`, `a0`–`a7`, `pc` of a register block set with
`PFRegisters`, and `{address}` indirection with `.b`/`.w`/`.l` (the long
at the address by default); operators `= & | < > + - * / ^`.
`PFErrorType`/`PFErrorText` give the last error, `PFIndirectAddr` the
address of the last indirection.

* **Source:** `rwelibsrc/EvalLib4.bb` (17/11/95; as TED shows it:
  `build/amiga/rwelibsrc/EvalLib4.bb`).
* **Reference:** BSS2 `RIEVALLIB.OBJ` (= AB3 `Rievallib.obj`), newer
  than the source (`docs/rwelibs/README.md` §3.2: the source plus the
  `{}` indirection, `PFIndirectAddr` and an error number fix);
  declarations equal (ctest `rwe_declarations_rievallib`). The reference
  build of the source only has to compile (`RWE_REFBUILD_NEWER`).
* **Variant:** `rwe-1999-eval` (below).
* **C file:** `libs/userlibs/rwe/rievallib.c`; test
  `libs/tests/rwe/rievallib.c` (snitch case "rwe rievallib").

## The number and the variant

RIEvalLib has library number 121 (`!libheader {#evallib,...}`, the BSS2
binary 121), the number of the acidlibs' polylib. Nothing on the CD
resolves it: every acidlibs of the CD (`BLITZ2/`, `BUM9A/`, `BUM10B/`)
has polylib at 121, no deflibs has polylib or RIEvalLib, BSS2 brings no
acidlibs or libnums, and its REDBlitzLibs documentation says nothing
about it (`blitzlib numbers` of each CD acidlibs with `RIEVALLIB.OBJ`:
"library number 121 twice"). With the CD's acidlibs, blitz2 would take
polylib for 121 (the first match, `docs/v2.10/libs.md` §2, not checked
in the emulator): RIEvalLib was usable only with polylib out of the
acidlibs or renumbered.

The number lists say where polylib belongs:

* `bblibsrc/polylib` and `bblibsrc/renderlib` are the same source: the
  render library ("3d rendering lib") with `!libheader {#renderlib,...}`
  and `; #renderlib=121` commented out, or with `#chunkylib=121`.
* The CD's `libnums.bb` has `#renderlib=87 ;* reserved`; RWE's "Official
  Blitz Library Number List" of 1997-02-02 (`docs/rwelibs/README.md`
  §3.3) has `#renderlib=87` and `#evallib=121`.
* AmiBlitz3's `sorted_libnums.ab3`: `#Rievallib=121`, `#renderlib=87
  ;* reserved Toby sent this to the list recently, except his was called
  polylib.obj ; It may also exist on some systems as libnum 121 (as it
  was on mine)`.

Design (the smallest change of the variant mechanism):

* Variant `libs/variants/rwe-1999-eval/libnums.h`: includes `rwe-1999`'s
  and changes `LIB_polylib` to 87 (`#renderlib`), adds `LIB_evallib` 121.
  Its acidlibs are bb2-2.10's with polylib's header and tokens at 87 (ctest
  `variant_rwe_1999_eval`: the token list is the CD's with lib 121 moved
  to 87; no library calls polylib).
* `RWE_OWN_VARIANTS` in `libs/userlibs/rwe/CMakeLists.txt` (`NAME|VARIANT`):
  rievallib is built with that variant's numbers to
  `build/amiga/variants/rwe-1999-eval/userlibs/rwe/rievallib.obj` and
  checked in its build step against that variant's acidlibs and deflibs.
  The other RWE userlibs are the same in both variants (no other number
  changes), so `rwe-1999-eval` = its acidlibs/deflibs + `rwe-1999`'s
  userlibs + rievallib. ctest `rwe_numbers_rwe-1999-eval`: no number
  twice in that set; `rwe_numbers_rwe-1999-eval_clash` (must fail):
  rievallib with `rwe-1999`'s acidlibs - the reason for the variant.
* Reference build: its `LIBNUMS.BB` gets the numbers of `rwe-1999` and
  of the own variants (`#evallib=121`, `#polylib=87`;
  `refbuild-libnums.cmake` takes a list of headers, a name in two of
  them with different numbers fails: `rwe_refbuild_libnums_twice`).
  EvalLib4 compiles with the CD's acidlibs (polylib 121 is not used by
  it); the result is not compared (the binary is the reference).

Alternative (not done): renumber polylib in `rwe-1999` itself – then
`rwe-1999`'s acidlibs would no longer be bb2-2.10's, for every RWE
userlib, although only RIEvalLib needs it. Leaving polylib out of the
variant's acidlibs instead of moving it is not possible with the variant
mechanism (a variant changes numbers, not the set of libraries) and would
drop a library the user had.

## The binary's state

The binary is EvalLib4 plus insertions (`.cache` analysis,
`versions/Eval.md`; checked again in the disassembly):

* ConvToPostFix copies `.`, `{` and `}` into operands; a `.` copies the
  next character with it, unchecked.
* PFEvaluate: `{` before an operand sets an indirect flag; the number
  loops (decimal, `$`, `%`) also end at `}`; after the operand (sign
  applied) the indirection checks that the last character read was `}`
  (error 5 "No close bracket on indirect!"), keeps the address for
  `PFIndirectAddr`, reads the long there and takes `.b` (the byte at the
  address), `.w` (the word), `.l` or none (the long); another size is
  error 6 "Invalid size for indirection!".
* "No result value!" is error 4 (the source sets 1, the V2.5
  documentation lists 4).
* `PFIndirectAddr` (new entry 5): the address of the last indirection.

## C and assembler

All C behind `BLITZ_THUNK`s (they keep d1/a0/a1, ConvToPostFix and
PFRegisters also d0; the original changes more). init/finit are empty
C functions.

The original keeps its stacks on a7 (ConvToPostFix the operators,
PFEvaluate the values; `stack` holds a7 at the start, errors restore it).
The C build keeps them in chunks of 32 longs: the first in the routine's
frame, a new one in a new frame (`grow()`, which then runs the rest of
the routine – every push of the original is followed by a jump to the
main loop) when the ones there are full; pops go down into the chunks
below, later pushes up again into those above, so the depth has no
limit but the stack (about 6 bytes per value instead of 4). `grow()`
is compiled without sibling calls, its chunk must outlive the call.

The registers that carry state from one operand or operator to the next
are fields of the routine's state, with the original's operand sizes
(`BL_SETB`/`BL_SETW`): d1 (the last character a number read – what the
indirection checks; a register in `{...}` sets it from the string, #637),
d7 (the precedence of the last operator), d0's upper bytes. lmult and
ldivide are the original's algorithms (MULU of the word halves with the
SWAPs, each swapped cross product with its low word cleared before it
is added, #639; the bit-by-bit division with its `ROXR`), changing d1
as there.
The value count `argc` is a long (the original's byte: #645).

## Fixed

Intended deviations from the original (`docs/libs/deviations.md`,
kind F):

* PFEvaluate counts its values in a long; the original's byte, compared
  signed, made every operator fail with "Need 2 arguments" from 128
  values on (#645).
* PFEvaluate of a string of spaces only returns 0 with error 4 "No
  result value!"; the original returned the long above its empty stack,
  the caller's return address (#644).
* PFErrorText without an error returns the length 0; the original set
  only the low word of d0, the upper word of the length was the
  caller's (#640).
* PFEvaluate's `<` is a signed "less than", as the manual says ("Test
  for less than") and the mirror image of `>`; the original's was
  unsigned "lower or same" (SLS): `2 2 <` true, `-1 1 <` false (#638).
* PFEvaluate takes the sign before `$` and `%`: `-$10` is -16, `-%1` -1;
  the original tested `$`/`%` first and read them behind a `-` as
  decimal digits (-24410, -2451) (#643).
* ConvToPostFix skips spaces in front of a `-` before it decides: a sign
  behind an operator, `(` or only spaces (`4* -2`, ` -5`), a binary
  minus behind an operand (`3 -4` is 3-4); the original looked at the
  character right before it only, a space made it a binary minus
  without an operand (#633).
* ConvToPostFix copies a `.` with the character behind it, but not the
  string's 0: a `.` at the end ends the operand. The original copied the
  0 too and went on converting the bytes behind the string (#631).
* PFEvaluate's `*` (also used by `^`) gives the low 32 bits of the
  product; the original's `SWAP : ADD` added the cross products' high
  words into the low word too, wrong whenever the product overflows
  (`1000000 1000000 *` was -727379962 instead of -727379968) (#639).
* PFEvaluate: a register in `{...}` (`{d0}`) is followed by its `}`,
  which the register path checks and consumes itself. The original
  checked d1, the last character a number loop read, stale for a
  register (the caller's d1, or what the last number, `*`, `/` or `^`
  left): `{d0}` mostly failed with error 5, and if it passed, the `}`
  was then read as a value 0 (#637).
* PFEvaluate: the indirection reads the size behind the `}` first and
  then that many bytes one by one (`.b` a byte, `.w` a word, `.l` or
  none a long; the same values): no word or long access at an odd
  address. The original always read a long and took its top byte or
  word, an address error on a 68000/68010 at an odd address, and on
  every processor 3 bytes more than `.b` asks for (#636; the bug-orig
  test `rievallib-indirect-odd-long` runs on the A600).
* PFEvaluate checks the digits of a number: a character that is no digit
  of its base (decimal 0-9, `$` 0-9 and a-f, `%` 0 and 1) is error 4 "No
  result value!" with the result 0. The original took every character up
  to the next space as a digit (`12x` was 192, `%2` 2, `$g` 16) (#634).
  Hex digits are lower case, as ConvToPostFix writes them.
* PFEvaluate: a register operand is `a0`-`a7`, `d0`-`d7` or `pc`,
  followed by a space, the end or (in `{...}`) its `}`; any other is
  error 4 "No result value!" with the result 0. The original took the
  character behind `a`/`d` unchecked as the index (`d9`, `dx`; behind a
  final `d` the string's 0, and it went on behind the string) and read a
  character behind a register as the next operand (`d01` was d0 and 1)
  (#642).
* PFEvaluate: more than one value left at the end of the string is
  error 4 "No result value!" with the result 0; the original returned
  the top value without an error (`1 2` was 2, `1 2 + 3` 3) (#635).
* ConvToPostFix keeps every character of an operand up to a space, an
  operator, `)` or the end; PFEvaluate rejects one that is no part of a
  number or register (error 4). The original ended the operand at a
  character that is no digit, a-f, register, `$ % . { }` and dropped it:
  `1,2` became `1 2 `, `12x+3` `12 3 + ` (15) (#632).
* PFEvaluate's `^` is the integer power: `x^0` = 1, `0^n` = 0 for n > 0,
  the low 32 bits of the power over all 32 bits of the exponent (square
  and multiply); a negative exponent gives the truncated value (0; 1 for
  the base 1; 1 or -1 for the base -1, by the exponent's parity), and
  `0^-n` is error 3 "Divide by 0 error!". The original returned 1 for
  the base 0, the base for the exponents 1, 0 and every negative one,
  and counted with `DBRA` on the low word of exponent-2 (`3 65538 ^` was
  9) (#641).

## BUG(orig), kept

None left.

No defects, like the original: ConvToPostFix leaves a3 pointing to the
string (a statement: nothing uses a3 after it); PFErrorType leaves the
caller's upper word of d0 (a word result).

## Test

`libs/tests/rwe/rievallib.c` (snitch case "rwe rievallib"): each run
loads one library set (the C build or the original); every routine is
observed as cases (2103 checks of the original), compared with the expected data
`libs/tests/expected/rwe_rievallib.txt` recorded from the original –
ConvToPostFix on 52 strings
(all operators and precedences, brackets in excess, signs, operands of
every kind, upper case, the `.` at the end with a tail behind the 0, 200
nested brackets and 150-deep operators), PFEvaluate on about 120 strings
(every operator with signs, overflow, division by 0, the power cases,
numbers and their quirks, registers without and with a block, a register
at the string end, `{address}` with every size and the errors,
`{register}` with the marker d1 and with d1 = `}`, 130 values (left
over and in a complete expression), 300
values with operators and pushes across chunk borders, empty, spaces
only, two results), after each PFErrorType, PFErrorText,
PFIndirectAddr; init/finit and the start state, and a second run
(PFRegisters and an error, then finit and init: PFErrorType and a
register after them). A run with `CHUNK` 4
passes too; mutations of the `.` copy and of `<` are caught.
