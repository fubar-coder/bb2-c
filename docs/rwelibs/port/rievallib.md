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

## The number and the variant

RIEvalLib has library number 121 (`!libheader {#evallib,...}`, the BSS2
binary 121), the number of the acidlibs' polylib. Nothing on the CD
resolves it: every acidlibs of the CD (`BLITZ2/`, `BUM9A/`, `BUM10B/`)
has polylib at 121, no deflibs has polylib or RIEvalLib, BSS2 brings no
acidlibs or libnums, and its REDBlitzLibs documentation says nothing
about it (`biglib check` of each CD acidlibs with `RIEVALLIB.OBJ`:
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
