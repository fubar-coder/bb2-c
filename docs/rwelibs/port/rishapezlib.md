# rishapezlib (21): CludgeAcid, PeekByte, SetWinDrawShapeZ

Reflective Images, 1995. Three small helpers:

* `CludgeAcid on|off` – with d0.b = 1 a4 (the locals pointer of the
  Blitz program) is loaded with the value saved before; any other value
  saves a4. Before anything was saved, `CludgeAcid 1` sets a4 = 0.
* `PeekByte(address)` – the byte at the address (inline routine).
* `SetWinDrawShapeZ window#,mode` – the used window's RastPort gets the
  area pattern `$AAAA/$5555` (`AreaPtrn`, `AreaPtSz` 1) for mode ≠ 0,
  none for mode 0 (the window# argument is not used: the window is the
  used one, `!libs #windowslib,#ua0`). The same routine is MapEd's
  `SetWindowDraw` (`rimapeditorlib`).

init and finit do nothing.

* **Source:** `rwelibsrc/shapezlib.bb` (as TED shows it:
  `build/amiga/rwelibsrc/shapezlib.bb`).
* **Reference:** the reference build of the source by blitz2 2.10
  (`refbuild`, `build/amiga/libs/userlibs/rwe/ref/rishapezlib.obj`); no
  binary exists (`docs/rwelibs/README.md` §3.3). Declarations equal
  (ctest `rwe_declarations_rishapezlib`).
* **C file:** `libs/userlibs/rwe/rishapezlib.c`; test
  `libs/tests/rwe/rishapezlib.c` (snitch case "rwe rishapezlib").

## C and assembler

`SetWinDrawShapeZ` is C behind a thunk (keeps d0–d1/a0–a1; the original
changes a0 and, with a pattern, a1). `CludgeAcid` sets a4, which GCC has
fixed, and `PeekByte` is inline (`Dc $a000`): both stay assembler, in one
block in the original's order (see below). The saved a4 is the global
`rishapez_myold` (the original keeps it in its code).

## PeekByte without RTS: not a defect in compiled programs

`PeekByte` has no RTS behind its inline code: called as a subroutine it
would run on into `CludgeAcid` with d0 = the byte (a byte 1 loads a4 with
the saved value, any other byte saves a4). No compiled program calls it
so (#907): the compiler always copies an inline routine to the call
(`doajsr2`, blitz2.s:11541, `programs/blitz2/src/libcall.c`); in constant
evaluation it would emit a JSR into the library instead, but it allows
that only for a library whose header has the flag for it (`uselib3`,
"Can't use this Library!"), and rishapezlib's header has no flags; no
other library calls rishapezlib. Only the libs test calls the code as a
subroutine, which is why its cases show a4 saved and loaded.

## Test

Each run loads one library set (the C build or the original); the
scenario writes its observations as cases, compared with the expected
data `libs/tests/expected/rwe_rishapezlib.txt` recorded from the
original. Window and RastPort are memory of the test (a windowslib
object = a pointer to the Window). Observed: d0 and a4, the RastPort fields set
(the pattern by its words) and their neighbours, the kept registers.
Covered: `CludgeAcid` before anything was saved, 0/1 with garbage above
the low byte; `PeekByte` inline with the same length, and its code as a
subroutine with the bytes 0, 1, $80, $FF (a4 saved and loaded: the
fall-through into `CludgeAcid`, which compiled programs never reach);
`SetWinDrawShapeZ` with modes 1, 0, $8000 and a word 0 with garbage
above it; init, finit.
