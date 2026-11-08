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
PFRegisters also d0; the original changes more). PFEvaluate's thunk
passes the caller's return address in a1 (`BLITZ_THUNK_PRE`, `12(sp)`):
for a string of spaces the original returns the long above its empty
stack, which is that address. init/finit are empty C functions.

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
indirection checks, also for a register, whose parse does not set it),
d7 (the precedence of the last operator), d0's upper bytes. lmult and
ldivide are the original's algorithms (MULU of the word halves with the
SWAPs, the bit-by-bit division with its `ROXR`), changing d1 as there.
The value count `argc` is a byte compared signed.

## BUG(orig), kept

In the C file's head: the `.` at the end of a string (ConvToPostFix
reads on behind the 0, #631), dropped characters (#632), signs after a
space (#633); `<` is unsigned "less or same", `>` signed (#638); the
product's high words added into the low word on overflow (#639); `x^0`
= `x^1` = `x`, `0^n` = 1, the exponent's low word as the loop count
(#641); 128 or more values make every operator fail (#645); extra values
are no error (#635); a string of spaces returns the caller's return
address with error 4 (#644); a register operand reads its index
character unchecked (also the string's 0, then it parses behind the
string) and any index (#642); `{register}` checks a stale d1, and the
`}` behind it is then a value 0 (#637); `-$10`/`-%1` read `$`/`%` as a
digit (#643); digits are not checked (#634); the indirection reads any
address (an odd one is an address error, #636); PFErrorText without an
error returns the caller's upper word of d0 as the length (#640).

No defects, like the original: ConvToPostFix leaves a3 pointing to the
string (a statement: nothing uses a3 after it); PFErrorType leaves the
caller's upper word of d0 (a word result).

## Test

`libs/tests/rwe/rievallib.c` (snitch case "rwe rievallib"): every
routine against the original, 2049 checks – ConvToPostFix on 52 strings
(all operators and precedences, brackets in excess, signs, operands of
every kind, upper case, the `.` at the end with a tail behind the 0, 200
nested brackets and 150-deep operators), PFEvaluate on about 120 strings
(every operator with signs, overflow, division by 0, the power cases,
numbers and their quirks, registers without and with a block, a register
at the string end, `{address}` with every size and the errors,
`{register}` with the marker d1 and with d1 = `}`, 130 values, 300
values with operators and pushes across chunk borders, empty, spaces
only, two results), after each PFErrorType, PFErrorText,
PFIndirectAddr; init/finit and the start state. A run with `CHUNK` 4
passes too; mutations of the `.` copy and of `<` are caught.
