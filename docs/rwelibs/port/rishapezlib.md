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
block in the original's order, because of the first bug below. The saved
a4 is the global `rishapez_myold` (the original keeps it in its code).

## BUG(orig), kept

* `PeekByte` has no RTS behind its inline code: called as a subroutine
  (not inline) it runs on into `CludgeAcid` with d0 = the byte – a byte
  1 loads a4 with the saved value, any other byte saves a4 (#907).

## Test

Window and RastPort are memory of the test (a windowslib object = a
pointer to the Window). Compared: d0 and a4, the RastPort fields set
(the pattern by its words) and their neighbours, the kept registers.
Covered: `CludgeAcid` before anything was saved, 0/1 with garbage above
the low byte; `PeekByte` inline with the same length, and its code as a
subroutine with the bytes 0, 1, $80, $FF (the bug: a4 saved and loaded);
`SetWinDrawShapeZ` with modes 1, 0, $8000 and a word 0 with garbage
above it; init, finit.
